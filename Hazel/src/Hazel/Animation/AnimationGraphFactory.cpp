#include "hzpch.h"
#include "AnimationGraphFactory.h"
#include "AnimationGraphPrototype.h"
#include "AnimationGraph.h"

#include "Nodes/AllNodes.h"

#include "Hazel/Utilities/StringUtils.h"

#include <CDT/CDT.h>

// Using the same string transformation here as in the editor nodes factory
// to translate NodeProcessor type to friendly name ensures that they are going to match.
// This allows us to minimize name coupling.
#define NODE(name) { Identifier(Hazel::Utils::CreateUserFriendlyTypeName(#name)), [](UUID nodeID) { return new name(#name, nodeID); } }
#define NODE_ALIAS(aliasName, processor) { Identifier(aliasName), [](UUID nodeID) { return new processor(aliasName, nodeID); } }

namespace Hazel::AnimationGraph {

	using Registry = std::unordered_map<Identifier, std::function<NodeProcessor* (UUID nodeID)>>;

	// This is the runtime node registry
	static const Registry NodeProcessors
	{
		// Animation nodes
		NODE(AnimationPlayer),
		NODE(SampleAnimation),

		// Array nodes
		NODE_ALIAS(Alias::GetAnimation, Get<int64_t>),
		NODE_ALIAS(Alias::GetRandomAnimation, GetRandom<int64_t>),
		NODE(Get<float>),
		NODE(Get<int>),
		
		// Blend nodes
		NODE(BlendSpace),
		NODE(BlendSpaceVertex),
		NODE(ConditionalBlend),
		NODE(OneShot),
		NODE(PoseBlend),
		NODE(RangedBlend),

		// Inverse Kinematics nodes
		NODE(AimIK),

		// Logic nodes
		NODE(And),
		NODE(CheckEqual<float>),
		NODE(CheckEqual<int>),
		NODE(CheckGreater<float>),
		NODE(CheckGreater<int>),
		NODE(CheckGreaterEqual<float>),
		NODE(CheckGreaterEqual<int>),
		NODE(CheckLess<float>),
		NODE(CheckLess<int>),
		NODE(CheckLessEqual<float>),
		NODE(CheckLessEqual<int>),
		NODE(CheckNotEqual<float>),
		NODE(CheckNotEqual<int>),
		NODE(Not),
		NODE(Or),

		// Math nodes
		NODE(Add<float>),
		NODE(Add<int>),
		NODE(Clamp<float>),
		NODE(Clamp<int>),
		NODE(Divide<float>),
		NODE(Divide<int>),
		NODE(Log),
		NODE(MapRange<float>),
		NODE(MapRange<int>),
		NODE(Max<float>),
		NODE(Max<int>),
		NODE(Min<float>),
		NODE(Min<int>),
		NODE(Modulo),
		NODE(Multiply<float>),
		NODE(Multiply<int>),
		NODE(Power),
		NODE(Subtract<float>),
		NODE(Subtract<int>),

		// State machine nodes
		NODE(QuickState),
		NODE(State),
		NODE(StateMachine),
		NODE_ALIAS(Alias::Transition, TransitionNode),

		// Trigger nodes
		NODE(BoolTrigger),
	};


	//==============================================================================
	NodeProcessor* Factory::Create(Identifier nodeTypeID, UUID nodeID)
	{
		if (!NodeProcessors.count(nodeTypeID))
		{
			if (!nodeTypeID.GetDBGName().empty())
				HZ_CORE_ERROR("AnimationGraph::Factory::Create - Node with type ID {} is not in the registry", nodeTypeID.GetDBGName());
			else
				HZ_CORE_ERROR("AnimationGraph::Factory::Create - Node with type ID {} is not in the registry", (uint32_t)nodeTypeID);

			return nullptr;
		}

		return NodeProcessors.at(nodeTypeID)(nodeID);
	}


	bool Factory::Contains(Identifier nodeTypeID)
	{
		return NodeProcessors.count(nodeTypeID);
	}


	bool ValidateGraph(const Graph& graph)
	{
		// TODO: add proper compilation/validation error messages
		//std::vector<std::string> errors;

		auto validateElements = [](const auto& elements, const auto& test)
		{
			for (const auto& element : elements)
			{
				if (!test(element))
				{
					HZ_CORE_ASSERT(false);
					return false;
				}
			}

			return true;
		};

		using EndpointStream = std::unique_ptr<StreamWriter>;

		const auto validateEndpoints = [](const EndpointStream& endpointStream)
		{
			if (endpointStream->DestinationID)
			{
				if (!endpointStream->outV.getRawData())
					return false;
				else
					return true;
			}
			else
			{
				return false;
			}
		};

		if (!validateElements(graph.EndpointOutputStreams.Ins, [](const std::pair<const Identifier, const choc::value::ValueView>& in) { return (bool)in.second.getRawData(); }))
		{
			HZ_CORE_ASSERT(false);
			return false;
		}

		for (auto& node : graph.Nodes)
		{
			bool pass = true;
			//if (!validateElements(node->Ins, [](const std::pair<const Identifier, const choc::value::ValueView>& in) { return (bool)in.second.getRawData(); })) pass = false;
			if (!validateElements(node->Outs, [](const std::pair<const Identifier, const choc::value::ValueView>& out) { return (bool)out.second.getRawData(); })) pass = false;
			if (!validateElements(node->InEvs, [](const std::pair<const Identifier, const NodeProcessor::InputEvent>& inEv) { return (bool)inEv.second.Event; })) pass = false;

			// We probably don't need to test output events, they use vectors of destinations internally, which may be empty
			//if (!validateElements(node->OutEvs, [](const std::pair<const Identifier, const NodeProcessor::OutputEvent>& outEv) { return (bool)!outEv.second.DestinationEvs.empty(); })) pass = false;

			HZ_CORE_ASSERT(pass);
			if (!pass)
				return false;
		}

		return true;
	}


	Scope<StateMachine> CreateStateMachineInstance(Ref<AnimationGraph> root, UUID id, const Prototype& prototype);
	Scope<State> CreateStateInstance(Ref<AnimationGraph> root, UUID id, const Prototype& prototype);
	Scope<QuickState> CreateQuickStateInstance(const Prototype::Node& subNode);
	Scope<TransitionNode> CreateTransitionInstance(Ref<AnimationGraph> root, UUID id, const Prototype& prototype, StateBase* sourceState, StateBase* destState);

	bool InitStateMachine(Ref<AnimationGraph> root, StateMachine* stateMachine, const Prototype& prototype)
	{
		// Construct an instance for each state, and add it to state machine's list of states
		size_t numStates = 0;
		for (auto& node : prototype.Nodes)
		{
			if ((node.TypeID == IDs::StateMachine) || (node.TypeID == IDs::State) || (node.TypeID == IDs::QuickState))
			{
				++numStates;
			}
		}

		if (numStates == 0)
		{
			return false;
		}

		stateMachine->m_States.reserve(numStates);
		for (auto& subnode : prototype.Nodes)
		{
			if (subnode.TypeID == IDs::StateMachine)
			{
				HZ_CORE_ASSERT(subnode.Subroutine, "State Machine node has no subroutine! (check graph compilation)");
				if (!stateMachine->m_States.emplace_back(CreateStateMachineInstance(root, subnode.ID, *subnode.Subroutine)))
				{
					return false;
				}
			}
			else if (subnode.TypeID == IDs::State)
			{
				HZ_CORE_ASSERT(subnode.Subroutine, "State node has no subroutine! (check graph compilation)");
				if (!stateMachine->m_States.emplace_back(CreateStateInstance(root, subnode.ID, *subnode.Subroutine)))
				{
					return false;
				}
			}
			else if (subnode.TypeID == IDs::QuickState)
			{
				if (!stateMachine->m_States.emplace_back(CreateQuickStateInstance(subnode)))
				{
					return false;
				}
			}
			stateMachine->m_States.back()->m_Parent = stateMachine;
		}

		// Having created all the states, we can now do transitions
		for (auto& subnode : prototype.Nodes)
		{
			if ((subnode.TypeID == IDs::Transition))
			{
				UUID sourceStateID = 0;
				UUID destStateID = 0;
				for (auto& connection : prototype.Connections)
				{
					// We create TransitionNodes for each "connection".
					if (connection.Source.NodeID == subnode.ID)
					{
						destStateID = connection.Destination.NodeID;
					}
					else if (connection.Destination.NodeID == subnode.ID)
					{
						sourceStateID = connection.Source.NodeID;
					}
				}
				HZ_CORE_ASSERT(sourceStateID != 0, "Transition node should have a source state");
				HZ_CORE_ASSERT(destStateID != 0, "Transition node should have a destination state");
				StateBase* sourceNode = nullptr;
				StateBase* destNode = nullptr;
				for (auto& state : stateMachine->m_States)
				{
					if (state->ID == sourceStateID)
					{
						sourceNode = state.get();
					}
					else if (state->ID == destStateID)
					{
						destNode = state.get();
					}
				}
				HZ_CORE_ASSERT(sourceNode, "Transition node source state does not exist");
				HZ_CORE_ASSERT(destNode, "Transition node destination state does not exist");
				if (!sourceNode->m_Transitions.emplace_back(CreateTransitionInstance(root, subnode.ID, *subnode.Subroutine, sourceNode, destNode))) {
					return false;
				}
				sourceNode->m_Transitions.back()->m_Parent = stateMachine;
			}
		}

		return true;
	}


	Scope<BlendSpaceVertex> CreateBlendSpaceVertexInstance(const Prototype::Node& subnode);

	bool InitBlendSpace(BlendSpace* blendSpace, const float lerpSecondsPerUnitX, const float lerpSecondsPerUnitY, const Prototype& prototype)
	{
		blendSpace->m_LerpSecondsPerUnitX = lerpSecondsPerUnitX;
		blendSpace->m_LerpSecondsPerUnitY = lerpSecondsPerUnitY;
		size_t numVertices = 0;
		for (auto& node : prototype.Nodes)
		{
			++numVertices;
		}

		if (numVertices == 0)
		{
			return false;
		}

		blendSpace->m_Vertices.reserve(numVertices);
		for (auto& subnode : prototype.Nodes)
		{
			if (!blendSpace->m_Vertices.emplace_back(CreateBlendSpaceVertexInstance(subnode)))
			{
				return false;
			}
		}

		return true;
	}


	bool InitInstance(Ref<Graph> instance, Ref<AnimationGraph> root, const Prototype& prototype)
	{
		// Construct IO
		for (auto& input : prototype.Inputs)
		{
			instance->AddGraphInputStream(input.ID, choc::value::Value(input.DefaultValue));
		}
		for (auto& output : prototype.Outputs)
		{
			instance->AddGraphOutputStream(output.ID, output.DefaultValue);
		}

		// Local Variables
		for (const auto& localVariable : prototype.LocalVariablePlugs)
		{
			instance->AddLocalVariableStream(localVariable.ID, localVariable.DefaultValue);
		}

		// Construct Nodes
		instance->Nodes.reserve(prototype.Nodes.size());
		for (auto& pnode : prototype.Nodes)
		{
			instance->AddNode(Factory::Create(pnode.TypeID, pnode.ID));
			auto& node = instance->Nodes.back();

			float lerpSecondsPerUnitX = 0.0f;
			float lerpSecondsPerUnitY = 0.0f;
			for (const Prototype::Endpoint& defaultValuePlug : pnode.DefaultValuePlugs)
			{
				if (defaultValuePlug.ID == IDs::LerpSecondsPerUnitX)
				{
					lerpSecondsPerUnitX = defaultValuePlug.DefaultValue.getFloat32();
				}
				else if (defaultValuePlug.ID == IDs::LerpSecondsPerUnitY)
				{
					lerpSecondsPerUnitY = defaultValuePlug.DefaultValue.getFloat32();
				}
				else
				{
					node->DefaultValuePlugs.emplace_back(new StreamWriter{ defaultValuePlug.DefaultValue, defaultValuePlug.ID });
					node->InValue(defaultValuePlug.ID) = node->DefaultValuePlugs.back()->outV;
				}
			}

			if (pnode.TypeID == IDs::StateMachine)
			{
				StateMachine* stateMachine = static_cast<StateMachine*>(node.get());
				if (!InitStateMachine(root, stateMachine, *pnode.Subroutine))
				{
					return false;
				}
			}
			else if (pnode.TypeID == IDs::BlendSpace)
			{
				BlendSpace* blendSpace = static_cast<BlendSpace*>(node.get());
				if(!InitBlendSpace(blendSpace, lerpSecondsPerUnitX, lerpSecondsPerUnitY, *pnode.Subroutine))
				{
					return false;
				}
			}
			else if (pnode.TypeID == IDs::ConditionalBlend)
			{
				ConditionalBlend* conditionalBlend = static_cast<ConditionalBlend*>(node.get());
				if (!InitConditionalBlend(root, conditionalBlend, *pnode.Subroutine))
				{
					return false;
				}
			}
			else if (pnode.TypeID == IDs::OneShot)
			{
				OneShot* oneShot = static_cast<OneShot*>(node.get());
				if (!InitOneShot(root, oneShot, *pnode.Subroutine))
				{
					return false;
				}
			}
		}

		// Construct Connections
		for (const auto& connection : prototype.Connections)
		{
			const auto& source = connection.Source;
			const auto& dest = connection.Destination;

			// TODO: add proper error message depending on which one of these fail
			switch (connection.Type)
			{
				case Prototype::Connection::NodeValue_NodeValue:
					if (!instance->AddValueConnection(source.NodeID, source.EndpointID, dest.NodeID, dest.EndpointID)) return false;
					break;
				case Prototype::Connection::NodeEvent_NodeEvent:
					if (!instance->AddEventConnection(source.NodeID, source.EndpointID, dest.NodeID, dest.EndpointID)) return false;
					break;
				case Prototype::Connection::GraphValue_NodeValue:
					if (!instance->AddInputValueRoute(root, source.EndpointID, dest.NodeID, dest.EndpointID)) return false;
					break;
				case Prototype::Connection::GraphValue_GraphValue:
					if (!instance->AddInputValueRouteToGraphOutput(root, source.EndpointID, dest.EndpointID)) return false;
					break;
				case Prototype::Connection::GraphEvent_NodeEvent:
					if (!instance->AddInputValueRouteToEvent(root, source.EndpointID, dest.NodeID, dest.EndpointID)) return false;
					break;
				case Prototype::Connection::NodeValue_GraphValue:
					if (!instance->AddToGraphOutputConnection(source.NodeID, source.EndpointID, dest.EndpointID)) return false;
					break;
				case Prototype::Connection::NodeEvent_GraphEvent:
					if (!instance->AddToGraphOutEventConnection(source.NodeID, source.EndpointID, root, IDs::Event, dest.EndpointID)) return false;
					break;
				case Prototype::Connection::GraphEvent_GraphEvent:
					if (!instance->AddInputValueRouteToGraphOutEventConnection(root, source.EndpointID, dest.EndpointID)) return false;
					break;
				case Prototype::Connection::LocalVariable_NodeValue:
					if (!instance->AddLocalVariableRoute(source.EndpointID, dest.NodeID, dest.EndpointID)) return false;
					break;
				case Prototype::Connection::LocalVariable_GraphValue:
					if (!instance->AddLocalVariableRouteToGraphOutput(source.EndpointID, dest.EndpointID)) return false;
					break;
				default: HZ_CORE_ASSERT(false);
					return false;
					break;
			}
		}

		return ValidateGraph(*instance);
	}


	Scope<StateMachine> CreateStateMachineInstance(Ref<AnimationGraph> root, UUID id, const Prototype& prototype)
	{
		auto instance = CreateScope<StateMachine>("StateMachine", id);

		if (!InitStateMachine(root, instance.get(), prototype))
		{
			return nullptr;
		}

		return instance;
	}


	Scope<State> CreateStateInstance(Ref<AnimationGraph> root, UUID id, const Prototype& prototype)
	{
		auto instance = CreateScope<State>("State", id);
		instance->m_AnimationGraph = Ref<AnimationGraph>::Create(prototype.DebugName, prototype.ID, root->GetSkeleton());

		if (!InitInstance(instance->m_AnimationGraph, root, prototype))
		{
			return nullptr;
		}
		return instance;
	}


	Scope<QuickState> CreateQuickStateInstance(const Prototype::Node& subnode)
	{
		auto instance = CreateScope<QuickState>("QuickState", subnode.ID);

		for (const Prototype::Endpoint& defaultValuePlug : subnode.DefaultValuePlugs)
		{
			instance->DefaultValuePlugs.emplace_back(new StreamWriter{ defaultValuePlug.DefaultValue, defaultValuePlug.ID });
			instance->InValue(defaultValuePlug.ID) = instance->DefaultValuePlugs.back()->outV;
		}
		return instance;
	}


	Scope<TransitionNode> CreateTransitionInstance(Ref<AnimationGraph> root, UUID id, const Prototype& prototype, StateBase* sourceState, StateBase* destState)
	{
		auto instance = CreateScope<TransitionNode>("Transition", id);
		instance->m_SourceState = sourceState;
		instance->m_DestinationState = destState;

		instance->m_ConditionGraph = Ref<Graph>::Create(prototype.DebugName, prototype.ID);

		if (!InitInstance(instance->m_ConditionGraph, root, prototype))
		{
			return nullptr;
		}
		return instance;
	}


	Scope<BlendSpaceVertex> CreateBlendSpaceVertexInstance(const Prototype::Node& subnode)
	{
		auto instance = CreateScope<BlendSpaceVertex>("BlendSpaceVertex", subnode.ID);

		for (const Prototype::Endpoint& defaultValuePlug : subnode.DefaultValuePlugs)
		{
			if (defaultValuePlug.ID == IDs::X)
			{
				instance->X = defaultValuePlug.DefaultValue.getFloat32();
			}
			else if (defaultValuePlug.ID == IDs::Y)
			{
				instance->Y = defaultValuePlug.DefaultValue.getFloat32();
			}
			else
			{
				instance->DefaultValuePlugs.emplace_back(new StreamWriter{ defaultValuePlug.DefaultValue, defaultValuePlug.ID });
				instance->InValue(defaultValuePlug.ID) = instance->DefaultValuePlugs.back()->outV;
			}
		}
		return instance;
	}


	bool InitConditionalBlend(Ref<AnimationGraph> root, ConditionalBlend* conditionalBlend, const Prototype& prototype)
	{
		conditionalBlend->m_AnimationGraph = Ref<AnimationGraph>::Create(prototype.DebugName, prototype.ID, root->GetSkeleton());
		return InitInstance(conditionalBlend->m_AnimationGraph, root, prototype);
	}


	bool InitOneShot(Ref<AnimationGraph> root, OneShot* oneShot, const Prototype& prototype)
	{
		oneShot->m_AnimationGraph = Ref<AnimationGraph>::Create(prototype.DebugName, prototype.ID, root->GetSkeleton());
		return InitInstance(oneShot->m_AnimationGraph, root, prototype);
	}


	Ref<AnimationGraph> CreateInstance(const Prototype& prototype)
	{
		auto instance = Ref<AnimationGraph>::Create(prototype.DebugName, prototype.ID, prototype.Skeleton);
		if (!InitInstance(instance, instance, prototype))
		{
			return nullptr;
		}
		return instance;
	}

} // namespace Hazel::AnimationGraph

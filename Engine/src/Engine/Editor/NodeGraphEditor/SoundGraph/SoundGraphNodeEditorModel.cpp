#include "hzpch.h"
#include "SoundGraphNodeEditorModel.h"

#include "Engine/Audio/AudioEngine.h"
#include "Engine/Audio/SoundGraph/SoundGraph.h"
#include "Engine/Audio/SoundGraph/SoundGraphSource.h"
#include "Engine/Audio/SoundGraph/GraphGeneration.h" 

#include "Engine/Asset/AssetManager.h"

#include "Engine/Project/Project.h"

#include "Engine/Utilities/ContainerUtils.h"
#include "Engine/Utilities/FileSystem.h"

#include "imgui-node-editor/imgui_node_editor.h"

#include "choc/text/choc_UTF8.h"
#include "choc/text/choc_Files.h"
#include "choc/text/choc_JSON.h"

#include <limits>

static constexpr auto MAX_NUM_CACHED_GRAPHS = 256; // TODO: get this value from somewhere reasonable

namespace SG = Hazel::SoundGraph;

namespace Hazel {
	//? DBG. Print values of input nodes
	void NodeDump(const Ref<SG::SoundGraph>& soundGraph, UUID nodeID, std::string_view dumpIdentifier)
	{
		if (const auto* node = soundGraph->FindNodeByID(7220023940811122597))
		{
			HZ_CORE_WARN("======================");
			HZ_CORE_WARN("NODE DUMP");
			HZ_CORE_WARN(dumpIdentifier);
			HZ_CORE_TRACE("----------------------");
			HZ_CORE_TRACE("{}", node->dbgName);
			for (const auto& [id, value] : node->Ins)
			{
				HZ_CORE_TRACE("[{0}] {1}", id, choc::json::toString(value));
			}
			HZ_CORE_TRACE("----------------------");
		}
	}

	//static const char* s_PatchDirectory = "Resources/Cache/Shader/ShaderRegistry.cache";

	SoundGraphNodeEditorModel::SoundGraphNodeEditorModel(Ref<SoundGraphAsset> graphAsset)
		: m_GraphAsset(graphAsset)
	{
		// Initialize preview player
		m_SoundGraphSource = CreateScope<SoundGraphSource>();
		Audio::AudioCallback::BusConfig busConfig;
		busConfig.InputBuses.push_back(2);
		busConfig.OutputBuses.push_back(2);
		m_SoundGraphSource->Initialize(MiniAudioEngine::GetMiniaudioEngine(), busConfig);

		// Need to make sure the Context / Player is running before posting any events or parameter changes
		m_SoundGraphSource->StartNode();
		m_SoundGraphSource->SuspendProcessing(true);
		// Wait for audio Process Block to stop
		while (!m_SoundGraphSource->IsSuspended()) {}

		// Deserialize. Check if we have compiled graph prototype in cache
		std::filesystem::path cacheDirectory = Project::GetCacheDirectory() / "SoundGraph";
		m_Cache = CreateScope<SoundGraphCache>(cacheDirectory, MAX_NUM_CACHED_GRAPHS);

		Deserialize();

		onPinValueChanged = [&](UUID nodeID, UUID pinID)
		{
			if (const auto* node = FindNode(nodeID))
			{
				if (IsLocalVariableNode(node))
					SetPropertyValue(EPropertyType::LocalVariable, node->Name, node->Inputs[0]->Value);
			}

			InvalidatePlayer();
		};

		onCompile = [&] (Ref<Asset> graphAsset)
		{
			if (!Sort())
				return false;

			auto graph = graphAsset.As<SoundGraphAsset>();
			const AssetMetadata& md = Project::GetEditorAssetManager()->GetMetadata(graph);
			const std::string name = md.FilePath.stem().string();

			SG::GraphGeneratorOptions options
			{
				name,
				0,
				2,
				graphAsset,
				this,
				m_Cache.get()
			};
			 
			auto soundGraphPrototype = SG::ConstructPrototype(options, graph->WaveSources);

			if (!soundGraphPrototype)
				return false;

			auto soundGraph = SG::CreateInstance(soundGraphPrototype);

			// Return if failed to "compile" or validate graph
			if (!soundGraph)
				return false;

			graph->Prototype = soundGraphPrototype;

			// 4. Prepare streaming wave sources
			m_SoundGraphSource->SuspendProcessing(true);
			// Wait for audio Process Block to stop
			while (!m_SoundGraphSource->IsSuspended()) {}

			m_SoundGraphSource->UninitializeDataSources();
			m_SoundGraphSource->InitializeDataSources(graph->WaveSources);

			// 5. Update preview player with the new compiled patch player
			m_SoundGraphSource->ReplacePlayer(soundGraph);

			// 6. Initialize player's default parameter values (graph "preset")
			if (soundGraph)
			{
				bool parametersSet = m_SoundGraphSource->ApplyParameterPreset(graph->GraphInputs);

				// Preset must match parameters (input events) of the compiled patch player 
				HZ_CORE_ASSERT(parametersSet);
			}

			return soundGraph != nullptr;
		};

		onPlay = [&]
		{
			// in case graph is still processing, suspend
			m_SoundGraphSource->SuspendProcessing(true);
			// Wait for audio Process Block to stop
			while (!m_SoundGraphSource->IsSuspended()) {}
			
			// reset graph to initial state
			m_SoundGraphSource->GetGraph()->Reinit();
			m_SoundGraphSource->SuspendProcessing(false);

			if (m_SoundGraphSource->SendPlayEvent())
			{
				//? DBG
				//HZ_CORE_INFO("Posted Play");
			}
		};

		onStop = [&](bool onClose)
		{
			if (auto graph = m_SoundGraphSource->GetGraph())
			{
				// TODO: fade out before stopping?
				// TODO: clear buffer

				m_SoundGraphSource->SuspendProcessing(true);
				// Wait for audio Process Block to stop
				while (!m_SoundGraphSource->IsSuspended()) {}

				graph->Reset(); 

				if (!onClose && !IsPlayerDirty())
				{
					// Need to reinitialize parameters after the player has been reset
					bool parametersSet = m_SoundGraphSource->ApplyParameterPreset(m_GraphAsset->GraphInputs);

					// Preset must match parameters (input events) of the compiled patch player 
					HZ_CORE_ASSERT(parametersSet);
				}
			}
		};
	}

	SoundGraphNodeEditorModel::~SoundGraphNodeEditorModel()
	{
		m_SoundGraphSource->Uninitialize();
	}

	bool SoundGraphNodeEditorModel::SaveGraphState(const char* data, size_t size)
	{
		m_GraphAsset->GraphState = data;
		return true;
	}

	const std::string& SoundGraphNodeEditorModel::LoadGraphState()
	{
		return m_GraphAsset->GraphState;
	}

	void SoundGraphNodeEditorModel::AssignSomeDefaultValue(Pin* pin)
	{
		if (!pin || pin->Kind == PinKind::Output)
			return;

		// First we try to get default value override from the factory
		if (const Node* node = FindNode(pin->NodeID))
		{
			if (auto optValueOverride = SoundGraph::GetPinDefaultValueOverride(choc::text::replace(node->Name, " ", ""), choc::text::replace(pin->Name, " ", "")))
			{
				pin->Value = *optValueOverride;
				return;
			}
		}

		if (pin->Storage == StorageKind::Array)
		{
			// TODO: JP. in the future we might want to store actual array type since this is editor-only nodes
			pin->Value = choc::value::Value();
			return;
		}

		// If we couldn't find the default value override in the factory,
		// we construct a default node of the same type and get the default value from it.
		const Pin* defaultPin = SG::SGTypes::CreatePinForType(pin->GetType());

		pin->Value = defaultPin->Value;

		delete defaultPin;
	}

	void SoundGraphNodeEditorModel::OnCompileGraph()
	{
		HZ_CORE_INFO_TAG("Audio", "Compiling graph...");
		
		//SaveAll();

		if (onCompile)
		{
			if (onCompile(m_GraphAsset.As<Asset>()))
			{
				InvalidatePlayer(false);

				HZ_CORE_INFO_TAG("Audio", "Graph has been compiled.");
				if (onCompiledSuccessfully)
					onCompiledSuccessfully();

				HZ_CONSOLE_LOG_TRACE("Sound graph successfully compiled.");

				// Saving newly created prototype.
				// If failed to compile, the old prototype is going to be used
				// for playback
				SaveAll();
			}
			else
			{
				HZ_CONSOLE_LOG_ERROR("Failed to compile sound graph!");
			}
		}
		else
		{
			HZ_CONSOLE_LOG_ERROR("Failed to compile sound graph!");
		}
	}


	bool SoundGraphNodeEditorModel::Sort()
	{
		for (auto& node : m_GraphAsset->Nodes)
			node->SortIndex = Node::UndefinedSortIndex;

		// TODO: get better candidate for the starting node?

		// For now we always start sorting form Output Audio, since our graph must play audio.
		// This might not be the case in the future when we implement nested graph processors/modules
		// that going to be able to do processiong on just value streams.
		Node* sortEndpointNode = FindNodeByName("Output Audio");
		if (!sortEndpointNode)
			sortEndpointNode = FindNodeByName("Input Action");

		if (!sortEndpointNode)
		{
			// TODO: JP. there may be cases when the user just saves graph without playable endpoints for the future,
			//		editor shouldn't crash in that case, need to handle it softer
			// TODO: JP. perhaps we could display a warning or error icon on top of the "Compile" button
			//		to show that compilation has failed and the graph is invalid
			HZ_CORE_ERROR("Failed to find endpoint node to sort the graph!");
			//HZ_CORE_ASSERT(false);
			return false;
		}

		uint32_t currentIndex = 0;
		sortEndpointNode->SortIndex = currentIndex++;

		auto correctSort = [&](Node* context, Node* previous, auto& correctSort) -> void
		{
			// Local variables are a special case,
			// we treat all nodes coresponding to the same LV
			// as the same node
			if (IsLocalVariableNode(context))
			{
				std::vector<Node*> localVarNodes;
				std::vector<const Pin*> localVarOutputs;

				const auto isSetter = [](const Node* n) { return n->Inputs.size() == 1; };

				volatile int32_t localVarSortIndex = context->SortIndex;

				// Collect all of the nodes corresponding to this Local Variable
				for (auto& n : GetNodes())
				{
					if (IsSameLocalVariableNodes(n, context))
						localVarNodes.push_back(n);
				}

				// Go over inputs, which should be just one
				// becase its value can reference only one output
				for (auto* n : localVarNodes)
				{
					if (isSetter(n))
					{
						// Find node connected to this input
						Node* node = GetNodeConnectedToPin(n->Inputs[0]->ID);

						// Local variable setter might not have conntected input,
						// we still need to assign it some valid sort index.
						if (!node && n->SortIndex == Node::UndefinedSortIndex)
						{
							n->SortIndex = currentIndex++;
							localVarSortIndex = n->SortIndex;
						}

						if (!node || node == previous || node->SortIndex != Node::UndefinedSortIndex) continue;

						correctSort(node, n, correctSort);

						if (n->SortIndex == Node::UndefinedSortIndex)
						{
							n->SortIndex = currentIndex++;
							localVarSortIndex = n->SortIndex;
						}

						// in some complex cases we can get into this situation, where we get to the same LV node from different branches
						if (context->SortIndex != Node::UndefinedSortIndex && localVarSortIndex == Node::UndefinedSortIndex)
							localVarSortIndex = context->SortIndex;

						break;
					}
				}
				HZ_CORE_ASSERT(localVarSortIndex != Node::UndefinedSortIndex);

				// Collect outputs from all instances of getter nodes
				for (auto* n : localVarNodes)
				{
					if (!isSetter(n))
					{
						localVarOutputs.push_back(n->Outputs[0]);
						if (n->SortIndex == Node::UndefinedSortIndex)
							n->SortIndex = localVarSortIndex;
					}
				}
				// Go over collected outputs
				for (const auto* out : localVarOutputs)
				{
					// Find node connected to this output
					Node* node = GetNodeConnectedToPin(out->ID);
					if (!node || node == previous || node->SortIndex != Node::UndefinedSortIndex) continue;

					correctSort(node, context, correctSort);
				}
			}
			else // Non Local Varialbe nodes are much simpler case
			{
				for (const auto& in : context->Inputs)
				{
					// Find node connected to this input
					Node* node = GetNodeConnectedToPin(in->ID);
					if (!node || node == previous || node->SortIndex != Node::UndefinedSortIndex) continue;

					correctSort(node, context, correctSort);
				}

				if (context->SortIndex == Node::UndefinedSortIndex)
					context->SortIndex = currentIndex++;

				for (auto& out : context->Outputs)
				{
					// Find node connected to this output
					Node* node = GetNodeConnectedToPin(out->ID);
					if (!node || node == previous || node->SortIndex != Node::UndefinedSortIndex) continue;

					correctSort(node, context, correctSort);
				}
			}
		};
		correctSort(sortEndpointNode, nullptr, correctSort);

		// Sort Nodes
		std::vector<Node*> sortedNodes = m_GraphAsset->Nodes;

		std::sort(sortedNodes.begin(), sortedNodes.end(), [&](const Node* first, const Node* second)
		{
			if (first->SortIndex == Node::UndefinedSortIndex)
				return false;
			if (second->SortIndex == Node::UndefinedSortIndex)
				return true;
			return first->SortIndex < second->SortIndex;
		});

		m_GraphAsset->Nodes = sortedNodes;

		for (uint32_t i = 0; i < m_GraphAsset->Nodes.size(); ++i)
		{
			auto& node = m_GraphAsset->Nodes[i];
			if (node->SortIndex == Node::UndefinedSortIndex)
				continue;

			node->SortIndex = i;
		}

		// Sort Links
		std::vector<Link> sortedLinks = m_GraphAsset->Links;
		std::sort(sortedLinks.begin(), sortedLinks.end(), [&](const Link& first, const Link& second)
		{
			bool isLess = false;

			auto getIndex = [](auto vec, auto valueFunc)
			{
				auto it = std::find_if(vec.begin(), vec.end(), valueFunc);
				HZ_CORE_ASSERT(it != vec.end());
				return it - vec.begin();
			};

			const Node* node1 = GetNodeForPin(first.StartPinID);
			const Node* node2 = GetNodeForPin(second.StartPinID);

			//if (node1 == node2)
			{
				// TODO: check index of pins in the nodes
			}

			const size_t index1 = getIndex(m_GraphAsset->Nodes, [node1](const Node* n) { return n->ID == node1->ID; });
			const size_t index2 = getIndex(m_GraphAsset->Nodes, [node2](const Node* n) { return n->ID == node2->ID; });

			if (index1 != index2)
			{
				isLess = index1 < index2;
			}
			else
			{
				const Node* nodeEnd1 = GetNodeForPin(first.EndPinID);
				const Node* nodeEnd2 = GetNodeForPin(second.EndPinID);

				//if (nodeEnd1 == nodeEnd2)
				{
					// TODO: check index of pins in the nodes
				}

				const size_t indexEnd1 = getIndex(m_GraphAsset->Nodes, [node1](const Node* n) { return n->ID == node1->ID; });
				const size_t indexEnd2 = getIndex(m_GraphAsset->Nodes, [node2](const Node* n) { return n->ID == node2->ID; });

				//if (indexEnd1 == indexEnd2)
				{
					// TODO: check index of pins in the nodes
				}

				isLess = indexEnd1 < indexEnd2;
			}

			return isLess;
		});

		m_GraphAsset->Links = sortedLinks;

		return true;
	}

	void SoundGraphNodeEditorModel::Deserialize()
	{
		// If we have cached prototype, create graph playback instance
		if (!m_GraphAsset->CachedPrototype.empty())
		{
			// TODO: check if m_GraphAsset is newer than cached prototype, if yes, potentially invalidate cache

			const AssetMetadata& md = Project::GetEditorAssetManager()->GetMetadata(m_GraphAsset);
			const std::string name = md.FilePath.stem().string();

			if (Ref<SG::Prototype> prototype = m_Cache->ReadPtototype(name.c_str()))
			{
				auto soundGraph = SG::CreateInstance(prototype);
				// Return if failed to "compile" or validate graph
				if (!soundGraph)
				{
					InvalidatePlayer(true);
					return;
				}

				m_GraphAsset->Prototype = prototype;

				// TODO: validate Prototype graph

				// 4. Prepare streaming wave sources
				m_SoundGraphSource->SuspendProcessing(true);
				// Wait for audio Process Block to stop
				while (!m_SoundGraphSource->IsSuspended()) {}

				m_SoundGraphSource->UninitializeDataSources();
				m_SoundGraphSource->InitializeDataSources(m_GraphAsset->WaveSources);

				// 5. Update preview player with the new compiled patch player
				m_SoundGraphSource->ReplacePlayer(soundGraph);

				// 6. Initialize player's default parameter values (graph "preset")
				if (soundGraph)
				{
					bool parametersSet = m_SoundGraphSource->ApplyParameterPreset(m_GraphAsset->GraphInputs);
					
					// Preset must match parameters (input events) of the compiled patch player 
					InvalidatePlayer(!parametersSet);
				}
			}
		}
	}


	void SoundGraphNodeEditorModel::OnGraphPropertyValueChanged(EPropertyType type, const std::string& inputName)
	{
		IONodeEditorModel::OnGraphPropertyValueChanged(type, inputName);

		// If player is dirty, need to recompile it first and update the parameter set
		// before sending new values from the GUI
		if (!IsPlayerDirty() && type == EPropertyType::Input)
		{
			// Call parameter change for the live preview player
			auto newValue = GetPropertySet(type).GetValue(inputName);
			bool set = m_SoundGraphSource->SetParameter(choc::text::replace(inputName, " ", ""), newValue);
			HZ_CORE_ASSERT(set);
		}
	}


	Node* SoundGraphNodeEditorModel::SpawnGraphInputNode(const std::string& inputName)
	{
		Node* newNode = SG::SoundGraphNodeFactory::SpawnGraphPropertyNode(m_GraphAsset, inputName, EPropertyType::Input);

		if (!newNode)
			return nullptr;

		OnNodeSpawned(newNode);
		OnNodeCreated();
		return GetNodes().back();
	}


	Node* SoundGraphNodeEditorModel::SpawnLocalVariableNode(const std::string& variableName, bool getter)
	{
		// We don't allow to create multiple setter nodes for graph Local Variables
		if (!getter)
		{
			for (const auto& node : m_GraphAsset->Nodes)
			{
				if (node->Category == GetPropertyToken(EPropertyType::LocalVariable)
					&& node->Name == variableName
					&& node->Inputs.size() == 1
					&& node->Inputs[0]->Name == variableName)
				{
					HZ_CONSOLE_LOG_TRACE("Setter node for the variable '{}' already exists!", variableName);
					ax::NodeEditor::SelectNode((uint64_t)node->ID, false);
					ax::NodeEditor::NavigateToSelection();
					return nullptr;
				}
			}
		}

		Node* newNode = SG::SoundGraphNodeFactory::SpawnGraphPropertyNode(m_GraphAsset, variableName, EPropertyType::LocalVariable, getter);

		if (!newNode)
			return nullptr;

		OnNodeSpawned(newNode);
		OnNodeCreated();

		return GetNodes().back();
	}


	uint64_t SoundGraphNodeEditorModel::GetCurrentPlaybackFrame() const
	{
		if (!m_SoundGraphSource || m_SoundGraphSource->IsSuspended())
			return 0;

		return m_SoundGraphSource->GetCurrentFrame();
	}

	void SoundGraphNodeEditorModel::OnUpdate(Timestep ts)
	{
		//! Care should be taken as this may be called not from Audio Thread
		if (m_SoundGraphSource)
			m_SoundGraphSource->Update(ts);
	}

} // namespace Hazel

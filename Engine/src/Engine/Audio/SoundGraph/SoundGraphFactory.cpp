#include "pch.h"
#include "SoundGraphFactory.h"

#include "Nodes/NodeTypes.h"
#include "Nodes/NodeTypeImpls.h"
#include "Nodes/NodeDescriptors.h"
#include "Engine/Utilities/StringUtils.h"


// Using the same string transformation here as in the editor nodes factory
// to translate NodeProcessor type to friendly name ensures that they are going to match.
// This allows us to minimizes name coupling.
#define NODE(name) { Identifier(Engine::Utils::CreateUserFriendlyTypeName(#name)), [](UUID nodeID) { return new name(#name, nodeID); } }
#define NODE_ALIAS(aliasName, processor) { Identifier(aliasName), [](UUID nodeID) { return new processor(aliasName, nodeID); } }

 //? slight connascence of type with aliases, we add the same pair to the editor registry in SoundGraphNodes.cpp
namespace Alias = Engine::SoundGraph::NameAliases;

namespace Engine::SoundGraph
{
	//==============================================================================
	using Registry = std::unordered_map<Identifier, std::function<NodeProcessor*(UUID nodeID)>>;

	// Translation from editor graph node name to NodeProcessory type
	// NodeProcessor type -> user friendly name for the editor -> NodeProcessor type
	static const Registry NodeProcessors
	{
		// Base nodes
		NODE(WavePlayer),

		// Math nodes
		NODE_ALIAS(Alias::AddAudio, Add<float>),
		NODE_ALIAS(Alias::AddFloatAudio, Add<float>),
		NODE(Add<float>),
		NODE(Add<int>),
		NODE(Divide<float>),
		NODE(Divide<int>),
		NODE(Log),
		NODE(LinearToLogFrequency),
		NODE(FrequencyLogToLinear),
		
		NODE(MapRange<float>),
		NODE(MapRange<int>),
		NODE_ALIAS(Alias::MapRangeAudio, MapRange<float>),
		NODE(Min<float>),
		NODE(Min<int>),
		NODE_ALIAS(Alias::MinAudio, Min<float>),
		NODE(Max<float>),
		NODE(Max<int>),
		NODE_ALIAS(Alias::MaxAudio, Max<float>),
		NODE(Clamp<float>),
		NODE(Clamp<int>),
		NODE_ALIAS(Alias::ClampAudio, Clamp<float>),

		NODE(Modulo),
		NODE_ALIAS(Alias::MultAudioFloat, Multiply<float>),
		NODE_ALIAS(Alias::MultAudio, Multiply<float>),
		NODE(Multiply<float>),
		NODE(Multiply<int>),
		NODE(Power),
		NODE_ALIAS(Alias::SubtractAudio, Subtract<float>),
		NODE(Subtract<float>),
		NODE(Subtract<int>),

		// Array nodes
		NODE(Get<float>),
		NODE(Get<int>),
		NODE_ALIAS(Alias::GetWave, Get<int64_t>),
		NODE(GetRandom<float>),
		NODE(GetRandom<int>),
		NODE_ALIAS(Alias::GetRandomWave, GetRandom<int64_t>),

		// Generator nodes
		NODE(Noise),
		NODE(Sine),
		NODE(Random<float>),
		NODE(Random<int>),
		NODE_ALIAS(Alias::ADEnvelope, ADEnvelope),

		// Triggers
		NODE(RepeatTrigger),
		NODE(TriggerCounter),
		NODE(DelayedTrigger),

		// Music nodes
		NODE_ALIAS(Alias::BPMToSeconds, BPMToSeconds),
		NODE(NoteToFrequency<float>),
		NODE(NoteToFrequency<int>),
		NODE(FrequencyToNote),
	};

	//==============================================================================
	NodeProcessor* Factory::Create(Identifier nodeTypeID, UUID nodeID)
	{
		if (!NodeProcessors.count(nodeTypeID))
		{
			// TODO: other node types
			if (!nodeTypeID.GetDBGName().empty())
				ZONG_CORE_ERROR("SoundGraph::Factory::Create - Node with type ID {} is not in the registry", nodeTypeID.GetDBGName());
			else
				ZONG_CORE_ERROR("SoundGraph::Factory::Create - Node with type ID {} is not in the registry", (uint32_t)nodeTypeID);
				
			return nullptr;
		}

		return NodeProcessors.at(nodeTypeID)(nodeID);
	}

	bool Factory::Contains(Identifier nodeTypeID)
	{
		return NodeProcessors.count(nodeTypeID);
	}

} // namespace Engine::SoundGraph

#pragma once

#include "Engine/Audio/SoundGraph/NodeProcessor.h"

#include "NodeTypes.h"
#include "Engine/Reflection/TypeDescriptor.h"

// Sometimes we need to reuse NodeProcessor types for different nodes,
// so that they have different names and other editor UI properties
// but the same NodeProcessor type in the backend.
// In such case we declare alias ID here and define it in the SoundGraphNodes.cpp,
// adding and extra entry to the registry of the same NodeProcessor type,
// but with a differen name.
// Actual NodeProcessor type for the alias is assigned in SoundgGraphFactory.cpp
//! Name aliases must already be "User Friendly Type Name" in format: "Get Random (Float)" instead of "GetRandom<float>"
namespace Engine::SoundGraph::NameAliases
{
	static constexpr auto AddAudio			= "Add (Audio)"
						, AddFloatAudio		= "Add (Float to Audio)"
						, MultAudioFloat	= "Multiply (Audio by Float)"
						, MultAudio			= "Multiply (Audio)"
						, SubtractAudio		= "Subtract (Audio)"
						, MinAudio			= "Min (Audio)"
						, MaxAudio			= "Max (Audio)"
						, ClampAudio		= "Clamp (Audio)"
						, MapRangeAudio		= "Map Range (Audio)"
						, GetWave			= "Get (Wave)"
						, GetRandomWave		= "Get Random (Wave)"
						, ADEnvelope		= "AD Envelope"
						, BPMToSeconds		= "BPM to Seconds";

#undef NAME_ALIAS
}


// TODO: somehow add value overrides here?
//		Alternativelly I could create a function that takes NodeProcessor type and returns only named overrides and verifies member names

namespace Engine::SoundGraph {
	struct TagInputs {};
	struct TagOutputs {};
	template<typename T> struct NodeDescription;

	template<typename T>
	using DescribedNode = Type::is_specialized<NodeDescription<std::remove_cvref_t<T>>>;
}

//! Example
#if 0
	DESCRIBED_TAGGED(Engine::SoundGraph::Add<float>, Engine::Nodes::TagInputs,
		&Engine::SoundGraph::Add<float>::in_Value1,
		&Engine::SoundGraph::Add<float>::in_Value2);

	DESCRIBED_TAGGED(Engine::SoundGraph::Add<float>, Engine::Nodes::TagOutputs,
		&Engine::SoundGraph::Add<float>::out_Out);

	template<> struct Engine::Nodes::NodeDescription<Engine::SoundGraph::Add<float>>
	{
		using Inputs = Engine::Type::Description<Engine::SoundGraph::Add<float>, Engine::Nodes::TagInputs>;
		using Outputs = Engine::Type::Description<Engine::SoundGraph::Add<float>, Engine::Nodes::TagOutputs>;
	};
#endif

#ifndef NODE_INPUTS
#define NODE_INPUTS(...) __VA_ARGS__
#endif // !NODE_INPUTS

#ifndef NODE_OUTPUTS
#define NODE_OUTPUTS(...) __VA_ARGS__
#endif // !NODE_OUTPUTS

// TODO: type and name overrides
// TODO: node with no inputs / outputs
#ifndef DESCRIBE_NODE
	#define DESCRIBE_NODE(NodeType, InputList, OutputList)									\
	DESCRIBED_TAGGED(NodeType, Engine::SoundGraph::TagInputs, InputList)						\
	DESCRIBED_TAGGED(NodeType, Engine::SoundGraph::TagOutputs, OutputList)					\
																							\
	template<> struct Engine::SoundGraph::NodeDescription<NodeType>							\
	{																						\
		using Inputs = Engine::Type::Description<NodeType, Engine::SoundGraph::TagInputs>;	\
		using Outputs = Engine::Type::Description<NodeType, Engine::SoundGraph::TagOutputs>;	\
	};		
#endif // !DESCRIBE_NODE

DESCRIBE_NODE(Engine::SoundGraph::Add<float>,
	NODE_INPUTS(
		&Engine::SoundGraph::Add<float>::in_Value1,
		&Engine::SoundGraph::Add<float>::in_Value2),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Add<float>::out_Out)
);

DESCRIBE_NODE(Engine::SoundGraph::Add<int>,
	NODE_INPUTS(
		&Engine::SoundGraph::Add<int>::in_Value1,
		&Engine::SoundGraph::Add<int>::in_Value2),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Add<int>::out_Out)
);

DESCRIBE_NODE(Engine::SoundGraph::Subtract<float>,
	NODE_INPUTS(
		&Engine::SoundGraph::Subtract<float>::in_Value1,
		&Engine::SoundGraph::Subtract<float>::in_Value2),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Subtract<float>::out_Out)
);

DESCRIBE_NODE(Engine::SoundGraph::Subtract<int>,
	NODE_INPUTS(
		&Engine::SoundGraph::Subtract<int>::in_Value1,
		&Engine::SoundGraph::Subtract<int>::in_Value2),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Subtract<int>::out_Out)
);

DESCRIBE_NODE(Engine::SoundGraph::Multiply<float>,
	NODE_INPUTS(
		&Engine::SoundGraph::Multiply<float>::in_Value,
		&Engine::SoundGraph::Multiply<float>::in_Multiplier),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Multiply<float>::out_Out)
);

DESCRIBE_NODE(Engine::SoundGraph::Multiply<int>,
	NODE_INPUTS(
		&Engine::SoundGraph::Multiply<int>::in_Value,
		&Engine::SoundGraph::Multiply<int>::in_Multiplier),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Multiply<int>::out_Out)
);

DESCRIBE_NODE(Engine::SoundGraph::Divide<float>,
	NODE_INPUTS(
		&Engine::SoundGraph::Divide<float>::in_Value,
		&Engine::SoundGraph::Divide<float>::in_Denominator),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Divide<float>::out_Out)
);

DESCRIBE_NODE(Engine::SoundGraph::Divide<int>,
	NODE_INPUTS(
		&Engine::SoundGraph::Divide<int>::in_Value,
		&Engine::SoundGraph::Divide<int>::in_Denominator),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Divide<int>::out_Out)
);

DESCRIBE_NODE(Engine::SoundGraph::Power,
	NODE_INPUTS(
		&Engine::SoundGraph::Power::in_Base,
		&Engine::SoundGraph::Power::in_Exponent),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Power::out_Out)
);

DESCRIBE_NODE(Engine::SoundGraph::Log,
	NODE_INPUTS(
		&Engine::SoundGraph::Log::in_Base,
		&Engine::SoundGraph::Log::in_Value),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Log::out_Out)
);

DESCRIBE_NODE(Engine::SoundGraph::LinearToLogFrequency,
	NODE_INPUTS(
		&Engine::SoundGraph::LinearToLogFrequency::in_Value,
		&Engine::SoundGraph::LinearToLogFrequency::in_Min,
		&Engine::SoundGraph::LinearToLogFrequency::in_Max,
		&Engine::SoundGraph::LinearToLogFrequency::in_MinFrequency,
		&Engine::SoundGraph::LinearToLogFrequency::in_MaxFrequency),
	NODE_OUTPUTS(
		&Engine::SoundGraph::LinearToLogFrequency::out_Frequency)
);

DESCRIBE_NODE(Engine::SoundGraph::FrequencyLogToLinear,
	NODE_INPUTS(
		&Engine::SoundGraph::FrequencyLogToLinear::in_Frequency,
		&Engine::SoundGraph::FrequencyLogToLinear::in_MinFrequency,
		&Engine::SoundGraph::FrequencyLogToLinear::in_MaxFrequency,
		&Engine::SoundGraph::FrequencyLogToLinear::in_Min,
		&Engine::SoundGraph::FrequencyLogToLinear::in_Max),
	NODE_OUTPUTS(
		&Engine::SoundGraph::FrequencyLogToLinear::out_Value)
);

DESCRIBE_NODE(Engine::SoundGraph::Modulo,
	NODE_INPUTS(
		&Engine::SoundGraph::Modulo::in_Value,
		&Engine::SoundGraph::Modulo::in_Modulo),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Modulo::out_Out)
);

DESCRIBE_NODE(Engine::SoundGraph::Min<float>,
	NODE_INPUTS(
		&Engine::SoundGraph::Min<float>::in_A,
		&Engine::SoundGraph::Min<float>::in_B),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Min<float>::out_Value)
);

DESCRIBE_NODE(Engine::SoundGraph::Min<int>,
	NODE_INPUTS(
		&Engine::SoundGraph::Min<int>::in_A,
		&Engine::SoundGraph::Min<int>::in_B),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Min<int>::out_Value)
);

DESCRIBE_NODE(Engine::SoundGraph::Max<float>,
	NODE_INPUTS(
		&Engine::SoundGraph::Max<float>::in_A,
		&Engine::SoundGraph::Max<float>::in_B),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Max<float>::out_Value)
);

DESCRIBE_NODE(Engine::SoundGraph::Max<int>,
	NODE_INPUTS(
		&Engine::SoundGraph::Max<int>::in_A,
		&Engine::SoundGraph::Max<int>::in_B),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Max<int>::out_Value)
);

DESCRIBE_NODE(Engine::SoundGraph::Clamp<float>,
	NODE_INPUTS(
		&Engine::SoundGraph::Clamp<float>::in_In,
		&Engine::SoundGraph::Clamp<float>::in_Min,
		&Engine::SoundGraph::Clamp<float>::in_Max),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Clamp<float>::out_Value)
);

DESCRIBE_NODE(Engine::SoundGraph::Clamp<int>,
	NODE_INPUTS(
		&Engine::SoundGraph::Clamp<int>::in_In,
		&Engine::SoundGraph::Clamp<int>::in_Min,
		&Engine::SoundGraph::Clamp<int>::in_Max),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Clamp<int>::out_Value)
);

DESCRIBE_NODE(Engine::SoundGraph::MapRange<float>,
	NODE_INPUTS(
		&Engine::SoundGraph::MapRange<float>::in_In,
		&Engine::SoundGraph::MapRange<float>::in_InRangeA,
		&Engine::SoundGraph::MapRange<float>::in_InRangeB,
		&Engine::SoundGraph::MapRange<float>::in_OutRangeA,
		&Engine::SoundGraph::MapRange<float>::in_OutRangeB,
		&Engine::SoundGraph::MapRange<float>::in_Clamped),
	NODE_OUTPUTS(
		&Engine::SoundGraph::MapRange<float>::out_Value)
);

DESCRIBE_NODE(Engine::SoundGraph::MapRange<int>,
	NODE_INPUTS(
		&Engine::SoundGraph::MapRange<int>::in_In,
		&Engine::SoundGraph::MapRange<int>::in_InRangeA,
		&Engine::SoundGraph::MapRange<int>::in_InRangeB,
		&Engine::SoundGraph::MapRange<int>::in_OutRangeA,
		&Engine::SoundGraph::MapRange<int>::in_OutRangeB,
		&Engine::SoundGraph::MapRange<int>::in_Clamped),
	NODE_OUTPUTS(
		&Engine::SoundGraph::MapRange<int>::out_Value)
);

DESCRIBE_NODE(Engine::SoundGraph::WavePlayer,
	NODE_INPUTS(
		&Engine::SoundGraph::WavePlayer::Play,
		&Engine::SoundGraph::WavePlayer::Stop,
		&Engine::SoundGraph::WavePlayer::in_WaveAsset,
		&Engine::SoundGraph::WavePlayer::in_StartTime,
		&Engine::SoundGraph::WavePlayer::in_Loop,
		&Engine::SoundGraph::WavePlayer::in_NumberOfLoops),
	NODE_OUTPUTS(
		&Engine::SoundGraph::WavePlayer::out_OnPlay,
		&Engine::SoundGraph::WavePlayer::out_OnFinish,
		&Engine::SoundGraph::WavePlayer::out_OnLooped,
		&Engine::SoundGraph::WavePlayer::out_PlaybackPosition,
		&Engine::SoundGraph::WavePlayer::out_OutLeft,
		&Engine::SoundGraph::WavePlayer::out_OutRight)
);

DESCRIBE_NODE(Engine::SoundGraph::Get<float>,
	NODE_INPUTS(
		&Engine::SoundGraph::Get<float>::Trigger,
		&Engine::SoundGraph::Get<float>::in_Array,
		&Engine::SoundGraph::Get<float>::in_Index),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Get<float>::out_OnTrigger,
		&Engine::SoundGraph::Get<float>::out_Element)
);

DESCRIBE_NODE(Engine::SoundGraph::Get<int>,
	NODE_INPUTS(
		&Engine::SoundGraph::Get<int>::Trigger,
		&Engine::SoundGraph::Get<int>::in_Array,
		&Engine::SoundGraph::Get<int>::in_Index),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Get<int>::out_OnTrigger,
		&Engine::SoundGraph::Get<int>::out_Element)
);

DESCRIBE_NODE(Engine::SoundGraph::Get<int64_t>,
	NODE_INPUTS(
		&Engine::SoundGraph::Get<int64_t>::Trigger,
		&Engine::SoundGraph::Get<int64_t>::in_Array,
		&Engine::SoundGraph::Get<int64_t>::in_Index),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Get<int64_t>::out_OnTrigger,
		&Engine::SoundGraph::Get<int64_t>::out_Element)
);

DESCRIBE_NODE(Engine::SoundGraph::GetRandom<float>,
	NODE_INPUTS(
		&Engine::SoundGraph::GetRandom<float>::Next,
		&Engine::SoundGraph::GetRandom<float>::Reset,
		&Engine::SoundGraph::GetRandom<float>::in_Array,
		&Engine::SoundGraph::GetRandom<float>::in_Seed),
	NODE_OUTPUTS(
		&Engine::SoundGraph::GetRandom<float>::out_OnNext,
		&Engine::SoundGraph::GetRandom<float>::out_OnReset,
		&Engine::SoundGraph::GetRandom<float>::out_Element)
);

DESCRIBE_NODE(Engine::SoundGraph::GetRandom<int>,
	NODE_INPUTS(
		&Engine::SoundGraph::GetRandom<int>::Next,
		&Engine::SoundGraph::GetRandom<int>::Reset,
		&Engine::SoundGraph::GetRandom<int>::in_Array,
		&Engine::SoundGraph::GetRandom<int>::in_Seed),
	NODE_OUTPUTS(
		&Engine::SoundGraph::GetRandom<int>::out_OnNext,
		&Engine::SoundGraph::GetRandom<int>::out_OnReset,
		&Engine::SoundGraph::GetRandom<int>::out_Element)
);

DESCRIBE_NODE(Engine::SoundGraph::GetRandom<int64_t>,
	NODE_INPUTS(
		&Engine::SoundGraph::GetRandom<int64_t>::Next,
		&Engine::SoundGraph::GetRandom<int64_t>::Reset,
		&Engine::SoundGraph::GetRandom<int64_t>::in_Array,
		&Engine::SoundGraph::GetRandom<int64_t>::in_Seed),
	NODE_OUTPUTS(
		&Engine::SoundGraph::GetRandom<int64_t>::out_OnNext,
		&Engine::SoundGraph::GetRandom<int64_t>::out_OnReset,
		&Engine::SoundGraph::GetRandom<int64_t>::out_Element)
);

DESCRIBE_NODE(Engine::SoundGraph::Random<int>,
	NODE_INPUTS(
		&Engine::SoundGraph::Random<int>::Next,
		&Engine::SoundGraph::Random<int>::Reset,
		&Engine::SoundGraph::Random<int>::in_Min,
		&Engine::SoundGraph::Random<int>::in_Max,
		&Engine::SoundGraph::Random<int>::in_Seed),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Random<int>::out_OnNext,
		&Engine::SoundGraph::Random<int>::out_OnReset,
		&Engine::SoundGraph::Random<int>::out_Value)
);

DESCRIBE_NODE(Engine::SoundGraph::Random<float>,
	NODE_INPUTS(
		&Engine::SoundGraph::Random<float>::Next,
		&Engine::SoundGraph::Random<float>::Reset,
		&Engine::SoundGraph::Random<float>::in_Min,
		&Engine::SoundGraph::Random<float>::in_Max,
		&Engine::SoundGraph::Random<float>::in_Seed),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Random<float>::out_OnNext,
		&Engine::SoundGraph::Random<float>::out_OnReset,
		&Engine::SoundGraph::Random<float>::out_Value)
);

DESCRIBE_NODE(Engine::SoundGraph::Noise,
	NODE_INPUTS(
		&Engine::SoundGraph::Noise::in_Seed,
		&Engine::SoundGraph::Noise::in_Type),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Noise::out_Value)
);

DESCRIBE_NODE(Engine::SoundGraph::Sine,
	NODE_INPUTS(
		&Engine::SoundGraph::Sine::ResetPhase,
		&Engine::SoundGraph::Sine::in_Frequency,
		&Engine::SoundGraph::Sine::in_PhaseOffset),
	NODE_OUTPUTS(
		&Engine::SoundGraph::Sine::out_Sine)
);

DESCRIBE_NODE(Engine::SoundGraph::ADEnvelope,
	NODE_INPUTS(
		&Engine::SoundGraph::ADEnvelope::Trigger,
		&Engine::SoundGraph::ADEnvelope::in_AttackTime,
		&Engine::SoundGraph::ADEnvelope::in_DecayTime,
		&Engine::SoundGraph::ADEnvelope::in_AttackCurve,
		&Engine::SoundGraph::ADEnvelope::in_DecayCurve,
		&Engine::SoundGraph::ADEnvelope::in_Looping),
	NODE_OUTPUTS(
		&Engine::SoundGraph::ADEnvelope::out_OnTrigger,
		&Engine::SoundGraph::ADEnvelope::out_OnComplete,
		&Engine::SoundGraph::ADEnvelope::out_OutEnvelope)
);


DESCRIBE_NODE(Engine::SoundGraph::RepeatTrigger,
	NODE_INPUTS(
		&Engine::SoundGraph::RepeatTrigger::Start,
		&Engine::SoundGraph::RepeatTrigger::Stop,
		&Engine::SoundGraph::RepeatTrigger::in_Period),
	NODE_OUTPUTS(
		&Engine::SoundGraph::RepeatTrigger::out_Trigger)
);

DESCRIBE_NODE(Engine::SoundGraph::TriggerCounter,
	NODE_INPUTS(
		&Engine::SoundGraph::TriggerCounter::Trigger,
		&Engine::SoundGraph::TriggerCounter::Reset,
		&Engine::SoundGraph::TriggerCounter::in_StartValue,
		&Engine::SoundGraph::TriggerCounter::in_StepSize,
		&Engine::SoundGraph::TriggerCounter::in_ResetCount),
	NODE_OUTPUTS(
		&Engine::SoundGraph::TriggerCounter::out_OnTrigger,
		&Engine::SoundGraph::TriggerCounter::out_OnReset,
		&Engine::SoundGraph::TriggerCounter::out_Count,
		&Engine::SoundGraph::TriggerCounter::out_Value)
);

DESCRIBE_NODE(Engine::SoundGraph::DelayedTrigger,
	NODE_INPUTS(
		&Engine::SoundGraph::DelayedTrigger::Trigger,
		&Engine::SoundGraph::DelayedTrigger::Reset,
		&Engine::SoundGraph::DelayedTrigger::in_DelayTime),
	NODE_OUTPUTS(
		&Engine::SoundGraph::DelayedTrigger::out_DelayedTrigger,
		&Engine::SoundGraph::DelayedTrigger::out_OnReset)
);


DESCRIBE_NODE(Engine::SoundGraph::BPMToSeconds,
	NODE_INPUTS(
		&Engine::SoundGraph::BPMToSeconds::in_BPM),
	NODE_OUTPUTS(
		&Engine::SoundGraph::BPMToSeconds::out_Seconds)
);

DESCRIBE_NODE(Engine::SoundGraph::NoteToFrequency<float>,
	NODE_INPUTS(
		&Engine::SoundGraph::NoteToFrequency<float>::in_MIDINote),
	NODE_OUTPUTS(
		&Engine::SoundGraph::NoteToFrequency<float>::out_Frequency)
);

DESCRIBE_NODE(Engine::SoundGraph::NoteToFrequency<int>,
	NODE_INPUTS(
		&Engine::SoundGraph::NoteToFrequency<int>::in_MIDINote),
	NODE_OUTPUTS(
		&Engine::SoundGraph::NoteToFrequency<int>::out_Frequency)
);

DESCRIBE_NODE(Engine::SoundGraph::FrequencyToNote,
	NODE_INPUTS(
		&Engine::SoundGraph::FrequencyToNote::in_Frequency),
	NODE_OUTPUTS(
		&Engine::SoundGraph::FrequencyToNote::out_MIDINote)
);

#include "choc/text/choc_StringUtilities.h"
#include <optional>

//=============================================================================
/**
	Utilities to procedurally registerand initialize node processor endpoints.
*/
namespace Engine::SoundGraph::EndpointUtilities
{
	namespace Impl
	{
		// TODO: remove  prefix from the members, maybe keep in_ / out_
		constexpr std::string_view RemovePrefixAndSuffix(std::string_view name)
		{
			if (Engine::Utils::StartsWith(name, "in_"))
				name.remove_prefix(sizeof("in_") - 1);
			else if (Engine::Utils::StartsWith(name, "out_"))
				name.remove_prefix(sizeof("out_") - 1);
			
			return name;
		}

		//=============================================================================
		/// Implementation of the RegisterEndpoints function. Parsing type data into
		/// node processor enpoints.
		template<typename T>
		static bool RegisterEndpointInputsImpl(NodeProcessor* node, T& v, std::string_view memberName)
		{
			using TMember = T;
			constexpr bool isInputEvent = std::is_member_function_pointer_v<T>;
			//constexpr bool isOutputEvent = std::is_same_v<TMember, Engine::SoundGraph::NodeProcessor::OutputEvent>;

			if constexpr (isInputEvent)
			{
			}
			/*else if constexpr (isOutputEvent)
			{
				node->AddOutEvent(Identifier(RemovePrefixAndSuffix(memberName)), v);
			}*/
			else
			{
				//const bool isArray = std::is_array_v<TMember>;
				//using TMemberDecay = std::conditional_t<isArray, std::remove_pointer_t<std::decay_t<TMember>>, std::decay_t<TMember>>;

				//const bool isArray = Type::is_array_v<TMember> || std::is_array_v<TMember> || Engine::Utils::StartsWith(pinName, "in_Array") || Engine::Utils::StartsWith(pinName, "out_Array");
				//constexpr bool isInput = /*isArray ? Engine::Utils::StartsWith(pinName, "in_") : */std::is_pointer_v<TMember>;

				//if constexpr (isInput)
				{
					node->AddInStream(Identifier(RemovePrefixAndSuffix(memberName)));
				}
			}

			return true;
		}

		template<typename T>
		static bool RegisterEndpointOutputsImpl(NodeProcessor* node, T& v, std::string_view memberName)
		{
			using TMember = T;
			constexpr bool isOutputEvent = std::is_same_v<TMember, NodeProcessor::OutputEvent>;

			if constexpr (isOutputEvent)
			{
				node->AddOutEvent(Identifier(RemovePrefixAndSuffix(memberName)), v);
			}
			else
			{
				node->AddOutStream<TMember>(Identifier(RemovePrefixAndSuffix(memberName)), v);
			}

			return true;
		}

		template<typename T>
		static bool InitializeInputsImpl(NodeProcessor* node, T& v, std::string_view memberName)
		{
			using TMember = T;
			constexpr bool isInputEvent = std::is_member_function_pointer_v<T>;
			constexpr bool isOutputEvent = std::is_same_v<TMember, NodeProcessor::OutputEvent>;

			//? DBG
			//std::string_view str = typeid(TMember).name();

			if constexpr (isInputEvent || isOutputEvent)
			{
			}
			else
			{
				//const bool isArray = std::is_array_v<TMember>;
				//using TMemberDecay = std::conditional_t<isArray, std::remove_pointer_t<std::decay_t<TMember>>, std::decay_t<TMember>>;

				//const bool isArray = Type::is_array_v<TMember> || std::is_array_v<TMember> || Engine::Utils::StartsWith(pinName, "in_Array") || Engine::Utils::StartsWith(pinName, "out_Array");
				constexpr bool isInput = /*isArray ? Engine::Utils::StartsWith(pinName, "in_") : */std::is_pointer_v<TMember>;

				if constexpr (isInput)
					v = (TMember)node->InValue(Identifier(RemovePrefixAndSuffix(memberName))).getRawData();
			}

			return true;
		}

		//=============================================================================
		template<typename TNodeType>
		static bool RegisterEndpoints(TNodeType* node)
		{
			static_assert(DescribedNode<TNodeType>::value);
			using InputsDescription = typename NodeDescription<TNodeType>::Inputs;
			using OutputsDescription = typename NodeDescription<TNodeType>::Outputs;

			const bool insResult = InputsDescription::ApplyToStaticType(
				[&node](const auto&... members)
				{
					auto unpack = [&node, memberIndex = 0](auto memberPtr) mutable
					{
						using TMember = std::remove_reference_t<decltype(memberPtr)>;
						constexpr bool isInputEvent = std::is_member_function_pointer_v<TMember>;
						const std::string_view name = InputsDescription::MemberNames[memberIndex++];

						if constexpr (isInputEvent)
						{
							// TODO: hook up fFlags (?)
							return true;
						}
						else // output events also go here because they are wrapped into a callable object
						{
							return RegisterEndpointInputsImpl(node, node->*memberPtr, name);
						}

						return true;
					};

					return (unpack(members) && ...);
				});

			const bool outsResult = OutputsDescription::ApplyToStaticType(
				[&node](const auto&... members)
				{
					auto unpack = [&node, memberIndex = 0](auto memberPtr) mutable
					{
						using TMember = std::remove_reference_t<decltype(memberPtr)>;
						constexpr bool isInputEvent = std::is_member_function_pointer_v<TMember>;
						const std::string_view name = OutputsDescription::MemberNames[memberIndex++];

						if constexpr (isInputEvent)
						{
							return true;
						}
						else // output events also go here because they are wrapped into a callable object
						{
							return RegisterEndpointOutputsImpl(node, node->*memberPtr, name);
						}

						return true;
					};

					return (unpack(members) && ...);
				});

			return insResult && outsResult;
		}

		//=============================================================================
		template<typename TNodeType>
		static bool InitializeInputs(TNodeType* node)
		{
			static_assert(DescribedNode<TNodeType>::value);
			using InputsDescription = typename NodeDescription<TNodeType>::Inputs;

			return InputsDescription::ApplyToStaticType(
				[&node](const auto&... members)
				{
					auto unpack = [&node, memberIndex = 0](auto memberPtr) mutable
					{
						using TMember = decltype(memberPtr);
						constexpr bool isInputEvent = std::is_member_function_pointer_v<TMember>;
						const std::string_view name = InputsDescription::MemberNames[memberIndex++];
						
						//? DBG
						//std::string_view str = typeid(TMember).name();

						if constexpr (isInputEvent)
							return true;
						else
							return InitializeInputsImpl(node, node->*memberPtr, name);
					};
					return (unpack(members) && ...);
				});
		}

	} // namespace Impl

	template<typename TNodeType>
	static bool RegisterEndpoints(TNodeType* node)
	{
		return Impl::RegisterEndpoints(node);
	}

	template<typename TNodeType>
	static bool InitializeInputs(TNodeType* node)
	{
		return Impl::InitializeInputs(node);
	}

} // Engine::SoundGraph::EndpointUtilities

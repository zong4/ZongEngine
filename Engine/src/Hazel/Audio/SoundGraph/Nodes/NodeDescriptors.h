#pragma once

#include "Hazel/Audio/SoundGraph/NodeProcessor.h"

#include "NodeTypes.h"
#include "Hazel/Reflection/TypeDescriptor.h"

// Sometimes we need to reuse NodeProcessor types for different nodes,
// so that they have different names and other editor UI properties
// but the same NodeProcessor type in the backend.
// In such case we declare alias ID here and define it in the SoundGraphNodes.cpp,
// adding and extra entry to the registry of the same NodeProcessor type,
// but with a differen name.
// Actual NodeProcessor type for the alias is assigned in SoundgGraphFactory.cpp
//! Name aliases must already be "User Friendly Type Name" in format: "Get Random (Float)" instead of "GetRandom<float>"
namespace Hazel::SoundGraph::NameAliases
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

namespace Hazel::SoundGraph {
	struct TagInputs {};
	struct TagOutputs {};
	template<typename T> struct NodeDescription;

	template<typename T>
	using DescribedNode = Type::is_specialized<NodeDescription<std::remove_cvref_t<T>>>;
}

//! Example
#if 0
	DESCRIBED_TAGGED(Hazel::SoundGraph::Add<float>, Hazel::Nodes::TagInputs,
		&Hazel::SoundGraph::Add<float>::in_Value1,
		&Hazel::SoundGraph::Add<float>::in_Value2);

	DESCRIBED_TAGGED(Hazel::SoundGraph::Add<float>, Hazel::Nodes::TagOutputs,
		&Hazel::SoundGraph::Add<float>::out_Out);

	template<> struct Hazel::Nodes::NodeDescription<Hazel::SoundGraph::Add<float>>
	{
		using Inputs = Hazel::Type::Description<Hazel::SoundGraph::Add<float>, Hazel::Nodes::TagInputs>;
		using Outputs = Hazel::Type::Description<Hazel::SoundGraph::Add<float>, Hazel::Nodes::TagOutputs>;
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
	DESCRIBED_TAGGED(NodeType, Hazel::SoundGraph::TagInputs, InputList)						\
	DESCRIBED_TAGGED(NodeType, Hazel::SoundGraph::TagOutputs, OutputList)					\
																							\
	template<> struct Hazel::SoundGraph::NodeDescription<NodeType>							\
	{																						\
		using Inputs = Hazel::Type::Description<NodeType, Hazel::SoundGraph::TagInputs>;	\
		using Outputs = Hazel::Type::Description<NodeType, Hazel::SoundGraph::TagOutputs>;	\
	};		
#endif // !DESCRIBE_NODE

DESCRIBE_NODE(Hazel::SoundGraph::Add<float>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Add<float>::in_Value1,
		&Hazel::SoundGraph::Add<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Add<float>::out_Out)
);

DESCRIBE_NODE(Hazel::SoundGraph::Add<int>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Add<int>::in_Value1,
		&Hazel::SoundGraph::Add<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Add<int>::out_Out)
);

DESCRIBE_NODE(Hazel::SoundGraph::Subtract<float>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Subtract<float>::in_Value1,
		&Hazel::SoundGraph::Subtract<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Subtract<float>::out_Out)
);

DESCRIBE_NODE(Hazel::SoundGraph::Subtract<int>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Subtract<int>::in_Value1,
		&Hazel::SoundGraph::Subtract<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Subtract<int>::out_Out)
);

DESCRIBE_NODE(Hazel::SoundGraph::Multiply<float>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Multiply<float>::in_Value,
		&Hazel::SoundGraph::Multiply<float>::in_Multiplier),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Multiply<float>::out_Out)
);

DESCRIBE_NODE(Hazel::SoundGraph::Multiply<int>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Multiply<int>::in_Value,
		&Hazel::SoundGraph::Multiply<int>::in_Multiplier),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Multiply<int>::out_Out)
);

DESCRIBE_NODE(Hazel::SoundGraph::Divide<float>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Divide<float>::in_Value,
		&Hazel::SoundGraph::Divide<float>::in_Denominator),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Divide<float>::out_Out)
);

DESCRIBE_NODE(Hazel::SoundGraph::Divide<int>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Divide<int>::in_Value,
		&Hazel::SoundGraph::Divide<int>::in_Denominator),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Divide<int>::out_Out)
);

DESCRIBE_NODE(Hazel::SoundGraph::Power,
	NODE_INPUTS(
		&Hazel::SoundGraph::Power::in_Base,
		&Hazel::SoundGraph::Power::in_Exponent),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Power::out_Out)
);

DESCRIBE_NODE(Hazel::SoundGraph::Log,
	NODE_INPUTS(
		&Hazel::SoundGraph::Log::in_Base,
		&Hazel::SoundGraph::Log::in_Value),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Log::out_Out)
);

DESCRIBE_NODE(Hazel::SoundGraph::LinearToLogFrequency,
	NODE_INPUTS(
		&Hazel::SoundGraph::LinearToLogFrequency::in_Value,
		&Hazel::SoundGraph::LinearToLogFrequency::in_Min,
		&Hazel::SoundGraph::LinearToLogFrequency::in_Max,
		&Hazel::SoundGraph::LinearToLogFrequency::in_MinFrequency,
		&Hazel::SoundGraph::LinearToLogFrequency::in_MaxFrequency),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::LinearToLogFrequency::out_Frequency)
);

DESCRIBE_NODE(Hazel::SoundGraph::FrequencyLogToLinear,
	NODE_INPUTS(
		&Hazel::SoundGraph::FrequencyLogToLinear::in_Frequency,
		&Hazel::SoundGraph::FrequencyLogToLinear::in_MinFrequency,
		&Hazel::SoundGraph::FrequencyLogToLinear::in_MaxFrequency,
		&Hazel::SoundGraph::FrequencyLogToLinear::in_Min,
		&Hazel::SoundGraph::FrequencyLogToLinear::in_Max),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::FrequencyLogToLinear::out_Value)
);

DESCRIBE_NODE(Hazel::SoundGraph::Modulo,
	NODE_INPUTS(
		&Hazel::SoundGraph::Modulo::in_Value,
		&Hazel::SoundGraph::Modulo::in_Modulo),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Modulo::out_Out)
);

DESCRIBE_NODE(Hazel::SoundGraph::Min<float>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Min<float>::in_A,
		&Hazel::SoundGraph::Min<float>::in_B),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Min<float>::out_Value)
);

DESCRIBE_NODE(Hazel::SoundGraph::Min<int>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Min<int>::in_A,
		&Hazel::SoundGraph::Min<int>::in_B),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Min<int>::out_Value)
);

DESCRIBE_NODE(Hazel::SoundGraph::Max<float>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Max<float>::in_A,
		&Hazel::SoundGraph::Max<float>::in_B),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Max<float>::out_Value)
);

DESCRIBE_NODE(Hazel::SoundGraph::Max<int>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Max<int>::in_A,
		&Hazel::SoundGraph::Max<int>::in_B),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Max<int>::out_Value)
);

DESCRIBE_NODE(Hazel::SoundGraph::Clamp<float>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Clamp<float>::in_In,
		&Hazel::SoundGraph::Clamp<float>::in_Min,
		&Hazel::SoundGraph::Clamp<float>::in_Max),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Clamp<float>::out_Value)
);

DESCRIBE_NODE(Hazel::SoundGraph::Clamp<int>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Clamp<int>::in_In,
		&Hazel::SoundGraph::Clamp<int>::in_Min,
		&Hazel::SoundGraph::Clamp<int>::in_Max),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Clamp<int>::out_Value)
);

DESCRIBE_NODE(Hazel::SoundGraph::MapRange<float>,
	NODE_INPUTS(
		&Hazel::SoundGraph::MapRange<float>::in_In,
		&Hazel::SoundGraph::MapRange<float>::in_InRangeA,
		&Hazel::SoundGraph::MapRange<float>::in_InRangeB,
		&Hazel::SoundGraph::MapRange<float>::in_OutRangeA,
		&Hazel::SoundGraph::MapRange<float>::in_OutRangeB,
		&Hazel::SoundGraph::MapRange<float>::in_Clamped),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::MapRange<float>::out_Value)
);

DESCRIBE_NODE(Hazel::SoundGraph::MapRange<int>,
	NODE_INPUTS(
		&Hazel::SoundGraph::MapRange<int>::in_In,
		&Hazel::SoundGraph::MapRange<int>::in_InRangeA,
		&Hazel::SoundGraph::MapRange<int>::in_InRangeB,
		&Hazel::SoundGraph::MapRange<int>::in_OutRangeA,
		&Hazel::SoundGraph::MapRange<int>::in_OutRangeB,
		&Hazel::SoundGraph::MapRange<int>::in_Clamped),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::MapRange<int>::out_Value)
);

DESCRIBE_NODE(Hazel::SoundGraph::WavePlayer,
	NODE_INPUTS(
		&Hazel::SoundGraph::WavePlayer::Play,
		&Hazel::SoundGraph::WavePlayer::Stop,
		&Hazel::SoundGraph::WavePlayer::in_WaveAsset,
		&Hazel::SoundGraph::WavePlayer::in_StartTime,
		&Hazel::SoundGraph::WavePlayer::in_Loop,
		&Hazel::SoundGraph::WavePlayer::in_NumberOfLoops),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::WavePlayer::out_OnPlay,
		&Hazel::SoundGraph::WavePlayer::out_OnFinish,
		&Hazel::SoundGraph::WavePlayer::out_OnLooped,
		&Hazel::SoundGraph::WavePlayer::out_PlaybackPosition,
		&Hazel::SoundGraph::WavePlayer::out_OutLeft,
		&Hazel::SoundGraph::WavePlayer::out_OutRight)
);

DESCRIBE_NODE(Hazel::SoundGraph::Get<float>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Get<float>::Trigger,
		&Hazel::SoundGraph::Get<float>::in_Array,
		&Hazel::SoundGraph::Get<float>::in_Index),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Get<float>::out_OnTrigger,
		&Hazel::SoundGraph::Get<float>::out_Element)
);

DESCRIBE_NODE(Hazel::SoundGraph::Get<int>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Get<int>::Trigger,
		&Hazel::SoundGraph::Get<int>::in_Array,
		&Hazel::SoundGraph::Get<int>::in_Index),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Get<int>::out_OnTrigger,
		&Hazel::SoundGraph::Get<int>::out_Element)
);

DESCRIBE_NODE(Hazel::SoundGraph::Get<int64_t>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Get<int64_t>::Trigger,
		&Hazel::SoundGraph::Get<int64_t>::in_Array,
		&Hazel::SoundGraph::Get<int64_t>::in_Index),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Get<int64_t>::out_OnTrigger,
		&Hazel::SoundGraph::Get<int64_t>::out_Element)
);

DESCRIBE_NODE(Hazel::SoundGraph::GetRandom<float>,
	NODE_INPUTS(
		&Hazel::SoundGraph::GetRandom<float>::Next,
		&Hazel::SoundGraph::GetRandom<float>::Reset,
		&Hazel::SoundGraph::GetRandom<float>::in_Array,
		&Hazel::SoundGraph::GetRandom<float>::in_Seed),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::GetRandom<float>::out_OnNext,
		&Hazel::SoundGraph::GetRandom<float>::out_OnReset,
		&Hazel::SoundGraph::GetRandom<float>::out_Element)
);

DESCRIBE_NODE(Hazel::SoundGraph::GetRandom<int>,
	NODE_INPUTS(
		&Hazel::SoundGraph::GetRandom<int>::Next,
		&Hazel::SoundGraph::GetRandom<int>::Reset,
		&Hazel::SoundGraph::GetRandom<int>::in_Array,
		&Hazel::SoundGraph::GetRandom<int>::in_Seed),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::GetRandom<int>::out_OnNext,
		&Hazel::SoundGraph::GetRandom<int>::out_OnReset,
		&Hazel::SoundGraph::GetRandom<int>::out_Element)
);

DESCRIBE_NODE(Hazel::SoundGraph::GetRandom<int64_t>,
	NODE_INPUTS(
		&Hazel::SoundGraph::GetRandom<int64_t>::Next,
		&Hazel::SoundGraph::GetRandom<int64_t>::Reset,
		&Hazel::SoundGraph::GetRandom<int64_t>::in_Array,
		&Hazel::SoundGraph::GetRandom<int64_t>::in_Seed),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::GetRandom<int64_t>::out_OnNext,
		&Hazel::SoundGraph::GetRandom<int64_t>::out_OnReset,
		&Hazel::SoundGraph::GetRandom<int64_t>::out_Element)
);

DESCRIBE_NODE(Hazel::SoundGraph::Random<int>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Random<int>::Next,
		&Hazel::SoundGraph::Random<int>::Reset,
		&Hazel::SoundGraph::Random<int>::in_Min,
		&Hazel::SoundGraph::Random<int>::in_Max,
		&Hazel::SoundGraph::Random<int>::in_Seed),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Random<int>::out_OnNext,
		&Hazel::SoundGraph::Random<int>::out_OnReset,
		&Hazel::SoundGraph::Random<int>::out_Value)
);

DESCRIBE_NODE(Hazel::SoundGraph::Random<float>,
	NODE_INPUTS(
		&Hazel::SoundGraph::Random<float>::Next,
		&Hazel::SoundGraph::Random<float>::Reset,
		&Hazel::SoundGraph::Random<float>::in_Min,
		&Hazel::SoundGraph::Random<float>::in_Max,
		&Hazel::SoundGraph::Random<float>::in_Seed),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Random<float>::out_OnNext,
		&Hazel::SoundGraph::Random<float>::out_OnReset,
		&Hazel::SoundGraph::Random<float>::out_Value)
);

DESCRIBE_NODE(Hazel::SoundGraph::Noise,
	NODE_INPUTS(
		&Hazel::SoundGraph::Noise::in_Seed,
		&Hazel::SoundGraph::Noise::in_Type),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Noise::out_Value)
);

DESCRIBE_NODE(Hazel::SoundGraph::Sine,
	NODE_INPUTS(
		&Hazel::SoundGraph::Sine::ResetPhase,
		&Hazel::SoundGraph::Sine::in_Frequency,
		&Hazel::SoundGraph::Sine::in_PhaseOffset),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::Sine::out_Sine)
);

DESCRIBE_NODE(Hazel::SoundGraph::ADEnvelope,
	NODE_INPUTS(
		&Hazel::SoundGraph::ADEnvelope::Trigger,
		&Hazel::SoundGraph::ADEnvelope::in_AttackTime,
		&Hazel::SoundGraph::ADEnvelope::in_DecayTime,
		&Hazel::SoundGraph::ADEnvelope::in_AttackCurve,
		&Hazel::SoundGraph::ADEnvelope::in_DecayCurve,
		&Hazel::SoundGraph::ADEnvelope::in_Looping),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::ADEnvelope::out_OnTrigger,
		&Hazel::SoundGraph::ADEnvelope::out_OnComplete,
		&Hazel::SoundGraph::ADEnvelope::out_OutEnvelope)
);


DESCRIBE_NODE(Hazel::SoundGraph::RepeatTrigger,
	NODE_INPUTS(
		&Hazel::SoundGraph::RepeatTrigger::Start,
		&Hazel::SoundGraph::RepeatTrigger::Stop,
		&Hazel::SoundGraph::RepeatTrigger::in_Period),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::RepeatTrigger::out_Trigger)
);

DESCRIBE_NODE(Hazel::SoundGraph::TriggerCounter,
	NODE_INPUTS(
		&Hazel::SoundGraph::TriggerCounter::Trigger,
		&Hazel::SoundGraph::TriggerCounter::Reset,
		&Hazel::SoundGraph::TriggerCounter::in_StartValue,
		&Hazel::SoundGraph::TriggerCounter::in_StepSize,
		&Hazel::SoundGraph::TriggerCounter::in_ResetCount),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::TriggerCounter::out_OnTrigger,
		&Hazel::SoundGraph::TriggerCounter::out_OnReset,
		&Hazel::SoundGraph::TriggerCounter::out_Count,
		&Hazel::SoundGraph::TriggerCounter::out_Value)
);

DESCRIBE_NODE(Hazel::SoundGraph::DelayedTrigger,
	NODE_INPUTS(
		&Hazel::SoundGraph::DelayedTrigger::Trigger,
		&Hazel::SoundGraph::DelayedTrigger::Reset,
		&Hazel::SoundGraph::DelayedTrigger::in_DelayTime),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::DelayedTrigger::out_DelayedTrigger,
		&Hazel::SoundGraph::DelayedTrigger::out_OnReset)
);


DESCRIBE_NODE(Hazel::SoundGraph::BPMToSeconds,
	NODE_INPUTS(
		&Hazel::SoundGraph::BPMToSeconds::in_BPM),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::BPMToSeconds::out_Seconds)
);

DESCRIBE_NODE(Hazel::SoundGraph::NoteToFrequency<float>,
	NODE_INPUTS(
		&Hazel::SoundGraph::NoteToFrequency<float>::in_MIDINote),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::NoteToFrequency<float>::out_Frequency)
);

DESCRIBE_NODE(Hazel::SoundGraph::NoteToFrequency<int>,
	NODE_INPUTS(
		&Hazel::SoundGraph::NoteToFrequency<int>::in_MIDINote),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::NoteToFrequency<int>::out_Frequency)
);

DESCRIBE_NODE(Hazel::SoundGraph::FrequencyToNote,
	NODE_INPUTS(
		&Hazel::SoundGraph::FrequencyToNote::in_Frequency),
	NODE_OUTPUTS(
		&Hazel::SoundGraph::FrequencyToNote::out_MIDINote)
);

#include "choc/text/choc_StringUtilities.h"
#include <optional>

//=============================================================================
/**
	Utilities to procedurally registerand initialize node processor endpoints.
*/
namespace Hazel::SoundGraph::EndpointUtilities
{
	namespace Impl
	{
		// TODO: remove  prefix from the members, maybe keep in_ / out_
		constexpr std::string_view RemovePrefixAndSuffix(std::string_view name)
		{
			if (Hazel::Utils::StartsWith(name, "in_"))
				name.remove_prefix(sizeof("in_") - 1);
			else if (Hazel::Utils::StartsWith(name, "out_"))
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
			//constexpr bool isOutputEvent = std::is_same_v<TMember, Hazel::SoundGraph::NodeProcessor::OutputEvent>;

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

				//const bool isArray = Type::is_array_v<TMember> || std::is_array_v<TMember> || Hazel::Utils::StartsWith(pinName, "in_Array") || Hazel::Utils::StartsWith(pinName, "out_Array");
				//constexpr bool isInput = /*isArray ? Hazel::Utils::StartsWith(pinName, "in_") : */std::is_pointer_v<TMember>;

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

				//const bool isArray = Type::is_array_v<TMember> || std::is_array_v<TMember> || Hazel::Utils::StartsWith(pinName, "in_Array") || Hazel::Utils::StartsWith(pinName, "out_Array");
				constexpr bool isInput = /*isArray ? Hazel::Utils::StartsWith(pinName, "in_") : */std::is_pointer_v<TMember>;

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

} // Hazel::SoundGraph::EndpointUtilities

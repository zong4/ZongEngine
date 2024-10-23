#pragma once

#include "AnimationNodes.h"
#include "ArrayNodes.h"
#include "MathNodes.h"
#include "LogicNodes.h"
#include "StateMachineNodes.h"
#include "TriggerNodes.h"

#include "Engine/Reflection/TypeDescriptor.h"

#include <choc/text/choc_StringUtilities.h>
#include <optional>

namespace Hazel::AnimationGraph {

	// Sometimes we need to reuse NodeProcessor types for different nodes,
	// so that they have different names and other editor UI properties
	// but the same NodeProcessor type in the backend.
	// In such case we declare alias ID here and define it in the AnimationGraphNodes.cpp,
	// adding and extra entry to the registry of the same NodeProcessor type,
	// but with a different name.
	// Actual NodeProcessor type for the alias is assigned in AnimationGraphFactory.cpp
	//! Aliases must already be "User Friendly Type Name" in format: "Get Random (Float)" instead of "GetRandom<float>"
	namespace Alias {
		static constexpr auto GetAnimation = "Get (Animation)";
		static constexpr auto GetRandomAnimation = "Get Random (Animation)";
		static constexpr auto Transition = "Transition";
	}

	struct TagInputs {};
	struct TagOutputs {};
	template<typename T> struct NodeDescription;

	template<typename T>
	using DescribedNode = Type::is_specialized<NodeDescription<std::remove_cvref_t<T>>>;


	namespace EndpointUtilities {

		namespace Impl {

			constexpr std::string_view RemovePrefixAndSuffix(std::string_view name)
			{
				if (Hazel::Utils::StartsWith(name, "in_"))
					name.remove_prefix(sizeof("in_") - 1);
				else if (Hazel::Utils::StartsWith(name, "out_"))
					name.remove_prefix(sizeof("out_") - 1);

				return name;
			}


			template<typename T>
			static bool RegisterEndpointInputsImpl(NodeProcessor* node, T& v, std::string_view memberName)
			{
				using TMember = T;
				constexpr bool isInputEvent = std::is_member_function_pointer_v<T>;

				if constexpr (isInputEvent)
				{
				}
				else
				{
					node->AddInStream(Identifier(RemovePrefixAndSuffix(memberName)));
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

				if constexpr (isInputEvent || isOutputEvent)
				{
				}
				else
				{
					constexpr bool isInput = std::is_pointer_v<TMember>;

					if constexpr (isInput)
						v = (TMember)node->InValue(Identifier(RemovePrefixAndSuffix(memberName))).getRawData();
				}

				return true;
			}


			template<typename TNodeType>
			static bool RegisterEndpoints(TNodeType* node)
			{
				static_assert(DescribedNode<TNodeType>::value);
				using InputsDescription = typename NodeDescription<TNodeType>::Inputs;
				using OutputsDescription = typename NodeDescription<TNodeType>::Outputs;

				const bool insResult = InputsDescription::ApplyToStaticType([&node](const auto&... members)
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

				const bool outsResult = OutputsDescription::ApplyToStaticType([&node](const auto&... members)
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

						if constexpr (isInputEvent)
							return true;
						else
							return InitializeInputsImpl(node, node->*memberPtr, name);
					};
					return (unpack(members) && ...);
				});
			}
		}


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
	}
}

#ifndef NODE_INPUTS
#define NODE_INPUTS(...) __VA_ARGS__
#endif // !NODE_INPUTS

#ifndef NODE_OUTPUTS
#define NODE_OUTPUTS(...) __VA_ARGS__
#endif // !NODE_OUTPUTS

#ifndef DESCRIBE_NODE
#define DESCRIBE_NODE(NodeType, InputList, OutputList)                                         \
	DESCRIBED_TAGGED(NodeType, Hazel::AnimationGraph::TagInputs, InputList)                    \
	DESCRIBED_TAGGED(NodeType, Hazel::AnimationGraph::TagOutputs, OutputList)                  \
	                                                                                           \
	template<> struct Hazel::AnimationGraph::NodeDescription<NodeType>                         \
	{                                                                                          \
		using Inputs = Hazel::Type::Description<NodeType, Hazel::AnimationGraph::TagInputs>;   \
		using Outputs = Hazel::Type::Description<NodeType, Hazel::AnimationGraph::TagOutputs>; \
	};
#endif // !DESCRIBE_NODE

DESCRIBE_NODE(Hazel::AnimationGraph::AnimationPlayer,
	NODE_INPUTS(
		&Hazel::AnimationGraph::AnimationPlayer::in_Animation,
		&Hazel::AnimationGraph::AnimationPlayer::in_PlaybackSpeed,
		&Hazel::AnimationGraph::AnimationPlayer::in_Offset,
		&Hazel::AnimationGraph::AnimationPlayer::in_Loop),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::AnimationPlayer::out_Pose,
		&Hazel::AnimationGraph::AnimationPlayer::out_OnFinish,
		&Hazel::AnimationGraph::AnimationPlayer::out_OnLoop)
);

DESCRIBE_NODE(Hazel::AnimationGraph::RangedBlend,
	NODE_INPUTS(
		&Hazel::AnimationGraph::RangedBlend::in_AnimationA,
		&Hazel::AnimationGraph::RangedBlend::in_AnimationB,
		&Hazel::AnimationGraph::RangedBlend::in_PlaybackSpeedA,
		&Hazel::AnimationGraph::RangedBlend::in_PlaybackSpeedB,
		&Hazel::AnimationGraph::RangedBlend::in_Synchronize,
		&Hazel::AnimationGraph::RangedBlend::in_OffsetA,
		&Hazel::AnimationGraph::RangedBlend::in_OffsetB,
		&Hazel::AnimationGraph::RangedBlend::in_Loop,
		&Hazel::AnimationGraph::RangedBlend::in_RangeA,
		&Hazel::AnimationGraph::RangedBlend::in_RangeB,
		&Hazel::AnimationGraph::RangedBlend::in_Value),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::RangedBlend::out_Pose,
		&Hazel::AnimationGraph::RangedBlend::out_OnFinishA,
		&Hazel::AnimationGraph::RangedBlend::out_OnLoopA,
		&Hazel::AnimationGraph::RangedBlend::out_OnFinishB,
		&Hazel::AnimationGraph::RangedBlend::out_OnLoopB)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Get<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Get<float>::in_Array,
		&Hazel::AnimationGraph::Get<float>::in_Index),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Get<float>::out_Element)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Get<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Get<int>::in_Array,
		&Hazel::AnimationGraph::Get<int>::in_Index),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Get<int>::out_Element)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Get<int64_t>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Get<int64_t>::in_Array,
		&Hazel::AnimationGraph::Get<int64_t>::in_Index),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Get<int64_t>::out_Element)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Add<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Add<float>::in_Value1,
		&Hazel::AnimationGraph::Add<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Add<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Add<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Add<int>::in_Value1,
		&Hazel::AnimationGraph::Add<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Add<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Subtract<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Subtract<float>::in_Value1,
		&Hazel::AnimationGraph::Subtract<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Subtract<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Subtract<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Subtract<int>::in_Value1,
		&Hazel::AnimationGraph::Subtract<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Subtract<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Multiply<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Multiply<float>::in_Value,
		&Hazel::AnimationGraph::Multiply<float>::in_Multiplier),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Multiply<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Multiply<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Multiply<int>::in_Value,
		&Hazel::AnimationGraph::Multiply<int>::in_Multiplier),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Multiply<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Divide<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Divide<float>::in_Value,
		&Hazel::AnimationGraph::Divide<float>::in_Divisor),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Divide<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Divide<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Divide<int>::in_Value,
		&Hazel::AnimationGraph::Divide<int>::in_Divisor),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Divide<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Power,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Power::in_Base,
		&Hazel::AnimationGraph::Power::in_Exponent),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Power::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Log,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Log::in_Base,
		&Hazel::AnimationGraph::Log::in_Value),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Log::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Modulo,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Modulo::in_Value,
		&Hazel::AnimationGraph::Modulo::in_Divisor),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Modulo::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Min<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Min<float>::in_Value1,
		&Hazel::AnimationGraph::Min<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Min<float>::out_Value)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Min<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Min<int>::in_Value1,
		&Hazel::AnimationGraph::Min<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Min<int>::out_Value)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Max<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Max<float>::in_Value1,
		&Hazel::AnimationGraph::Max<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Max<float>::out_Value)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Max<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Max<int>::in_Value1,
		&Hazel::AnimationGraph::Max<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Max<int>::out_Value)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Clamp<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Clamp<float>::in_Value,
		&Hazel::AnimationGraph::Clamp<float>::in_Min,
		&Hazel::AnimationGraph::Clamp<float>::in_Max),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Clamp<float>::out_Value)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Clamp<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Clamp<int>::in_Value,
		&Hazel::AnimationGraph::Clamp<int>::in_Min,
		&Hazel::AnimationGraph::Clamp<int>::in_Max),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Clamp<int>::out_Value)
);

DESCRIBE_NODE(Hazel::AnimationGraph::MapRange<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::MapRange<float>::in_Value,
		&Hazel::AnimationGraph::MapRange<float>::in_InRangeMin,
		&Hazel::AnimationGraph::MapRange<float>::in_InRangeMax,
		&Hazel::AnimationGraph::MapRange<float>::in_OutRangeMin,
		&Hazel::AnimationGraph::MapRange<float>::in_OutRangeMax,
		&Hazel::AnimationGraph::MapRange<float>::in_Clamped),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::MapRange<float>::out_Value)
);

DESCRIBE_NODE(Hazel::AnimationGraph::MapRange<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::MapRange<int>::in_Value,
		&Hazel::AnimationGraph::MapRange<int>::in_InRangeMin,
		&Hazel::AnimationGraph::MapRange<int>::in_InRangeMax,
		&Hazel::AnimationGraph::MapRange<int>::in_OutRangeMin,
		&Hazel::AnimationGraph::MapRange<int>::in_OutRangeMax,
		&Hazel::AnimationGraph::MapRange<int>::in_Clamped),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::MapRange<int>::out_Value)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckEqual<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckEqual<int>::in_Value1,
		&Hazel::AnimationGraph::CheckEqual<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckEqual<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckNotEqual<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckNotEqual<int>::in_Value1,
		&Hazel::AnimationGraph::CheckNotEqual<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckNotEqual<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckLess<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckLess<int>::in_Value1,
		&Hazel::AnimationGraph::CheckLess<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckLess<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckLessEqual<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckLessEqual<int>::in_Value1,
		&Hazel::AnimationGraph::CheckLessEqual<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckLessEqual<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckGreater<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckGreater<int>::in_Value1,
		&Hazel::AnimationGraph::CheckGreater<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckGreater<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckGreaterEqual<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckGreaterEqual<int>::in_Value1,
		&Hazel::AnimationGraph::CheckGreaterEqual<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckGreaterEqual<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckEqual<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckEqual<float>::in_Value1,
		&Hazel::AnimationGraph::CheckEqual<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckEqual<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckNotEqual<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckNotEqual<float>::in_Value1,
		&Hazel::AnimationGraph::CheckNotEqual<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckNotEqual<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckLess<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckLess<float>::in_Value1,
		&Hazel::AnimationGraph::CheckLess<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckLess<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckLessEqual<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckLessEqual<float>::in_Value1,
		&Hazel::AnimationGraph::CheckLessEqual<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckLessEqual<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckGreater<float>,
		NODE_INPUTS(
		&Hazel::AnimationGraph::CheckGreater<float>::in_Value1,
		&Hazel::AnimationGraph::CheckGreater<float>::in_Value2),
		NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckGreater<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckGreaterEqual<float>,
		NODE_INPUTS(
		&Hazel::AnimationGraph::CheckGreaterEqual<float>::in_Value1,
		&Hazel::AnimationGraph::CheckGreaterEqual<float>::in_Value2),
		NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckGreaterEqual<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::And,
	NODE_INPUTS(
		&Hazel::AnimationGraph::And::in_Value1,
		&Hazel::AnimationGraph::And::in_Value2),
		NODE_OUTPUTS(
		&Hazel::AnimationGraph::And::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Or,
		NODE_INPUTS(
		&Hazel::AnimationGraph::Or::in_Value1,
		&Hazel::AnimationGraph::Or::in_Value2),
			NODE_OUTPUTS(
		&Hazel::AnimationGraph::Or::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Not,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Not::in_Value),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Not::out_Out)
);


DESCRIBE_NODE(Hazel::AnimationGraph::StateMachine,
	NODE_INPUTS(),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::StateMachine::out_Pose)
);

DESCRIBE_NODE(Hazel::AnimationGraph::State,
	NODE_INPUTS(),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::State::out_Pose)
);

DESCRIBE_NODE(Hazel::AnimationGraph::QuickState,
	NODE_INPUTS(
		&Hazel::AnimationGraph::QuickState::in_Animation),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::QuickState::out_Pose)
);

DESCRIBE_NODE(Hazel::AnimationGraph::TransitionNode,
	NODE_INPUTS(),
	NODE_OUTPUTS()
);

DESCRIBE_NODE(Hazel::AnimationGraph::BoolTrigger,
	NODE_INPUTS(
		&Hazel::AnimationGraph::BoolTrigger::in_Value),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::BoolTrigger::out_OnTrue,
		&Hazel::AnimationGraph::BoolTrigger::out_OnFalse)
);

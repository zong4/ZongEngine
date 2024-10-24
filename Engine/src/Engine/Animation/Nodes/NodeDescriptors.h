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

namespace Engine::AnimationGraph {

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
				if (Engine::Utils::StartsWith(name, "in_"))
					name.remove_prefix(sizeof("in_") - 1);
				else if (Engine::Utils::StartsWith(name, "out_"))
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
	DESCRIBED_TAGGED(NodeType, Engine::AnimationGraph::TagInputs, InputList)                    \
	DESCRIBED_TAGGED(NodeType, Engine::AnimationGraph::TagOutputs, OutputList)                  \
	                                                                                           \
	template<> struct Engine::AnimationGraph::NodeDescription<NodeType>                         \
	{                                                                                          \
		using Inputs = Engine::Type::Description<NodeType, Engine::AnimationGraph::TagInputs>;   \
		using Outputs = Engine::Type::Description<NodeType, Engine::AnimationGraph::TagOutputs>; \
	};
#endif // !DESCRIBE_NODE

DESCRIBE_NODE(Engine::AnimationGraph::AnimationPlayer,
	NODE_INPUTS(
		&Engine::AnimationGraph::AnimationPlayer::in_Animation,
		&Engine::AnimationGraph::AnimationPlayer::in_PlaybackSpeed,
		&Engine::AnimationGraph::AnimationPlayer::in_Offset,
		&Engine::AnimationGraph::AnimationPlayer::in_Loop),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::AnimationPlayer::out_Pose,
		&Engine::AnimationGraph::AnimationPlayer::out_OnFinish,
		&Engine::AnimationGraph::AnimationPlayer::out_OnLoop)
);

DESCRIBE_NODE(Engine::AnimationGraph::RangedBlend,
	NODE_INPUTS(
		&Engine::AnimationGraph::RangedBlend::in_AnimationA,
		&Engine::AnimationGraph::RangedBlend::in_AnimationB,
		&Engine::AnimationGraph::RangedBlend::in_PlaybackSpeedA,
		&Engine::AnimationGraph::RangedBlend::in_PlaybackSpeedB,
		&Engine::AnimationGraph::RangedBlend::in_Synchronize,
		&Engine::AnimationGraph::RangedBlend::in_OffsetA,
		&Engine::AnimationGraph::RangedBlend::in_OffsetB,
		&Engine::AnimationGraph::RangedBlend::in_Loop,
		&Engine::AnimationGraph::RangedBlend::in_RangeA,
		&Engine::AnimationGraph::RangedBlend::in_RangeB,
		&Engine::AnimationGraph::RangedBlend::in_Value),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::RangedBlend::out_Pose,
		&Engine::AnimationGraph::RangedBlend::out_OnFinishA,
		&Engine::AnimationGraph::RangedBlend::out_OnLoopA,
		&Engine::AnimationGraph::RangedBlend::out_OnFinishB,
		&Engine::AnimationGraph::RangedBlend::out_OnLoopB)
);

DESCRIBE_NODE(Engine::AnimationGraph::Get<float>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Get<float>::in_Array,
		&Engine::AnimationGraph::Get<float>::in_Index),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Get<float>::out_Element)
);

DESCRIBE_NODE(Engine::AnimationGraph::Get<int>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Get<int>::in_Array,
		&Engine::AnimationGraph::Get<int>::in_Index),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Get<int>::out_Element)
);

DESCRIBE_NODE(Engine::AnimationGraph::Get<int64_t>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Get<int64_t>::in_Array,
		&Engine::AnimationGraph::Get<int64_t>::in_Index),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Get<int64_t>::out_Element)
);

DESCRIBE_NODE(Engine::AnimationGraph::Add<float>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Add<float>::in_Value1,
		&Engine::AnimationGraph::Add<float>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Add<float>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::Add<int>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Add<int>::in_Value1,
		&Engine::AnimationGraph::Add<int>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Add<int>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::Subtract<float>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Subtract<float>::in_Value1,
		&Engine::AnimationGraph::Subtract<float>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Subtract<float>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::Subtract<int>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Subtract<int>::in_Value1,
		&Engine::AnimationGraph::Subtract<int>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Subtract<int>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::Multiply<float>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Multiply<float>::in_Value,
		&Engine::AnimationGraph::Multiply<float>::in_Multiplier),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Multiply<float>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::Multiply<int>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Multiply<int>::in_Value,
		&Engine::AnimationGraph::Multiply<int>::in_Multiplier),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Multiply<int>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::Divide<float>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Divide<float>::in_Value,
		&Engine::AnimationGraph::Divide<float>::in_Divisor),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Divide<float>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::Divide<int>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Divide<int>::in_Value,
		&Engine::AnimationGraph::Divide<int>::in_Divisor),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Divide<int>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::Power,
	NODE_INPUTS(
		&Engine::AnimationGraph::Power::in_Base,
		&Engine::AnimationGraph::Power::in_Exponent),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Power::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::Log,
	NODE_INPUTS(
		&Engine::AnimationGraph::Log::in_Base,
		&Engine::AnimationGraph::Log::in_Value),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Log::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::Modulo,
	NODE_INPUTS(
		&Engine::AnimationGraph::Modulo::in_Value,
		&Engine::AnimationGraph::Modulo::in_Divisor),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Modulo::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::Min<float>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Min<float>::in_Value1,
		&Engine::AnimationGraph::Min<float>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Min<float>::out_Value)
);

DESCRIBE_NODE(Engine::AnimationGraph::Min<int>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Min<int>::in_Value1,
		&Engine::AnimationGraph::Min<int>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Min<int>::out_Value)
);

DESCRIBE_NODE(Engine::AnimationGraph::Max<float>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Max<float>::in_Value1,
		&Engine::AnimationGraph::Max<float>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Max<float>::out_Value)
);

DESCRIBE_NODE(Engine::AnimationGraph::Max<int>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Max<int>::in_Value1,
		&Engine::AnimationGraph::Max<int>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Max<int>::out_Value)
);

DESCRIBE_NODE(Engine::AnimationGraph::Clamp<float>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Clamp<float>::in_Value,
		&Engine::AnimationGraph::Clamp<float>::in_Min,
		&Engine::AnimationGraph::Clamp<float>::in_Max),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Clamp<float>::out_Value)
);

DESCRIBE_NODE(Engine::AnimationGraph::Clamp<int>,
	NODE_INPUTS(
		&Engine::AnimationGraph::Clamp<int>::in_Value,
		&Engine::AnimationGraph::Clamp<int>::in_Min,
		&Engine::AnimationGraph::Clamp<int>::in_Max),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Clamp<int>::out_Value)
);

DESCRIBE_NODE(Engine::AnimationGraph::MapRange<float>,
	NODE_INPUTS(
		&Engine::AnimationGraph::MapRange<float>::in_Value,
		&Engine::AnimationGraph::MapRange<float>::in_InRangeMin,
		&Engine::AnimationGraph::MapRange<float>::in_InRangeMax,
		&Engine::AnimationGraph::MapRange<float>::in_OutRangeMin,
		&Engine::AnimationGraph::MapRange<float>::in_OutRangeMax,
		&Engine::AnimationGraph::MapRange<float>::in_Clamped),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::MapRange<float>::out_Value)
);

DESCRIBE_NODE(Engine::AnimationGraph::MapRange<int>,
	NODE_INPUTS(
		&Engine::AnimationGraph::MapRange<int>::in_Value,
		&Engine::AnimationGraph::MapRange<int>::in_InRangeMin,
		&Engine::AnimationGraph::MapRange<int>::in_InRangeMax,
		&Engine::AnimationGraph::MapRange<int>::in_OutRangeMin,
		&Engine::AnimationGraph::MapRange<int>::in_OutRangeMax,
		&Engine::AnimationGraph::MapRange<int>::in_Clamped),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::MapRange<int>::out_Value)
);

DESCRIBE_NODE(Engine::AnimationGraph::CheckEqual<int>,
	NODE_INPUTS(
		&Engine::AnimationGraph::CheckEqual<int>::in_Value1,
		&Engine::AnimationGraph::CheckEqual<int>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::CheckEqual<int>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::CheckNotEqual<int>,
	NODE_INPUTS(
		&Engine::AnimationGraph::CheckNotEqual<int>::in_Value1,
		&Engine::AnimationGraph::CheckNotEqual<int>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::CheckNotEqual<int>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::CheckLess<int>,
	NODE_INPUTS(
		&Engine::AnimationGraph::CheckLess<int>::in_Value1,
		&Engine::AnimationGraph::CheckLess<int>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::CheckLess<int>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::CheckLessEqual<int>,
	NODE_INPUTS(
		&Engine::AnimationGraph::CheckLessEqual<int>::in_Value1,
		&Engine::AnimationGraph::CheckLessEqual<int>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::CheckLessEqual<int>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::CheckGreater<int>,
	NODE_INPUTS(
		&Engine::AnimationGraph::CheckGreater<int>::in_Value1,
		&Engine::AnimationGraph::CheckGreater<int>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::CheckGreater<int>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::CheckGreaterEqual<int>,
	NODE_INPUTS(
		&Engine::AnimationGraph::CheckGreaterEqual<int>::in_Value1,
		&Engine::AnimationGraph::CheckGreaterEqual<int>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::CheckGreaterEqual<int>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::CheckEqual<float>,
	NODE_INPUTS(
		&Engine::AnimationGraph::CheckEqual<float>::in_Value1,
		&Engine::AnimationGraph::CheckEqual<float>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::CheckEqual<float>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::CheckNotEqual<float>,
	NODE_INPUTS(
		&Engine::AnimationGraph::CheckNotEqual<float>::in_Value1,
		&Engine::AnimationGraph::CheckNotEqual<float>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::CheckNotEqual<float>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::CheckLess<float>,
	NODE_INPUTS(
		&Engine::AnimationGraph::CheckLess<float>::in_Value1,
		&Engine::AnimationGraph::CheckLess<float>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::CheckLess<float>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::CheckLessEqual<float>,
	NODE_INPUTS(
		&Engine::AnimationGraph::CheckLessEqual<float>::in_Value1,
		&Engine::AnimationGraph::CheckLessEqual<float>::in_Value2),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::CheckLessEqual<float>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::CheckGreater<float>,
		NODE_INPUTS(
		&Engine::AnimationGraph::CheckGreater<float>::in_Value1,
		&Engine::AnimationGraph::CheckGreater<float>::in_Value2),
		NODE_OUTPUTS(
		&Engine::AnimationGraph::CheckGreater<float>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::CheckGreaterEqual<float>,
		NODE_INPUTS(
		&Engine::AnimationGraph::CheckGreaterEqual<float>::in_Value1,
		&Engine::AnimationGraph::CheckGreaterEqual<float>::in_Value2),
		NODE_OUTPUTS(
		&Engine::AnimationGraph::CheckGreaterEqual<float>::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::And,
	NODE_INPUTS(
		&Engine::AnimationGraph::And::in_Value1,
		&Engine::AnimationGraph::And::in_Value2),
		NODE_OUTPUTS(
		&Engine::AnimationGraph::And::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::Or,
		NODE_INPUTS(
		&Engine::AnimationGraph::Or::in_Value1,
		&Engine::AnimationGraph::Or::in_Value2),
			NODE_OUTPUTS(
		&Engine::AnimationGraph::Or::out_Out)
);

DESCRIBE_NODE(Engine::AnimationGraph::Not,
	NODE_INPUTS(
		&Engine::AnimationGraph::Not::in_Value),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::Not::out_Out)
);


DESCRIBE_NODE(Engine::AnimationGraph::StateMachine,
	NODE_INPUTS(),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::StateMachine::out_Pose)
);

DESCRIBE_NODE(Engine::AnimationGraph::State,
	NODE_INPUTS(),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::State::out_Pose)
);

DESCRIBE_NODE(Engine::AnimationGraph::QuickState,
	NODE_INPUTS(
		&Engine::AnimationGraph::QuickState::in_Animation),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::QuickState::out_Pose)
);

DESCRIBE_NODE(Engine::AnimationGraph::TransitionNode,
	NODE_INPUTS(),
	NODE_OUTPUTS()
);

DESCRIBE_NODE(Engine::AnimationGraph::BoolTrigger,
	NODE_INPUTS(
		&Engine::AnimationGraph::BoolTrigger::in_Value),
	NODE_OUTPUTS(
		&Engine::AnimationGraph::BoolTrigger::out_OnTrue,
		&Engine::AnimationGraph::BoolTrigger::out_OnFalse)
);

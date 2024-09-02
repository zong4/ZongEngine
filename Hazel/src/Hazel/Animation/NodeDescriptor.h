#pragma once

#include "NodeProcessor.h"

#include "Hazel/Reflection/TypeDescriptor.h"

#include <choc/text/choc_StringUtilities.h>
#include <optional>

namespace Hazel::AnimationGraph {

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

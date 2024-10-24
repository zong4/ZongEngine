#pragma once
#include "AnimationGraphAsset.h"
#include "AnimationGraphEditorTypes.h"

#include "Engine/Animation/Nodes/NodeDescriptors.h"
#include "Engine/Editor/NodeGraphEditor/Nodes.h"

#include <choc/containers/choc_Value.h>

#include <optional>
#include <string>
#include <string_view>

namespace Engine::AnimationGraph {

	std::optional<choc::value::Value> GetPinDefaultValueOverride(std::string_view nodeName, std::string_view memberNameSanitized);
	std::optional<Types::EPinType> GetPinTypeForMemberOverride(std::string_view nodeName, std::string_view memberNameSanitized);

	namespace Impl {

		// Utility struct to catch unwanted types by using function overloading instead of
		// function template partial specialization which is not available in C++
		template <typename>
		struct CatchType {};
	

		template<typename TProcNode, typename TListDescr>
		auto ConstructPinList() //! no constexpr
		{
			using NodeDescr = NodeDescription<TProcNode>;

			constexpr bool isInputs = std::is_same_v<TListDescr, typename NodeDescr::Inputs>;

			std::vector<Pin*> list;

			TListDescr::ApplyToStaticType([&list](const auto&... members)
			{
				auto unpack = [&list, memberIndex = 0](auto memberPtr) mutable
				{
					using TMemberPtr = /*typename */decltype(memberPtr);
					using TMember = typename Type::member_pointer::return_type<TMemberPtr>::type;

					const bool isArrayC = std::is_array_v<TMember>;
					using TMemberDecay = std::conditional_t<isArrayC, std::remove_pointer_t<std::decay_t<TMember>>, std::decay_t<TMember>>;

					std::string pinName{ TListDescr::MemberNames[memberIndex] };
					const std::string pinNameSanitized{ EndpointUtilities::Impl::RemovePrefixAndSuffix(pinName) };

					std::optional<Types::EPinType> opt = GetPinTypeForMemberOverride(TListDescr::ClassName, pinNameSanitized); //! no constexpr
					const Types::EPinType type = opt.value_or(Types::GetPinTypeForMember<TProcNode, TMemberPtr>()); //! no constexpr

					Pin* pin = Types::CreatePinForType(type);
					ZONG_CORE_ASSERT(pin);

					if (!pin)
					{
						memberIndex++;
						return false;
					}

					pin->Name = Utils::SplitAtUpperCase(pinNameSanitized);

					//? slight coupling: assuming all array input and output variables in NodeProcessors named this way
					const bool isArray = isArrayC || Type::is_array_v<TMemberDecay> || std::is_array_v<TMemberDecay> || Utils::StartsWith(pinName, "in_Array") || Utils::StartsWith(pinName, "out_Array");
					pin->Storage = isArray ? StorageKind::Array : StorageKind::Value;

					pin->Kind = isInputs ? PinKind::Input : PinKind::Output;

					if (isArray)
					{
						pin->Value = choc::value::Value();
					}
					else if (auto optValue = GetPinDefaultValueOverride(TListDescr::ClassName, pinNameSanitized))
					{
						pin->Value = *optValue;
					}

					list.push_back(pin);

					memberIndex++;

					return true;
				};

				return (unpack(members) && ...);
			});

			return list;
		}

		template<class TProcNode>
		[[nodiscard]] Node* CreateNodeForProcessor()
		{
			static_assert(DescribedNode<TProcNode>::value);

			using TDescriptor = NodeDescription<TProcNode>;

			auto* newNode = new Types::Node(Engine::Utils::SplitAtUpperCase(TDescriptor::Inputs::ClassName.data()), ImColor(255, 128, 128));
			newNode->Inputs = Impl::ConstructPinList<TProcNode, typename TDescriptor::Inputs>();
			newNode->Outputs = Impl::ConstructPinList<TProcNode, typename TDescriptor::Outputs>();

			return newNode;
		}

	} // namespace Impl


	class AnimationGraphNodeFactory : public Nodes::Factory<AnimationGraphNodeFactory>
	{
	public:
		using Types = Types;
		[[nodiscard]] Node* SpawnNode(const std::string& category, const std::string& name) const override { return SpawnNodeStatic(category, name); }

		[[nodiscard]] static Node* SpawnGraphPropertyNode(const Ref<AnimationGraphAsset>& graph, std::string_view propertyName, EPropertyType type, std::optional<bool> isLocalVarGetter = {});
	};


	class StateMachineNodeFactory : public Nodes::Factory<StateMachineNodeFactory>
	{
	public:
		using Types = Types;
		[[nodiscard]] Node* SpawnNode(const std::string& category, const std::string& name) const override { return SpawnNodeStatic(category, name); }

		// transition nodes are not added to the registry (so you cannot create them from the node graph context menu)
		static Node* CreateTransitionNode();
	};


	class TransitionGraphNodeFactory : public Nodes::Factory<TransitionGraphNodeFactory>
	{
	public:
		using Types = Types;
		[[nodiscard]] Node* SpawnNode(const std::string& category, const std::string& name) const override { return SpawnNodeStatic(category, name); }
	};

} // namespace Engine::Nodes

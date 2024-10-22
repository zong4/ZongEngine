#pragma once
#include "SoundGraphAsset.h"
#include "SoundGraphEditorTypes.h"

#include "Hazel/Audio/SoundGraph/Nodes/NodeDescriptors.h"
#include "Hazel/Editor/NodeGraphEditor/Nodes.h"

#include <choc/containers/choc_Value.h>

#include <optional>
#include <string>
#include <string_view>

namespace Hazel::SoundGraph {

	std::optional<choc::value::Value> GetPinDefaultValueOverride(std::string_view nodeName, std::string_view memberNameSanitized);
	std::optional<SGTypes::ESGPinType> GetPinTypeForMemberOverride(std::string_view nodeName, std::string_view memberNameSanitized);

	namespace Impl {
		//==============================================================================
		/** Utility struct to catch unwanted types by using function overloading instead of
			function template partial specialization which is not available in C++
		*/
		template <typename>
		struct CatchType {};


		//? Old way of parsing static node processor type info into dynamic editor graph nodes
#if 0
		//==============================================================================
		/// Implementation of the CreatePin function. Parsing type data into a Pin object.
		template<class TProcNode, typename TMemberPtr, typename TMember>
		constexpr bool CreatePinImpl(Node* node, std::string_view pinName, CatchType<TMember>)
		{
			using TMemberRaw = std::remove_pointer_t<TMember>;
			static_assert(!std::is_void_v<TMemberRaw>);


			//static const uint32_t nameID = (uint32_t)Identifier(pinName);
			std::optional<SGTypes::ESGPinType> opt = GetPinTypeForMemberOverride(Type::Description<TProcNode>::ClassName, pinName);
			const SGTypes::ESGPinType type = opt.value_or(GetPinTypeForMember<TProcNode, TMemberPtr>());

			//? slight coupling: assuming all array input and output variables in NodeProcessors named this way
			const bool isArray = Type::is_array_v<TMember> || std::is_array_v<TMember> || Utils::StartsWith(pinName, "in_Array") || Utils::StartsWith(pinName, "out_Array");
			const StorageKind storageKind = isArray ? StorageKind::Array : StorageKind::Value;

			const bool isInput = isArray ? Utils::StartsWith(pinName, "in_") : std::is_pointer_v<TMember>;

			if (isInput)
			{
				choc::value::Value defaultValue = GetPinDefaultValue(Type::Description<TProcNode>::ClassName, pinName);

				pinName = RemovePrefixAndSuffix(pinName);

				if (node->AddInputPin(type))
				{
					//node->
				}

				node->Inputs.emplace_back(0, Utils::SplitAtUpperCase(pinName), type, storageKind,
											isArray ? choc::value::Value() : choc::value::Value(defaultValue)).Kind = PinKind::Input;

				if (type == SGTypes::ESGPinType::Enum)
					node->Inputs.back().EnumTokens = SoundGraphNodeFactory::GetEnumTokens(Type::Description<TProcNode>::ClassName, pinName);
			}
			else
			{
				pinName = RemovePrefixAndSuffix(pinName);
				node->Outputs.emplace_back(0, Utils::SplitAtUpperCase(pinName), type, storageKind).Kind = PinKind::Output;
			}

			return true;
		};


		//==============================================================================
		/// This function catches compiler generated template from member function
		/// pointer. We're only insterested in member object pointers.
		template<class TProcNode, typename TMemberPtr, typename TMember>
		constexpr bool CreatePinImpl(Node& node, std::string_view pinName, CatchType<void>)
		{
			static_assert(false);
			return false;
		}

		//==============================================================================
		/// Event Pins
		static inline bool CreateInputEventPinImpl(Node* node, std::string_view pinName)
		{
			pinName = RemovePrefixAndSuffix(pinName);
			//node->Inputs.emplace_back(0, Utils::SplitAtUpperCase(pinName), SGTypes::ESGPinType::Function).Kind = PinKind::Input;
			return true;
		}

		static inline bool CreateOutputEventPinImpl(Node* node, std::string_view pinName)
		{
			pinName = RemovePrefixAndSuffix(pinName);
			//node->Outputs.emplace_back(0, Utils::SplitAtUpperCase(pinName), SGTypes::ESGPinType::Function).Kind = PinKind::Output;
			return true;
		}

		//==============================================================================
		/// Routing function. Selects implementation function for the member pointer type.
		template<class TProcNode, typename TMemberPtr>
		constexpr bool CreatePin(Node& node, std::string_view pinName)
		{
			using TMember = typename Type::member_pointer::return_type<TMemberPtr>::type;
			constexpr bool isInputEvent = std::is_member_function_pointer_v<TMemberPtr>;
			constexpr bool isOutputEvent = std::is_same_v<TMember, Hazel::SoundGraph::NodeProcessor::OutputEvent>;

			if constexpr (isInputEvent)
			{
				return CreateInputEventPinImpl(node, pinName);
			}
			else if constexpr (isOutputEvent)
			{
				return CreateOutputEventPinImpl(node, pinName);
			}
			else
			{
				// std::dekay turns array into pointer, pointer we parse as input,
				// therefore we don't decay arrays
				//? need to validate all array type options, ins and outs
				const bool isArray = std::is_array_v<TMember>;
				using TMemberDecay = std::conditional_t<isArray, std::remove_pointer_t<std::decay_t<TMember>>, std::decay_t<TMember>>;
				return CreatePinImpl<TProcNode, TMemberPtr, TMemberDecay>(node, pinName, CatchType<TMemberDecay>{});
			}
		}
#endif

		//==============================================================================
		template<typename TProcNode, typename TListDescr>
		auto ConstructPinList() //! no constexpr
		{
			using NodeDescr = NodeDescription<TProcNode>;

			constexpr bool isInputs = std::is_same_v<TListDescr, typename NodeDescr::Inputs>;

			std::vector<Pin*> list;

			TListDescr::ApplyToStaticType(
						[&list](const auto&... members)
			{
				auto unpack = [&list, memberIndex = 0](auto memberPtr) mutable
				{
					using TMemberPtr = /*typename */decltype(memberPtr);
					using TMember = typename Type::member_pointer::return_type<TMemberPtr>::type;

					const bool isArrayC = std::is_array_v<TMember>;
					using TMemberDecay = std::conditional_t<isArrayC, std::remove_pointer_t<std::decay_t<TMember>>, std::decay_t<TMember>>;

					std::string pinName{ TListDescr::MemberNames[memberIndex] };
					const std::string pinNameSanitized{ EndpointUtilities::Impl::RemovePrefixAndSuffix(pinName) };

					std::optional<SGTypes::ESGPinType> opt = GetPinTypeForMemberOverride(TListDescr::ClassName, pinNameSanitized); //! no constexpr
					const SGTypes::ESGPinType type = opt.value_or(SGTypes::GetPinTypeForMember<TProcNode, TMemberPtr>()); //! no constexpr

					Pin* pin = SGTypes::CreatePinForType(type);
					HZ_CORE_ASSERT(pin);

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
			}
			);

			return list;
		}
	} // namespace Impl


	class SoundGraphNodeFactory : public Nodes::Factory<SoundGraphNodeFactory>
	{
	public:

		using Types = SGTypes;

		[[nodiscard]] Hazel::Node* SpawnNode(const std::string& category, const std::string& type) const override { return SpawnNodeStatic(category, type); }

		using ProcessorInitArguments = std::map<std::string, std::map<std::string, const char*>>;
		static const ProcessorInitArguments ProcessorArguments;

#if 0
		template<class TProcNode>
		static constexpr Node* CreateNodeForProcessorOld()
		{
			static_assert(Type::Described<TProcNode>::value);

			using TDescriptor = Type::Description<TProcNode>;

			auto* node = new SGTypes::SGNode(0, Hazel::Utils::SplitAtUpperCase(TDescriptor::ClassName.data()), ImColor(255, 128, 128));

			// Iterate over members of the NodeProcessor type and construct pins from deduced member types
			TDescriptor::ApplyToStaticType(
				[&node](const auto&... members)
			{
				auto unpack = [&node, memberIndex = 0](auto memberPtr) mutable
				{
					using TMemberPtr = decltype(memberPtr);
					return Impl::CreatePin<TProcNode, TMemberPtr>(*node, TDescriptor::MemberNames[memberIndex++]);
				};

				return (unpack(members) && ...);
			});

			return node;
		}
#endif

		template<class TProcNode>
		[[nodiscard]] static Node* CreateNodeForProcessor()
		{
			static_assert(DescribedNode<TProcNode>::value);

			using TDescriptor = NodeDescription<TProcNode>;

			auto* newNode = new SGTypes::SGNode(Hazel::Utils::SplitAtUpperCase(TDescriptor::Inputs::ClassName.data()), ImColor(255, 128, 128));
			newNode->Inputs = Impl::ConstructPinList<TProcNode, typename TDescriptor::Inputs>();
			newNode->Outputs = Impl::ConstructPinList<TProcNode, typename TDescriptor::Outputs>();

			return newNode;
		}

		[[nodiscard]] static Node* SpawnGraphPropertyNode(const Ref<SoundGraphAsset>& graph, std::string_view propertyName, EPropertyType type, std::optional<bool> isLocalVarGetter = {});
	};

} // namespace Hazel::SoundGraph

/* TODO: Static vs Dynamic

	- Outer - Dynamic (Node*, Pin*)
	- Property edit - Static overload (DrawProperty<T>(int flags)?
	- Pin compatibility - Static?

	- Init choc::Value for pin - Static overload
		-- TPinTypesTuple->GetType-> CreateValue(int flags) overload
		-- GetValueOverload(PinFullNameId) -> get default value
*/

//! Testing purely static reflection / node fabrication
#if 0

using TTestNode = NodeDescription<Hazel::SoundGraph::Add<float>>;



template<int MemberIndex, typename TPinType>
struct TypeOverride
{
	static constexpr int member_index = MemberIndex;
	using pin_type = TPinType;
};

template<typename TNodeDescription, typename ...TOverrides>
struct NodePinFactory
{
	/*template<int MemberIndex>
	static constexpr auto CreatePin()
	{
	}*/

	//static constexpr int ii = magic_enum::enum_integer(*magic_enum::enum_cast<SGTypes::ESGPinType>("Audio"));

	//using TestType = typename TypeOverride<TTestNode::Inputs::IndexOf("in_Value1"), SGTypes::TPin<SGTypes::Audio>>;

	// TODO: use tuple type for type overrides
	using TupleType = std::tuple<TOverrides...>;

	std::variant<>();

	// TODO: maybe use std::variant to apply only some of the tuple value overrides???
	static inline constexpr std::tuple pinTypes = { TNodeDescription::TInputsDescr::TTuple }

		// TODO: use function to lazily construct pin type with default value override
	static inline constexpr std::tuple<TOverrides...> overrides{};

	template<int MemberIndex>
	class Construct
	{
	public:
	public:
		// check return value from the matching "test" overload:
		using Pin = std::tuple_element_t<MemberIndex, TupleType>::pin_type;// typename decltype(Impl<MemberIndex>());
	};
};



template<size_t Size>
using magic_string = magic_enum::detail::static_string<Size>;

#define magic_str(str) magic_string<std::string_view(str).size()>(str)

static constexpr auto ssss = magic_str("in_Value1");

using TestPinFactory = struct NodePinFactory<TTestNode,
	TypeOverride<TTestNode::Inputs::IndexOf("in_Value1"), SGTypes::TPin<SGTypes::Audio>>,
	TypeOverride<TTestNode::Inputs::IndexOf("in_Value2"), SGTypes::TPin<SGTypes::Float>>
>;

template<class ...Inputs>
using InputPins = meta::_Meta_list<Inputs>;

template<class ...PinTypes>
using InputPinTypes = meta::_Meta_list<PinTypes>;

using MetaIns = InputPins<meta::_Meta_as_list<TTestNode::Inputs::TTuple>>;

using _TTest = meta::_Meta_quote_i<SGTypes::TPin>;
using _TTestPin = meta::_Meta_invoke_i<_TTest, SGTypes::Audio>;
static_assert(std::is_same_v<_TTestPin, SGTypes::TPin<SGTypes::Audio>>);


using TTestL = meta::_Meta_transform<meta::_Meta_quote_i<SGTypes::TPin>, meta::_Meta_list_i<SGTypes::Audio, SGTypes::Float>>;

static_assert(std::is_same_v<TTestL, meta::_Meta_list<SGTypes::TPin<SGTypes::Audio>, SGTypes::TPin<SGTypes::Float>>>);

//static_assert(std::is_same_v<_TTest, _Meta_list<_Meta_list<char>, _Meta_list<unsigned int>>>);

// TODO: for each Input in InputPins get Type in InputPinTypes, InputPinTypes constructed with optional overrides


using testpin = TestPinFactory::Construct<0>;

//static constexpr auto ttts = std::get<0>(NodePinFactory<float>::overrides).member_index;
using ttts = std::tuple_element_t<1, TestPinFactory::TupleType>::pin_type;

static constexpr auto testvariable = TTestNode::Inputs::MemberNames[0];

static constexpr auto in = TTestNode::Inputs::IndexOf("in_Value3");

struct StaticAction
{
	template<class TFunc, class ...Args>
	constexpr StaticAction(TFunc&& function, Args&&...args)
	{
		function(std::forward<Args>(args)...);
	}
};

class PrivateMap
{
	static inline std::unordered_map<std::string_view, SGTypes::ESGPinType> s_Map;

public:
	static void Add(std::string_view string, int value) { s_Map.insert_or_assign(std::string(string), value); }
};


static const StaticAction sa([] { PrivateMap::Add("Hazel::SoundGraph::Add<float>::in_Value1", SGTypes::Audio); });

#define NODE_OVERRIDES(NodeType, Overrides, ...)\
static const StaticAction sa([] { PrivateMap::Add("Hazel::SoundGraph::Add<float>::in_Value1", SGTypes::Audio); });\


NODE_OVERRIDES(Hazel::SoundGraph::Add<float>,
	{ Hazel::SoundGraph::Add<float>::in_Value1, SGTypes::TPin<SGTypes::Audio> }
);
#endif

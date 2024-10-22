#pragma once

#include "PropertySet.h"

#include "Hazel/Core/UUID.h"
#include "Hazel/Reflection/TypeName.h"
#include "Hazel/Utilities/StringUtils.h"

#include <choc/containers/choc_Value.h>
#include <choc/text/choc_StringUtilities.h>
#include <imgui/imgui.h>
#include <magic_enum.hpp>

#include <functional>
#include <map>
#include <optional>

namespace Hazel {

	//! Add your custom pin / value types
	enum EPinType : int
	{
		Flow,
		Bool,
		Int,
		Float,
		String,
		Object,
		Function,
		DelegatePin,
		Enum
	};

	enum class PinKind
	{
		Output,
		Input
	};

	enum class StorageKind
	{
		Value = 0,
		Array
	};

	//! Add your custom Node types
	enum class NodeType
	{
		Blueprint,
		Simple,
		Comment,
		Subroutine
	};

	enum class EPropertyType : uint8_t
	{
		Invalid = 0,
		Input,
		Output,
		LocalVariable
	};

	//! Set color for your pin / value types
	inline ImColor GetIconColor(EPinType type)
	{
		switch (type)
		{
			default:
			case EPinType::Flow:        return ImColor(200, 200, 200);
			case EPinType::Bool:        return ImColor(220, 48, 48);
			case EPinType::Int:         return ImColor(68, 201, 156);
			case EPinType::Float:       return ImColor(147, 226, 74);
			case EPinType::String:      return ImColor(194, 75, 227);
			case EPinType::Object:      return ImColor(51, 150, 215);
			case EPinType::Function:    return ImColor(200, 200, 200);
			case EPinType::DelegatePin: return ImColor(255, 48, 48);
			case EPinType::Enum:        return ImColor(2, 92, 48);
		}
	};


	//=================================================================
	struct Node;

	struct Token
	{
		const std::string_view name;
		const int value;
	};

	struct TFlow {};
	struct TFunction {};
	struct TDelegate {};

	// TODO: JP. this utility could be moved out of this header
	//=================================================================
	/*	Creates Value for a type. For non-fundamental types
		creates an empty Value object with the name of the type.

		If non-default initialization required for a non-fundamental,
		create a template overload for your type.
	*/
	template<class T>
	static choc::value::Value ValueFrom(const T& obj)
	{
		static constexpr bool is_string = std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>;

		if constexpr (std::is_same_v<T, bool> || (std::is_arithmetic_v<T> && !std::is_unsigned_v<T>) || is_string)
		{
			return choc::value::Value(obj);
		}
		else
		{
			// By default, custom types we store as choc Object with name of the type and a binary array of "data".

			static_assert(sizeof(T) >= sizeof(int32_t), "T must be larger than int32.");
			static_assert(sizeof(T) % sizeof(int32_t) == 0, "Size of T must be a multiple of size of int32.");

			static constexpr auto dataSize = sizeof(T) / sizeof(int32_t);

			choc::value::Value valueObject = choc::value::createObject(type::type_name<T>());

			// We need to store data as choc Array instead of Vector to keep to/from YAML serialization generic
			// for choc Value and choc Array can handle any type, not just primitives.
			const auto* data = reinterpret_cast<const int32_t*>(&obj);
			valueObject.addMember("Data", choc::value::createArray((uint32_t)dataSize, [&data](uint32_t i) { return data[i]; }));

			return valueObject;
		}
	}

	template<> choc::value::Value ValueFrom(const TFlow& obj) { return choc::value::createObject(type::type_name<TFlow>()); }
	template<> choc::value::Value ValueFrom(const TFunction& obj) { return choc::value::createObject(type::type_name<TFunction>()); }
	template<> choc::value::Value ValueFrom(const TDelegate& obj) { return choc::value::createObject(type::type_name<TDelegate>()); }

	//? not used (see Utils::CreateAssetHandle<>()) for creation of values from asset handles
	template<> choc::value::Value ValueFrom(const UUID& obj)
	{
		choc::value::Value valueObject = choc::value::createObject(type::type_name<UUID>());
		valueObject.addMember("Value", choc::value::Value((int64_t)obj));
		return valueObject;
	}

	// TODO: JP. this utility could be moved out of this header
	//=================================================================
	/*	Read custom data type created with ValueFrom<T>() function.

		@returns std::nullopt if the 'customValueObject' was not created
		with ValueFrom<T>() function.
	*/
	template<class T>
	static std::optional<T> CustomValueTo(choc::value::ValueView customValueObject)
	{
		if constexpr (std::is_same_v<T, TFlow> || std::is_same_v<T, TFunction> || std::is_same_v<T, TDelegate>)
		{
			if (customValueObject.isObjectWithClassName(type::type_name<T>()))
				return T();
			else
				return {};
		}
		else
		{
			if (customValueObject.isObjectWithClassName(type::type_name<T>()))
				return *(T*)(customValueObject["Data"].getRawData());
			else
				return {};
		}
	}

	//=================================================================
	/**
		Pin base. Using 'int typeID' for type-erased implementation
		specific enum types.
	*/
	struct Pin
	{
		// Required Base members
		uint64_t ID;
		uint64_t NodeID;
		std::string Name;
		choc::value::Value Value; // Value should be the only thing to serialize, the rest is constructed from factory (TBD)

		StorageKind Storage;
		PinKind     Kind;

		std::optional<const std::vector<Token>*> EnumTokens{};	// TODO: replace with magic_enum utilities

		//! Pin type is erased from the base class and handled by the implementation

		bool IsSameType(const Pin* other) const { return GetType() == other->GetType(); };
		bool IsType(int typeID) const { return GetType() == typeID; };

	protected:
		Pin(uint64_t id,
			std::string_view name,
			StorageKind storageKind = StorageKind::Value,
			choc::value::Value defaultValue = choc::value::Value()
		)
			: ID(id)
			, NodeID(0)
			, Name(name)
			, Value(defaultValue)
			, Storage(storageKind)
			, Kind(PinKind::Input)
		{}

		Pin()
			: ID(0)
			, NodeID(0)
			, Name("")
			, Value(choc::value::Value())
			, Storage(StorageKind::Value)
			, Kind(PinKind::Input)
		{}

	public:
		virtual ~Pin() = default;

		virtual int GetType() const = 0;
		virtual std::string_view GetTypeString() const = 0;
	};

	//=================================================================
	/* Optional utility to handle Pin types statically
	*/
	template<int EType, class TValueType, ImU32 Color = IM_COL32_WHITE, class ETypeEnum = EPinType>
	struct PinDescr : Pin
	{
		static constexpr auto pin_type = EType;
		static constexpr auto color = Color;
		using value_type = TValueType;

		/*constexpr*/ explicit PinDescr(const value_type& value)
			//: DefaultValue(value)
		{
			Value = ValueFrom(value);
		}
#if 0
		const value_type DefaultValue;
#endif
		virtual int GetType() const override { return pin_type; }
		virtual std::string_view GetTypeString() const override { return magic_enum::enum_name<ETypeEnum>((ETypeEnum)EType); }

		//virtual ImU32 GetColor() const override { return color; }
	};

	template<typename TPin, int EType>
	struct is_matching_pin_type { static constexpr bool value = TPin::pin_type == EType; };

	template<typename TPin, int EType>
	static constexpr bool is_matching_pin_type_v = TPin::pin_type == EType;

	using TDefaultPinTypes = std::tuple
	<
		PinDescr<EPinType::Flow,        TFlow,        IM_COL32(200, 200, 200, 255)>,
		PinDescr<EPinType::Bool,        bool,         IM_COL32(220, 48, 48, 255)>,
		PinDescr<EPinType::Int,         int,          IM_COL32(68, 201, 156, 255)>,
		PinDescr<EPinType::Float,       float,        IM_COL32(147, 226, 74, 255)>,
		PinDescr<EPinType::String,      std::string,  IM_COL32(194, 75, 227, 255)>,
		PinDescr<EPinType::Object,      UUID,         IM_COL32(51, 150, 215, 255)>,
		PinDescr<EPinType::Function,    TFunction,    IM_COL32(200, 200, 200, 255)>,
		PinDescr<EPinType::DelegatePin, TDelegate,    IM_COL32(255, 48, 48, 255)>,
		PinDescr<EPinType::Enum,        int,          IM_COL32(2, 92, 48, 255)>
	>;

	// Immutable type doesn't allow changing type on the fly,
	// it requires creating new pin of the new type to replace the old one

	// Example of how you could define default values for pin types
#if 0
	static const TDefaultPinTypes s_MyPinDefaultValues
	{
		TFlow{},
		true,
		5,
		3.0f,
		"My string value",
		418ull,
		TFunction{},
		TDelegate{},
		9
	};
#endif

	//=================================================================
	/// Custom Pin types declaration
	template<typename TTupleList, int EType>
	using TPinType = std::tuple_element_t<EType, TTupleList>;

	// This is how you could declare graph specific pin types for
	// a variety of pin type utilities
	/*
		template<int EType>
		using TMyPinType = TPinType<TMyPinTypesTuple, EType>;

		TMyPinType<EMyPinType::Float> myPin;
	*/

	//=================================================================
	struct Node
	{
		UUID ID;
		std::string Category, Name, Description;
		std::vector<Pin*> Inputs;		// TODO: JP. these Ins and Outs can be assigned by the subclass
		std::vector<Pin*> Outputs;
		ImColor Color;
		NodeType Type;					// TODO: JP. this could also be implmentation specific types
		ImVec2 Size;

		std::string State;
		std::string SavedState;

		int32_t SortIndex;
		static constexpr int32_t UndefinedSortIndex = -1;

		Node(std::string_view name, ImColor color = ImColor(255, 255, 255))
			: ID()
			, Name(name)
			, Color(color)
			, Type(NodeType::Blueprint)
			, Size(0, 0)
			, SortIndex(UndefinedSortIndex)
		{
		}

		Node() = default;

		virtual ~Node()
		{
			for (auto* in : Inputs)
				delete in;

			for (auto* out : Outputs)
				delete out;
		}

		bool operator==(const Node& other) const
		{
			return ID == other.ID
				&& Category == other.Category
				&& Name == other.Name
				&& GetTypeID() == other.GetTypeID();
		}

		// not used in the base graph context yet
		virtual int GetTypeID() const { return TypeID; };

		ImVec2 GetPosition() const;
		void SetPosition(ImVec2 pos);

	protected:
		int TypeID = -1; // Implementation specific type identifier (if required)
	};

	// Example
#if 0
	//=================================================================
	/// Example of custom static Node type declaration
	template<typename TInputsList, typename TOutputsList>
	struct MyNode : Node
	{
		using TInputs   = TInputsList;
		using TOutputs  = TOutputsList;
		using TTopology = MyNode<TInputs, TOutputs>;

		virtual int GetTypeID() const override { return 0; }

		constexpr MyNode(TInputs&& inputs, TOutputs&& outputs)
			: Ins(std::forward<TInputs>(inputs))
			, Outs(std::forward<TOutputs>(outputs))
		{}

		TInputs Ins;
		TOutputs Outs;

		constexpr auto GetIn(size_t index) const { return std::get<index>(Ins); }
		constexpr auto GetOut(size_t index) const { return std::get<index>(Outs); }

		constexpr size_t GetInputCount() const { return std::tuple_size_v<TInputs>; }
		constexpr size_t GetOutputCount() const { return std::tuple_size_v<TOutputs>; }
	};


#define INPUT_LIST(...) __VA_ARGS__
#define OUTPUT_LIST(...) __VA_ARGS__

#define DECLARE_GRAPH_NODE(NodeType, Inputs, Outputs)											\
		struct NodeType : MyNode<decltype(std::tuple(Inputs)), decltype(std::tuple(Outputs))>		\
		{																							\
			using TTopology::TTopology;																\
																									\
			[[nodiscard]] static Node* Construct()													\
			{																						\
				return static_cast<Node*>(new NodeType(std::tuple(Inputs), std::tuple(Outputs)));	\
			}																						\
		};

	DECLARE_GRAPH_NODE(MyNodeType,
		INPUT_LIST(
			TMyPinType<EPinType::Float>(5.0f),
			TMyPinType<EPinType::Int>(2)
		),
		OUTPUT_LIST(
			TMyPinType<EPinType::Int>(3),
			TMyPinType<EPinType::Bool>(true)
		));
#endif

	struct Link
	{
		UUID ID;

		UUID StartPinID;
		UUID EndPinID;

		ImColor Color;

		Link(UUID startPinId, UUID endPinId) :
			ID(), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
		{
		}
	};


	//=================================================================
	/// Node factories

	namespace Nodes {
		using Registry = std::map<std::string, std::map<std::string, std::function<Node* ()>>>;
		using EnumTokensRegistry = std::unordered_map<std::string, const std::vector<Token>*>;

		class AbstractFactory
		{
		public:
			virtual ~AbstractFactory() = default;

			virtual Node* SpawnNode(const std::string& category, const std::string& type) const = 0;
			virtual Pin* CreatePinForType(int type) const = 0;
			virtual ImColor GetIconColor(int pinTypeID) const = 0;
			virtual ImColor GetValueColor(const choc::value::Value& v) const = 0;
			virtual std::pair<int, StorageKind> GetPinTypeAndStorageKindForValue(const choc::value::Value& v) const = 0;
		};

		template<class TDerivedFactory>
		class Factory : public AbstractFactory
		{
		public:
			static const Registry Registry;
			static const EnumTokensRegistry EnumTokensRegistry;

			Pin* CreatePinForType(int type) const override { return TDerivedFactory::Types::CreatePinForType(type); }

			ImColor GetIconColor(int pinTypeID) const override { return GetIconColorStatic(pinTypeID); }
			ImColor GetValueColor(const choc::value::Value& v) const override { return GetValueColorStatic(v); }
			std::pair<int, StorageKind> GetPinTypeAndStorageKindForValue(const choc::value::Value& v) const override { return GetPinTypeAndStorageKindForValueStatic(v); }

			static ImColor GetIconColorStatic(int pinTypeID) { return TDerivedFactory::Types::GetPinColor(pinTypeID); }
			static ImColor GetValueColorStatic(const choc::value::Value& v)
			{
				const auto pinType = TDerivedFactory::Types::GetPinTypeForValue(v);
				return TDerivedFactory::Types::GetPinColor(pinType);
			}

			static std::pair<int, StorageKind> GetPinTypeAndStorageKindForValueStatic(const choc::value::Value& v)
			{
				const auto pinType = TDerivedFactory::Types::GetPinTypeForValue(v);
				const bool isArray = v.isArray();
				return { pinType, isArray ? StorageKind::Array : StorageKind::Value };
			}


			[[nodiscard]] static Node* SpawnNodeStatic(std::string_view category, std::string_view type)
			{
				auto cat = Registry.find(std::string(category));
				if (cat != Registry.end())
				{
					auto nodes = cat->second.find(std::string(type));
					if (nodes != cat->second.end())
					{
						Node* node = nodes->second();
						node->Category = category;
						return node;
					}
					else
					{
						HZ_CORE_ERROR("SpawnNodeStatic() - Category {0} does not contain node type {1}", category, type);
						return nullptr;
					}
				}

				HZ_CORE_ERROR("SpawnNodeStatic() - Category {0} does not exist", category);
				return nullptr;
//				return T::SpawnNodeStatic();
			}


			// TODO: JP. utilize magic_enum
			static const std::vector<Token>* GetEnumTokens(std::string_view nodeName, std::string_view memberName)
			{
				// Passed in names must not contain namespace
				HZ_CORE_ASSERT(nodeName.find("::") == std::string_view::npos);
				HZ_CORE_ASSERT(memberName.find("::") == std::string_view::npos);

				// TODO: for now this is specific to SoundGraph nodes
				const std::string fullName = choc::text::template joinStrings<std::array<std::string_view, 2>>({ Utils::CreateUserFriendlyTypeName(nodeName), Utils::RemovePrefixAndSuffix(memberName) }, "::");
				if (!EnumTokensRegistry.count(fullName))
					return nullptr;

				return EnumTokensRegistry.at(fullName);
			}
		};


		inline std::string SanitizeMemberName(std::string_view name, bool removePrefixAndSuffix = false)
		{
			std::vector<std::string> tokens = Utils::SplitString(name, "::");
			HZ_CORE_ASSERT(tokens.size() >= 2);

			std::array<std::string, 2> validTokens = {
				*(tokens.rbegin() + 1),
				removePrefixAndSuffix ? std::string(Utils::RemovePrefixAndSuffix(*tokens.rbegin())) : *tokens.rbegin()
			};
			return choc::text::joinStrings(validTokens, "::");
		}

	} // namespace Nodes

} // namespace Hazel

#pragma once

#include "Hazel/Editor/NodeGraphEditor/Nodes.h"
#include "Hazel/Editor/NodeGraphEditor/NodeGraphUtils.h"
#include "Hazel/Reflection/TypeDescriptor.h"

#include <choc/containers/choc_Value.h>

#include <tuple>

namespace Hazel::AnimationGraph {

	struct Types final
	{
		enum EPinType : int
		{
			Flow,
			Bool,
			Int,
			Float,
			Vec3,
			AnimationAsset,
			String,
			SkeletonAsset,
			Enum,
			Pose,
			Bone,
		};

		enum ENodeType : int
		{
			None = -1,
			Animation,
			StateMachine,
			State,
			QuickState,
			Transition,
			BlendSpace,
			ConditionalBlend,
			OneShot,
		};

		struct TFlow {};
		struct TEnum { int Underlying = -1; };
		struct TBone { std::string Name; };

		template<int EType, class TValueType, ImU32 Color = IM_COL32_WHITE>
		using PinDescr = Hazel::PinDescr<EType, TValueType, Color, EPinType>;

		using TPinTypes = std::tuple
		<
			PinDescr<EPinType::Flow,           TFlow,       IM_COL32(200, 200, 200, 255)>,
			PinDescr<EPinType::Bool,           bool,        IM_COL32(220, 48, 48, 255)>,
			PinDescr<EPinType::Int,            int,         IM_COL32(68, 201, 156, 255)>,
			PinDescr<EPinType::Float,          float,       IM_COL32(147, 226, 74, 255)>,
			PinDescr<EPinType::Vec3,           glm::vec3,   IM_COL32(251, 199, 34, 255)>,
			PinDescr<EPinType::AnimationAsset, UUID,        IM_COL32(51, 150, 215, 255)>,
			PinDescr<EPinType::String,         std::string, IM_COL32(194, 75, 227, 255)>,
			PinDescr<EPinType::SkeletonAsset,  UUID,        IM_COL32(215, 150, 51, 255)>,
			PinDescr<EPinType::Enum,           TEnum,       IM_COL32(2, 92, 48, 255)>,
			PinDescr<EPinType::Pose,           int64_t,     IM_COL32(255, 255, 0, 255)>,
			PinDescr<EPinType::Bone,           TBone,       IM_COL32(200, 200, 200, 255)>
		>;

		static inline const TPinTypes s_PinDefaultValues
		{
			TFlow{},
			false,
			0,
			0.0f,
			glm::vec3{},
			0,
			"",
			0,
			TEnum{-1},
			0,
			TBone{"root"},
		};

		template<typename TValueType>
		static constexpr auto GetPinForValueType()
		{
			return Type::for_each_tuple(s_PinDefaultValues, [](auto pinDescr)
			{
				if constexpr (std::is_same_v<typename decltype(pinDescr)::value_type, TValueType>)
					return pinDescr;
			});
		}

		template<typename TMemberPtr>
		static constexpr auto ConstructPin(TMemberPtr member) -> decltype(GetPinForValueType<Type::member_pointer::return_type<TMemberPtr>::type>())
		{
			using TValue = typename Type::member_pointer::return_type<TMemberPtr>::type;
			return GetPinForValueType<TValue>();
		}

		template<int EType>
		using TPin = TPinType<TPinTypes, EType>;

		template<int EType, typename TMemberPtr>
		static constexpr auto ConstructPin(TMemberPtr member)
		{
			return TPin<EType>();
		}

		[[nodiscard]] static Pin* CreatePinForType(int type, std::string_view pinName = "")
		{
			Pin* newPin = nullptr;

			Type::for_each_tuple(s_PinDefaultValues, [&newPin, type](auto pinDescr)
			{
				using TDescr = TPin<decltype(pinDescr)::pin_type>;

				if (TDescr::pin_type == type)
					newPin = new TDescr(pinDescr/*.DefaultValue*/);
			});

			if (newPin)
				newPin->Name = pinName;

			return newPin;
		}

		static constexpr ImU32 GetPinColor(int type)
		{
			// Default color
			ImU32 color = IM_COL32(220, 220, 220, 255);

			Type::for_each_tuple(s_PinDefaultValues, [&](auto&& pinDescr) mutable
			{
				using TDescr = typename std::remove_cvref_t<decltype(pinDescr)>;

				if (TDescr::pin_type == type)
					color = TDescr::color;
			});

			return color;
		}

		template<class TNode, typename TMemberPtr>
		static constexpr EPinType GetPinTypeForMember()
		{
			using TMember = typename Type::member_pointer::return_type<TMemberPtr>::type;
			using TMemberRaw = std::remove_pointer_t<TMember>;

			//? DBG
			std::string_view tmp = typeid(TMemberPtr).name();
			std::string_view tm = typeid(TMember).name();
			std::string_view name = typeid(TMemberRaw).name();

			EPinType type = EPinType::Flow;

			if (std::is_same_v<TMemberRaw, float>)
				type = EPinType::Float;
			else if (std::is_same_v<TMemberRaw, int32_t>)
				type = EPinType::Int;
			else if (std::is_same_v<TMemberRaw, int64_t>)
				type = EPinType::AnimationAsset;
			else if (std::is_same_v<TMemberRaw, bool>)
				type = EPinType::Bool;
			else if (std::is_same_v<TMemberRaw, glm::vec3>)
				type = EPinType::Vec3;
			else if (std::is_same_v<TMemberRaw, std::string>)
				type = EPinType::String;

			return type;
		}

		static EPinType GetPinTypeForValue(const choc::value::ValueView value)
		{
			EPinType pinType = EPinType::Flow;

			choc::value::Type type = value.isArray() ? value.getType().getElementType() : value.getType();

			if (type.isObject())
			{
				if (type.isObjectWithClassName(type::type_name<TFlow>()))
				{
					HZ_CORE_ASSERT(false);
					pinType = EPinType::Flow;
				}
				else if (Utils::IsAssetHandle<AssetType::Animation>(type))
				{
					pinType = EPinType::AnimationAsset;
				}
				else if (Utils::IsAssetHandle<AssetType::Skeleton>(type))
				{
					pinType = EPinType::SkeletonAsset;
				}
				else if(type.isObjectWithClassName(type::type_name<glm::vec3>()))
				{
					pinType = EPinType::Vec3;
				}
				else if (value.hasObjectMember("Type"))
				{
					HZ_CORE_WARN("Found pin with custom type {}, need to handle it!", value["Type"].getString());
				}
			}
			else if (type.isFloat())  pinType = EPinType::Float;
			else if (type.isInt32())  pinType = EPinType::Int;
			else if (type.isBool())   pinType = EPinType::Bool;
			else if (type.isString()) pinType = EPinType::String;
			else if (type.isVoid())   pinType = EPinType::Flow;	// Trigger
			else HZ_CORE_ASSERT(false);

			return pinType;
		}


		// TODO: extend with Implementation interface
		//=================================================================
		//template</*typename TGraph, */typename TInputsList, typename TOutputsList>
		struct Node : Hazel::Node
		{
			using Hazel::Node::Node;

			bool IsEntryState = false;

			int GetTypeID() const override {
				if (TypeID == ENodeType::None)
				{
					// compute type id.  Cannot be done in constructor since we do not know node category there.
					if (Category == "StateMachine")
					{
						if      (Name == "State Machine")  TypeID = ENodeType::StateMachine;
						else if (Name == "State")          TypeID = ENodeType::State;
						else if (Name == "Quick State")    TypeID = ENodeType::QuickState;
						else if (Name == "Transition")     TypeID = ENodeType::Transition;
						else                               HZ_CORE_ASSERT(false, "Unknown node type");
						
					}
					else if (Name == "State Machine")      TypeID = ENodeType::StateMachine;
					else if (Name == "Blend Space")        TypeID = ENodeType::BlendSpace;
					else if (Name == "Conditional Blend")  TypeID = ENodeType::ConditionalBlend;
					else if (Name == "One Shot")           TypeID = ENodeType::OneShot;
					else                                   TypeID = ENodeType::Animation;
				}
				return TypeID;
			}

		ImVec2 Offset = { 0.0f, 0.0f };
		ImVec2 Scale = { 1.0f, -1.0f };
		ImVec2 LerpSecondsPerUnit = { 0.2f, 0.2f };
		bool ShowDetails = false;

		protected:
			mutable int TypeID = ENodeType::None;
		};

	};

} // namespace Hazel::Nodes


namespace Hazel {

	template<>
	choc::value::Value ValueFrom(const AnimationGraph::Types::TFlow& obj)
	{
		return choc::value::createObject(type::type_name<AnimationGraph::Types::TFlow>());
	}

	template<>
	std::optional<AnimationGraph::Types::TFlow> CustomValueTo<AnimationGraph::Types::TFlow>(choc::value::ValueView customValueObject)
	{
		if (customValueObject.isObjectWithClassName(type::type_name<AnimationGraph::Types::TFlow>()))
			return AnimationGraph::Types::TFlow();
		else
			return {};
	}

	template<>
	choc::value::Value ValueFrom(const AnimationGraph::Types::TEnum& obj)
	{
		return choc::value::createObject(type::type_name<AnimationGraph::Types::TEnum>(), "Value", obj.Underlying);
	}

	template<>
	std::optional<AnimationGraph::Types::TEnum> CustomValueTo<AnimationGraph::Types::TEnum>(choc::value::ValueView customValueObject)
	{
		if (customValueObject.isObjectWithClassName(type::type_name<AnimationGraph::Types::TEnum>()))
			return AnimationGraph::Types::TEnum{ customValueObject["Value"].get<int>() };
		else
			return {};
	}

	template<>
	choc::value::Value ValueFrom(const AnimationGraph::Types::TBone& obj)
	{
		return choc::value::createObject(type::type_name<AnimationGraph::Types::TBone>(), "Value", obj.Name);
	}

	template<>
	std::optional<AnimationGraph::Types::TBone> CustomValueTo<AnimationGraph::Types::TBone>(choc::value::ValueView customValueObject)
	{
		if (customValueObject.isObjectWithClassName(type::type_name<AnimationGraph::Types::TBone>()))
			return AnimationGraph::Types::TBone{ customValueObject["Value"].get<std::string>() };
		else
			return {};
	}

} // namespace Hazel

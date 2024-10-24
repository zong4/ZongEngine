#pragma once

#include "Engine/Editor/NodeGraphEditor/Nodes.h"
#include "Engine/Editor/NodeGraphEditor/NodeGraphUtils.h"
#include "Engine/Reflection/TypeDescriptor.h"

#include <choc/containers/choc_Value.h>

#include <tuple>

namespace Engine::SoundGraph {

	class SGTypes final
	{
	public:
		enum ESGPinType : int
		{
			Flow,
			Bool,
			Int,
			Float,
			Audio,		// audio output
			String,
			AudioAsset,	// audio asset (such as wave file)
			Enum
		};

		// TODO: consider removing TSG prefix, SGTypes namespace might be safe enough
		struct TSGFlow {};
		struct TSGAudio { const float Underlying = 0.0f; };
		struct TSGEnum { int Underlying = -1; };

		template<int EType, class TValueType, ImU32 Color = IM_COL32_WHITE>
		using SGPinDescr = Engine::PinDescr<EType, TValueType, Color, ESGPinType>;

		using TSGPinTypes = std::tuple
			<
			SGPinDescr<ESGPinType::Flow,		TSGFlow,		IM_COL32(200, 200, 200, 255)>,
			SGPinDescr<ESGPinType::Bool,		bool,			IM_COL32(220, 48, 48, 255)>,
			SGPinDescr<ESGPinType::Int,			int,			IM_COL32(68, 201, 156, 255)>,
			SGPinDescr<ESGPinType::Float,		float,			IM_COL32(147, 226, 74, 255)>,
			SGPinDescr<ESGPinType::Audio,		TSGAudio,		IM_COL32(102, 204, 163, 255)>,
			SGPinDescr<ESGPinType::String,		std::string,	IM_COL32(194, 75, 227, 255)>,
			SGPinDescr<ESGPinType::AudioAsset,	UUID,			IM_COL32(51, 150, 215, 255)>,
			SGPinDescr<ESGPinType::Enum,		TSGEnum,		IM_COL32(2, 92, 48, 255)>
			>;

		static inline const TSGPinTypes s_SGPinDefaultValues
		{
			TSGFlow{},
			false,
			0,
			0.0f,
			TSGAudio{0.0f},
			"",
			0,
			TSGEnum{-1}
		};

		// This is how to declare graph specific pin types
		template<int EType>
		using TPin = TPinType<TSGPinTypes, EType>;

		template<typename TValueType>
		static constexpr auto GetPinForValueType()
		{
			return Type::for_each_tuple(s_SGPinDefaultValues, [](auto pinDescr)
			{
				if constexpr (std::is_same_v<typename decltype(pinDescr)::value_type, TValueType>)
					return pinDescr;
			});
		}

		template<typename T>
		static constexpr auto ConstructPin(T);

		template<typename TMemberPtr>
		static constexpr auto ConstructPin(TMemberPtr member) -> decltype(GetPinForValueType<Type::member_pointer::return_type<TMemberPtr>::type>())
		{
			using TValue = typename Type::member_pointer::return_type<TMemberPtr>::type;
			return GetPinForValueType<TValue>();
		}

		template<int EType, typename TMemberPtr>
		static constexpr auto ConstructPin(TMemberPtr member)
		{
			return TPin<EType>();
		}

		[[nodiscard]] static Pin* CreatePinForType(int type, std::string_view pinName = "")
		{
			Pin* newPin = nullptr;

			Type::for_each_tuple(s_SGPinDefaultValues, [&newPin, type](auto pinDescr)
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

			Type::for_each_tuple(s_SGPinDefaultValues,
					[&](auto&& pinDescr) mutable
					{
						using TDescr = typename std::remove_cvref_t<decltype(pinDescr)>;

						if (TDescr::pin_type == type)
							color = TDescr::color;
					});

			return color;
		}

		template<class TNode, typename TMemberPtr>
		static constexpr ESGPinType GetPinTypeForMember()
		{
			using TMember = typename Type::member_pointer::return_type<TMemberPtr>::type;
			using TMemberRaw = std::remove_pointer_t<TMember>;

			//? DBG
			std::string_view tmp = typeid(TMemberPtr).name();
			std::string_view tm = typeid(TMember).name();
			std::string_view name = typeid(TMemberRaw).name();

			ESGPinType type = ESGPinType::Flow;

			if (std::is_same_v<TMemberRaw, float>)
				type = ESGPinType::Float;
			else if (std::is_same_v<TMemberRaw, int32_t>)
				type = ESGPinType::Int;
			else if (std::is_same_v<TMemberRaw, int64_t>)
				type = ESGPinType::AudioAsset;
			else if (std::is_same_v<TMemberRaw, bool>)
				type = ESGPinType::Bool;

			return type;
		}

		static ESGPinType GetPinTypeForValue(const choc::value::ValueView value)
		{
			ESGPinType pinType = ESGPinType::Flow;

			choc::value::Type type = value.isArray() ? value.getType().getElementType() : value.getType();

			if (type.isObject())
			{
				if (type.isObjectWithClassName(type::type_name<TSGFlow>()))
				{
					pinType = ESGPinType::Flow;
				}
				else if (Engine::Utils::IsAssetHandle<AssetType::Audio>(type))
				{
					pinType = ESGPinType::AudioAsset;
				}
				else if (value.hasObjectMember("Type"))
				{
					ZONG_CORE_WARN("Found pin with custom type {}, need to handle it!", value["Type"].getString());
				}
			}
			else if (type.isFloat())  pinType = ESGPinType::Float;
			else if (type.isInt32())  pinType = ESGPinType::Int;
			else if (type.isBool())   pinType = ESGPinType::Bool;
			else if (type.isString()) pinType = ESGPinType::String;
			else if (type.isVoid())	  pinType = ESGPinType::Flow;	// Trigger
			else if (type.isInt64())
			{
				pinType = ESGPinType::AudioAsset;
				ZONG_CORE_ASSERT(false, "Shouldn't be using Int64 Value for AudioAsset type in SoundGraph.");
			}
			else ZONG_CORE_ASSERT(false);

			return pinType;
		}

		// TODO: extend with Implementation interface
		//=================================================================
		//template</*typename TGraph, */typename TInputsList, typename TOutputsList>
		struct SGNode : Node
		{
			using Node::Node;
			//! if constexpr
#if 0
			using TInputs = TInputsList;
			using TOutputs = TOutputsList;
			using TTopology = MyNode<TInputs, TOutputs>;

			constexpr SGNode(TInputs&& inputs, TOutputs&& outputs)
				: Ins(std::forward<TInputs>(inputs))
				, Outs(std::forward<TOutputs>(outputs))
			{
			}

			TInputs Ins;
			TOutputs Outs;

			constexpr auto GetIn(size_t index) const { return std::get<index>(Ins); }
			constexpr auto GetOut(size_t index) const { return std::get<index>(Outs); }

			constexpr size_t GetInputCount() const { return std::tuple_size_v<TInputs>; }
			constexpr size_t GetOutputCount() const { return std::tuple_size_v<TOutputs>; }
#endif
			virtual int GetTypeID() const override { return 0; }

		};
	};


} // namespace Engine::Nodes

namespace Engine {

	template<>
	choc::value::Value ValueFrom(const SoundGraph::SGTypes::TSGFlow& obj)
	{
		return choc::value::createObject(type::type_name<SoundGraph::SGTypes::TSGFlow>());
	}

	template<>
	std::optional<SoundGraph::SGTypes::TSGFlow> CustomValueTo<SoundGraph::SGTypes::TSGFlow>(choc::value::ValueView customValueObject)
	{
		if (customValueObject.isObjectWithClassName(type::type_name<SoundGraph::SGTypes::TSGFlow>()))
			return SoundGraph::SGTypes::TSGFlow();
		else
			return {};
	}

	template<>
	choc::value::Value ValueFrom(const SoundGraph::SGTypes::TSGEnum& obj)
	{
		return choc::value::createObject(type::type_name<SoundGraph::SGTypes::TSGEnum>(), "Value", obj.Underlying);
	}

	template<>
	std::optional<SoundGraph::SGTypes::TSGEnum> CustomValueTo<SoundGraph::SGTypes::TSGEnum>(choc::value::ValueView customValueObject)
	{
		if (customValueObject.isObjectWithClassName(type::type_name<SoundGraph::SGTypes::TSGEnum>()))
			return SoundGraph::SGTypes::TSGEnum{ customValueObject["Value"].get<int>() };
		else
			return {};
	}

} // namespace Engine

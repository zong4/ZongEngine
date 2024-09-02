#pragma once

#include "Hazel/Audio/AudioCallback.h"
#include "Hazel/Core/Identifier.h"
#include "Hazel/Core/UUID.h"
#include "containers/choc_Value.h"

#include <string>
#include <functional>
#include <complex>
#include <type_traits>

#include <glm/gtc/constants.hpp>
#include <glm/glm.hpp>

#define LOG_DBG_MESSAGES 0

#if LOG_DBG_MESSAGES
#define DBG(...) HZ_CORE_WARN(__VA_ARGS__)
#else
#define DBG(...)
#endif

namespace Hazel::SoundGraph
{
	struct StreamWriter;

#if 0//! Pulling inputs API (not used, slow on performance)

	struct ValueGetterBase
	{
		virtual ~ValueGetterBase() = default;
	};

	template<typename T>	/// Get this type T when parsing, if pointer - inpout, non-pointer - output
	struct ValueGetter : public ValueGetterBase
	{
		virtual ~ValueGetter() = default;
		virtual T Get() = 0;
	};

	template<typename T>
	struct Constant : public ValueGetter<T>
	{
		Constant(T value) : Value(value) {}

		T Value;
		T Get() final
		{
			return Value;
		}

		inline operator T() const { return Value; }
	};

	template<typename T>
	struct ValueCb : public ValueGetter<T>
	{
		T* in_Value = nullptr;	/// named input
		T Get() final			/// named output
		{
			return operator T();
		}

		inline operator T() const { return *in_Value; }
	};

	template<typename T>
	struct SubtractCb : public ValueGetter<T>
	{
		ValueGetter<T>* in_Value1_Raw = nullptr;
		ValueGetter<T>* in_Value2_Raw = nullptr;

		T Get() final { return operator T(); }

		inline operator T() const { return in_Value1_Raw->Get() - in_Value2_Raw->Get(); }
	};

	template<typename T>
	struct MultiplyCb : public ValueGetter<T>
	{
		ValueGetter<T>* in_Value = nullptr;
		ValueGetter<T>* in_Multiplier = nullptr;

		T Get() final { return operator T(); }

		inline operator T() const { return in_Value->Get() * in_Multiplier->Get(); }
	};
#endif

	//==============================================================================
	/// NodeProcessor
	struct NodeProcessor
	{
		explicit NodeProcessor(const char* dbgName, UUID id) noexcept
			: dbgName(dbgName), ID(id)
		{
		}

		std::string dbgName;
		UUID ID;

		//? this base struct is unnecessary
		struct Input
		{
			Input() { throw std::invalid_argument("Must not construct Input using default constuctor"); }
			explicit Input(NodeProcessor& owner) noexcept : node(&owner) {}
			NodeProcessor* node = nullptr;
		};

		//? this base struct is unnecessary
		struct Output
		{
			Output() { throw std::invalid_argument("Must not construct Output using default constuctor"); }
			Output(NodeProcessor& owner) noexcept : node(&owner) {}
			NodeProcessor* node = nullptr;
		};

		struct InputEvent : public Input
		{
			using TEvent = std::function<void(float)>;

			explicit InputEvent(NodeProcessor& owner, TEvent ev) noexcept
				: Input(owner), Event(std::move(ev))
			{
			}

			inline virtual void operator()(float value) noexcept // TODO: value type?
			{
				Event(value);
			}

			// Should be bound to NodeProcessor member method
			TEvent Event;
		};

		struct OutputEvent : public Output
		{
			explicit OutputEvent(NodeProcessor& owner) noexcept
				: Output(owner)
			{
			}

			inline void operator()(float value) noexcept
			{
				for (auto& out : DestinationEvs)
					(*out)(value);
			}

			void AddDestination(const std::shared_ptr<InputEvent>& dest)
			{
				DestinationEvs.push_back(dest);
			}

			// OutputEvent should always have destination Input to call
			std::vector<std::shared_ptr<InputEvent>> DestinationEvs;
			//std::vector<std::function<void(float)>> DestinationEvs;
		};


		InputEvent& AddInEvent( /* Type */ Identifier id, InputEvent::TEvent function = nullptr)
		{
			const auto& [element, inserted] = InEvs.try_emplace(id, *this, function);
			HZ_CORE_ASSERT(inserted);
			return element->second;
		}

		void AddOutEvent(/* Type */ Identifier id, OutputEvent& out)
		{
			const auto& [element, inserted] = OutEvs.insert({ id, out });
			HZ_CORE_ASSERT(inserted);
		}

		choc::value::ValueView& AddInStream(/* Type */Identifier id, choc::value::ValueView* source = nullptr)
		{
			const auto& [element, inserted] = Ins.try_emplace(id);
			if (source)
				element->second = *source;

			HZ_CORE_ASSERT(inserted);
			return element->second;
		}

		template<typename T>
		choc::value::ValueView& AddOutStream(/* Type */ Identifier id, T& memberVariable)
		{
			// TODO: add overload to refer to "external" value (node's member variable)
			const auto& [element, inserted] = Outs.try_emplace(id,
															   choc::value::ValueView(choc::value::Type::createPrimitive<T>(),
																					  &memberVariable,
																					  nullptr));
			
			HZ_CORE_ASSERT(inserted);
			return element->second;
		}

		virtual void Init() {}
		virtual void Process() {}

		std::unordered_map<Identifier, InputEvent> InEvs;
		std::unordered_map<Identifier, OutputEvent&> OutEvs;

		std::unordered_map<Identifier, choc::value::ValueView> Ins;
		std::unordered_map<Identifier, choc::value::ValueView> Outs;

		//? TEMP. If nothing is connected to an input, need to initialize it with owned default value,
		//?		 storing them like this for now
		std::vector<std::shared_ptr<StreamWriter>> DefaultValuePlugs;


		inline choc::value::ValueView& InValue(const Identifier& id) { return Ins.at(id); }
		inline choc::value::ValueView& OutValue(const Identifier& id) { return Outs.at(id); }
		inline InputEvent& InEvent(const Identifier& id) { return InEvs.at(id); }
		inline OutputEvent& OutEvent(const Identifier& id) { return OutEvs.at(id); }
	};

	//==============================================================================
	/// StreamWriter
	struct StreamWriter : public NodeProcessor
	{
		template<typename T>
		explicit StreamWriter(choc::value::ValueView& destination, T&& externalObjectOrDefaultValue, Identifier destinationID, UUID id = UUID()) noexcept
			: NodeProcessor("Stream Writer", id)
			, DestinationV(destination)
			, outV(std::move(externalObjectOrDefaultValue))
			, DestinationID(destinationID)
		{
			DestinationV = outV; //? does this make `destination` constructor argument unnecessary?
		}

		inline void operator<<(float value) noexcept
		{
			*(float*)outV.getRawData() = value;
		}

		template<typename T>
		inline void operator<<(T value) noexcept
		{
			*(T*)outV.getRawData() = value;
		}

		// TODO: Stream Writer if used to write from Graph's Input can refer to an external variable.
		//		This would be useful to have a direct connection between the Graph and a wrapper object.
		//		Also passing Graph Input Arrays and other heavy types by copy is not very efficient
		//std::shared_ptr<OutputValue> out;

		Identifier DestinationID;
		choc::value::Value outV;
		choc::value::ValueView& DestinationV;
	};

	template<>
	inline void StreamWriter::operator<<(const choc::value::ValueView& value) noexcept
	{
		outV = value;
	}

	template<>
	inline void StreamWriter::operator<<(const choc::value::ValueView value) noexcept
	{
		outV = value;
	}

} // namespace Hazel::SoundGraph

#undef LOG_DBG_MESSAGES
#undef DBG

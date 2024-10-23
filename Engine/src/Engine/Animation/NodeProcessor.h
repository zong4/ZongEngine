#pragma once
#include "Animation.h"

#include "Engine/Core/Identifier.h"
#include "Engine/Core/UUID.h"

#include "choc/containers/choc_Value.h"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <string_view>

namespace Hazel::AnimationGraph {

	struct StreamWriter;

	struct NodeProcessor
	{

		inline static choc::value::Type Vec3Type = choc::value::Type::createArray<float>(3);
		inline static choc::value::Type QuatType = choc::value::Type::createArray<float>(4);

		inline static choc::value::Type TransformType = [] {
			choc::value::Type type = choc::value::Type::createArray(Vec3Type, 1);
			type.addArrayElements(QuatType, 1);
			type.addArrayElements(Vec3Type, 1);
			return type;
		}();

		inline static choc::value::Value IdentityTransform = [] {
			choc::value::Value transform(TransformType);
			*reinterpret_cast<glm::vec3*>(transform[0].getRawData()) = glm::vec3{ 0.0f, 0.0f, 0.0f };
			*reinterpret_cast<glm::quat*>(transform[1].getRawData()) = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
			*reinterpret_cast<glm::vec3*>(transform[2].getRawData()) = glm::vec3{ 1.0f, 1.0f, 1.0f };
			return transform;
		}();

		inline static choc::value::Type PoseType = [] {
			choc::value::Type type = choc::value::Type::createArray(TransformType, Hazel::Animation::MAXBONES + 1); // RootMotion, BoneTransforms
			type.addArrayElements(choc::value::Type::createFloat32(), 2); // AnimationDuration, AnimationTime
			type.addArrayElements(choc::value::Type::createInt32(), 1);   // NumBones
			return type;
		}();

		explicit NodeProcessor(std::string_view dbgName, UUID id) noexcept
			: dbgName(dbgName), ID(id)
		{
		}

		std::string dbgName;
		UUID ID;

		struct InputEvent
		{
			using TEvent = std::function<void(Identifier)>;

			explicit InputEvent(TEvent ev) noexcept : Event(std::move(ev)) {}

			virtual void operator()(Identifier eventID) noexcept // TODO: event values?
			{
				Event(eventID);
			}

			// Should be bound to NodeProcessor member method
			TEvent Event;
		};

		struct OutputEvent
		{
			void operator()(Identifier eventID) noexcept
			{
				for (auto& out : DestinationEvs)
					(*out)(eventID);
			}

			void AddDestination(const std::shared_ptr<InputEvent>& dest)
			{
				DestinationEvs.push_back(dest);
			}

			// OutputEvent should always have destination Input to call
			std::vector<std::shared_ptr<InputEvent>> DestinationEvs;
			//std::vector<std::function<void(float)>> DestinationEvs;
		};


		InputEvent& AddInEvent(Identifier id, InputEvent::TEvent function = nullptr)
		{
			const auto& [element, inserted] = InEvs.try_emplace(id, function);
			ZONG_CORE_ASSERT(inserted);
			return element->second;
		}

		void AddOutEvent(/* Type */ Identifier id, OutputEvent& out)
		{
			const auto& [element, inserted] = OutEvs.insert({ id, out });
			ZONG_CORE_ASSERT(inserted);
		}

		choc::value::ValueView& AddInStream(Identifier id, choc::value::ValueView* source = nullptr)
		{
			const auto& [element, inserted] = Ins.try_emplace(id);
			if (source)
				element->second = *source;

			ZONG_CORE_ASSERT(inserted);
			return element->second;
		}

		template<typename T>
		choc::value::ValueView& AddOutStream(Identifier id, T& memberVariable)
		{
			if constexpr (std::is_same_v<std::remove_const_t<T>, choc::value::Value> || std::is_same_v<std::remove_const_t<T>, choc::value::ValueView>)
			{
				const auto& [element, inserted] = Outs.try_emplace(id, memberVariable);
				ZONG_CORE_ASSERT(inserted);
				return element->second;
			}
			else
			{
				const auto& [element, inserted] = Outs.try_emplace(
					id,
					choc::value::ValueView(choc::value::Type::createPrimitive<T>(), &memberVariable, nullptr)
				);

				ZONG_CORE_ASSERT(inserted);
				return element->second;
			}
		}

		virtual void Init() {}
		virtual float Process(float timestep) { return 0.0; }

		// duration in seconds
		virtual float GetAnimationDuration() const { return 0.0f; }

		// 0 = beginning of animation, 1 = end of animation
		virtual float GetAnimationTimePos() const { return 0.0f; }
		virtual void SetAnimationTimePos(float timePos) {};

		std::unordered_map<Identifier, InputEvent> InEvs;
		std::unordered_map<Identifier, OutputEvent&> OutEvs;

		std::unordered_map<Identifier, choc::value::ValueView> Ins;
		std::unordered_map<Identifier, choc::value::ValueView> Outs;

		// Note (0x): If the NodeProcessor implementation is changed after graphs have been created,
		//            (e.g. somewhere down the track you decide to add a new input to the node processor)
		//            then it could be the case that the NodeProcessor has an input for which there
		//            is no DefaultValuePlug.
		//            This will then cause graph validation to fail, or worse, crash Hazel with a null ptr deference.
		//            This can be avoided if NodeProcessor inputs are always set to some non-null value.
		std::vector<std::shared_ptr<StreamWriter>> DefaultValuePlugs;


		inline choc::value::ValueView& InValue(Identifier id) { return Ins.at(id); }
		inline const choc::value::ValueView& InValue(Identifier id) const { return Ins.at(id); }
		inline choc::value::ValueView& OutValue(Identifier id) { return Outs.at(id); }
		inline const choc::value::ValueView& OutValue(Identifier id) const { return Outs.at(id); }
		inline InputEvent& InEvent(Identifier id) { return InEvs.at(id); }
		inline OutputEvent& OutEvent(Identifier id) { return OutEvs.at(id); }
	};


	struct StreamWriter : public NodeProcessor
	{
		template<typename T>
		explicit StreamWriter(T&& externalObjectOrDefaultValue, Identifier destinationID, UUID id = UUID()) noexcept
			: NodeProcessor("Stream Writer", id)
			, outV(std::move(externalObjectOrDefaultValue))
			, DestinationID(destinationID)
		{
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

		template<>
		inline void operator<<(const choc::value::ValueView& value) noexcept
		{
			outV = value;
		}

		template<>
		inline void operator<<(const choc::value::ValueView value) noexcept
		{
			outV = value;
		}

		// TODO: Stream Writer if used to write from Graph's Input can refer to an external variable.
		//		This would be useful to have a direct connection between the Graph and a wrapper object.
		//		Also passing Graph Input Arrays and other heavy types by copy is not very efficient
		//std::shared_ptr<OutputValue> out;

		Identifier DestinationID;
		choc::value::Value outV;
	};

} // namespace Hazel::AnimationGraph

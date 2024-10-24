#pragma once

#include "Event.h"

#include <sstream>

namespace Engine {

	class EditorExitPlayModeEvent : public Event
	{
	public:
		EditorExitPlayModeEvent() = default;

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "EditorExitPlayModeEvent";
			return ss.str();
		}

		EVENT_CLASS_TYPE(EditorExitPlayMode)
		EVENT_CLASS_CATEGORY(EventCategoryEditor)
	};

	class AnimationGraphCompiledEvent : public Event
	{
	public:
		AnimationGraphCompiledEvent(AssetHandle animationGraphHandle) : AnimationGraphHandle(animationGraphHandle) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "AnimationGraphCompiledEvent";
			return ss.str();
		}

		EVENT_CLASS_TYPE(AnimationGraphCompiled)
			EVENT_CLASS_CATEGORY(EventCategoryEditor)

	public:
		AssetHandle AnimationGraphHandle;
	};

}

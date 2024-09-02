#pragma once

#include "Event.h"

#include <sstream>

namespace Hazel {

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

	class AssetReloadedEvent : public Event
	{
	public:
		AssetReloadedEvent(AssetHandle assetHandle) : AssetHandle(assetHandle) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "AssetReloadedEvent";
			return ss.str();
		}

		EVENT_CLASS_TYPE(AssetReloaded)
			EVENT_CLASS_CATEGORY(EventCategoryEditor)

	public:
		AssetHandle AssetHandle;
	};

}

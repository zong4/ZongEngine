#pragma once

#include "Event.h"

namespace zong
{
namespace core
{

class WindowResizeEvent : public Event
{
private:
    unsigned int _width, _height;

public:
    unsigned int width() const { return _width; }
    unsigned int height() const { return _height; }

public:
    WindowResizeEvent(unsigned int width, unsigned int height) : Event(), _width(width), _height(height) {}

    EVENT_CLASS_TYPE(WindowResize)
    EVENT_CLASS_CATEGORY(EventCategory::EventCategoryApplication)

#ifdef DEBUG
    inline std::string toString() const override
    {
        return "WindowResizeEvent: " + std::to_string(width()) + ", " + std::to_string(height());
    }
#endif
};

class WindowCloseEvent : public Event
{
public:
    WindowCloseEvent() : Event() {}

    EVENT_CLASS_TYPE(WindowClose)
    EVENT_CLASS_CATEGORY(EventCategory::EventCategoryApplication)
};

class AppTickEvent : public Event
{
public:
    AppTickEvent() : Event() {}

    EVENT_CLASS_TYPE(AppTick)
    EVENT_CLASS_CATEGORY(EventCategory::EventCategoryApplication)
};

class AppUpdateEvent : public Event
{
public:
    AppUpdateEvent() : Event() {}

    EVENT_CLASS_TYPE(AppUpdate)
    EVENT_CLASS_CATEGORY(EventCategory::EventCategoryApplication)
};

class AppRenderEvent : public Event
{
public:
    AppRenderEvent() : Event() {}

    EVENT_CLASS_TYPE(AppRender)
    EVENT_CLASS_CATEGORY(EventCategory::EventCategoryApplication)
};

} // namespace core
} // namespace zong
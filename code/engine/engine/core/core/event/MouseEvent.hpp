#pragma once

#include "Event.h"
#include "MouseCodes.h"

namespace zong
{
namespace core
{

class MouseMovedEvent : public Event
{
private:
    float _mouseX, _mouseY;

public:
    MouseMovedEvent(float const x, float const y) : Event(), _mouseX(x), _mouseY(y) {}

    inline float x() const { return _mouseX; }
    inline float y() const { return _mouseY; }

    EVENT_CLASS_TYPE(MouseMoved)
    EVENT_CLASS_CATEGORY(static_cast<EventCategory>(EventCategory::EventCategoryMouse | EventCategory::EventCategoryInput))

#ifdef DEBUG
    inline std::string toString() const override { return "MouseMovedEvent: " + std::to_string(x()) + ", " + std::to_string(y()); }
#endif
};

class MouseScrolledEvent : public Event
{
private:
    float _xOffset, _yOffset;

public:
    inline float xOffset() const { return _xOffset; }
    inline float yOffset() const { return _yOffset; }

public:
    MouseScrolledEvent(float const xOffset, float const yOffset) : Event(), _xOffset(xOffset), _yOffset(yOffset) {}

    EVENT_CLASS_TYPE(MouseScrolled)
    EVENT_CLASS_CATEGORY(static_cast<EventCategory>(EventCategory::EventCategoryMouse | EventCategory::EventCategoryInput))

#ifdef DEBUG
    inline std::string toString() const override
    {
        return "MouseScrolledEvent: " + std::to_string(xOffset()) + ", " + std::to_string(yOffset());
    }
#endif
};

class MouseButtonEvent : public Event
{
private:
    MouseCode _button;

public:
    inline MouseCode button() const { return _button; }

protected:
    MouseButtonEvent(const MouseCode button) : Event(), _button(button) {}

public:
    EVENT_CLASS_CATEGORY(static_cast<EventCategory>(EventCategory::EventCategoryMouse |
                                                    (EventCategory::EventCategoryInput | EventCategory::EventCategoryMouseButton)))
};

class MouseButtonPressedEvent : public MouseButtonEvent
{
public:
    MouseButtonPressedEvent(MouseCode const button) : MouseButtonEvent(button) {}

    EVENT_CLASS_TYPE(MouseButtonPressed)

#ifdef DEBUG
    inline std::string toString() const override { return "MouseButtonPressedEvent: " + std::to_string(static_cast<int>(button())); }
#endif
};

class MouseButtonReleasedEvent : public MouseButtonEvent
{
public:
    MouseButtonReleasedEvent(MouseCode const button) : MouseButtonEvent(button) {}

    EVENT_CLASS_TYPE(MouseButtonReleased)

#ifdef DEBUG
    inline std::string toString() const override { return "MouseButtonReleasedEvent: " + std::to_string(static_cast<int>(button())); }
#endif
};

} // namespace core
} // namespace zong

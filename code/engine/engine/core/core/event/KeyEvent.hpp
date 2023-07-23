#pragma once

#include "Event.h"
#include "KeyCodes.h"

namespace zong
{
namespace core
{

class KeyEvent : public Event
{
private:
    KeyCode _keyCode;

public:
    inline KeyCode keyCode() const { return _keyCode; }

protected:
    KeyEvent(KeyCode const keycode) : Event(), _keyCode(keycode) {}

public:
    EVENT_CLASS_CATEGORY(static_cast<EventCategory>(EventCategory::EventCategoryKeyboard | EventCategory::EventCategoryInput))
};

class KeyPressedEvent : public KeyEvent
{
private:
    bool _isRepeat;

public:
    bool isRepeat() const { return _isRepeat; }

public:
    KeyPressedEvent(KeyCode const keycode, bool isRepeat = false) : KeyEvent(keycode), _isRepeat(isRepeat) {}

    EVENT_CLASS_TYPE(KeyPressed)

#ifdef DEBUG
    inline std::string toString() const override
    {
        return "KeyPressedEvent: " + std::to_string(static_cast<int>(keyCode())) + " (repeat = " + std::to_string(isRepeat()) + ")";
    }
#endif
};

class KeyReleasedEvent : public KeyEvent
{
public:
    KeyReleasedEvent(KeyCode const keycode) : KeyEvent(keycode) {}

    EVENT_CLASS_TYPE(KeyReleased)

#ifdef DEBUG
    inline std::string toString() const override { return "KeyReleasedEvent: " + std::to_string(static_cast<int>(keyCode())); }
#endif
};

class KeyTypedEvent : public KeyEvent
{
public:
    KeyTypedEvent(KeyCode const keycode) : KeyEvent(keycode) {}

    EVENT_CLASS_TYPE(KeyTyped)

#ifdef DEBUG
    inline std::string toString() const override { return "KeyTypedEvent: " + std::to_string(static_cast<int>(keyCode())); }
#endif
};

} // namespace core
} // namespace zong

#pragma once

#include "EventCategory.hpp"
#include "EventType.hpp"
#include "core/pch.hpp"

namespace zong
{
namespace core
{

class Event
{
private:
    bool _handled;

public:
    inline bool getHandled() const { return _handled; }

    inline void setHandled(bool const value) { _handled = value; }

    virtual EventType     getEventType() const    = 0;
    virtual std::string   getName() const         = 0;
    virtual EventCategory getCategoryFlag() const = 0;

public:
    Event() : _handled(false) {}
    virtual ~Event() = default;

    virtual bool isInCategory(EventCategory const category) const { return getCategoryFlag() & category; }

#ifdef DEBUG
    virtual std::string toString() const { return "EventName: " + getName(); }
#endif
};

} // namespace core
} // namespace zong

#define EVENT_CLASS_TYPE(type)                      \
    static EventType getStaticType()                \
    {                                               \
        return EventType::type;                     \
    }                                               \
    virtual EventType getEventType() const override \
    {                                               \
        return getStaticType();                     \
    }                                               \
    virtual std::string getName() const override    \
    {                                               \
        return #type;                               \
    }

#define EVENT_CLASS_CATEGORY(category)                     \
    virtual EventCategory getCategoryFlag() const override \
    {                                                      \
        return category;                                   \
    }

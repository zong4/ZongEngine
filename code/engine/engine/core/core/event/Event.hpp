#pragma once

#include "EventCategory.hpp"
#include "EventType.hpp"

namespace zong
{
namespace core
{

class Event
{
private:
    bool _handled;

public:
    inline bool handled() const { return _handled; }

    inline void setHandled(bool const value) { _handled = value; }

    virtual EventType     eventType() const    = 0;
    virtual std::string   name() const         = 0;
    virtual EventCategory categoryFlag() const = 0;

public:
    Event() : _handled(false) {}
    virtual ~Event() = default;

    virtual bool isInCategory(EventCategory const category) const { return categoryFlag() & category; }

#ifdef DEBUG
    virtual std::string toString() const { return "EventName: " + name(); }
#endif
};

} // namespace core
} // namespace zong

#define EVENT_CLASS_TYPE(type)                   \
    static EventType staticType()                \
    {                                            \
        return EventType::type;                  \
    }                                            \
    virtual EventType eventType() const override \
    {                                            \
        return staticType();                     \
    }                                            \
    virtual std::string name() const override    \
    {                                            \
        return #type;                            \
    }

#define EVENT_CLASS_CATEGORY(category)                  \
    virtual EventCategory categoryFlag() const override \
    {                                                   \
        return category;                                \
    }

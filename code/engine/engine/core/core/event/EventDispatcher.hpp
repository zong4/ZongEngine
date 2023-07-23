#pragma once

#include "Event.h"

namespace zong
{
namespace core
{

/**
 * \brief events in here are currently blocking, meaning when an event occurs it
 * immediately gets dispatched and must be dealt with right then an there. \n
 * TODO: for the future, a better strategy might be to buffer events in an event
 * bus and process them during the "event" part of the update stage.
 */
class EventDispatcher
{
private:
    Event& _event;

public:
    EventDispatcher(Event& event) : _event(event) {}

    // f will be deduced by the compiler
    template <typename T, typename F>
    inline bool dispatch(F const& func)
    {
        if (_event.eventType() == T::staticType())
        {
            _event.setHandled(_event.handled() | func(static_cast<T&>(_event)));
            return true;
        }
        return false;
    }
};

} // namespace core
} // namespace zong

#define ZONG_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }
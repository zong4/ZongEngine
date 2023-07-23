#pragma once

#include "../time/Timer.hpp"
#include "Instrumentor.hpp"

namespace zong
{
namespace core
{

class InstrumentationTimer : public Timer
{
private:
    std::string _name;
    bool        _stopped;

private:
    inline std::string name() const { return _name; }
    inline bool        stopped() const { return _stopped; }

    inline void setStopped(bool value) { _stopped = value; }

public:
    InstrumentationTimer(std::string const& name) : Timer(), _name(name), _stopped(false) {}
    ~InstrumentationTimer();

    void stop();
};

} // namespace core
} // namespace zong
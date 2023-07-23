#pragma once

#include "core/pch.hpp"

namespace zong
{
namespace core
{

typedef std::chrono::microseconds TIME_UNIT;

/**
 * \brief a timer
 */
class Timer
{
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> _now;

public:
    inline TIME_UNIT now() const { return std::chrono::duration_cast<TIME_UNIT>(_now.time_since_epoch()); }

public:
    Timer() { reset(); }
    virtual ~Timer() {}

    inline void reset() { _now = std::chrono::high_resolution_clock::now(); }

    TIME_UNIT                        elapsed() const;
    std::chrono::seconds             elapsedSeconds() const;
    inline std::chrono::milliseconds elapsedMillion() const { return elapsedSeconds() * 1000; }
    inline std::chrono::microseconds elapsedMicro() const { return elapsedMillion() * 1000; }
    inline std::chrono::nanoseconds  elapsedNano() const { return elapsedMicro() * 1000; }
};

} // namespace core
} // namespace zong

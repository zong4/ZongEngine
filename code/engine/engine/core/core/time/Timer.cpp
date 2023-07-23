#include "Timer.hpp"

zong::core::TIME_UNIT zong::core::Timer::elapsed() const
{
    return std::chrono::duration_cast<TIME_UNIT>(std::chrono::high_resolution_clock::now() - _now);
}

std::chrono::seconds zong::core::Timer::elapsedSeconds() const
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - _now);
}
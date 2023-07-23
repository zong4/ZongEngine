#include "InstrumentationTimer.hpp"

zong::core::InstrumentationTimer::~InstrumentationTimer()
{
    if (!stopped())
        stop();
}

void zong::core::InstrumentationTimer::stop()
{
    TIME_UNIT const elapsedTime  = this->elapsed();
    TIME_UNIT const highResStart = this->now();
    Instrumentor::instance().writeProfile({name(), highResStart, elapsedTime, std::this_thread::get_id()});
    setStopped(true);
}

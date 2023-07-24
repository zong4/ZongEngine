#include "InstrumentationTimer.hpp"

zong::core::InstrumentationTimer::~InstrumentationTimer()
{
    if (!getStopped())
        stop();
}

void zong::core::InstrumentationTimer::stop()
{
    TIME_UNIT const elapsedTime  = this->getElapsed();
    TIME_UNIT const highResStart = this->getNow();
    Instrumentor::instance().writeProfile({getName(), highResStart, elapsedTime, std::this_thread::get_id()});
    setStopped(true);
}

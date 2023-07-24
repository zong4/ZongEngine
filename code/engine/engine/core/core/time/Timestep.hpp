#pragma once

namespace zong
{
namespace core
{

/**
 * \brief time step, use to fix update
 */
class Timestep
{
private:
    float _time;

public:
    inline float getTime() const { return _time; }
    inline float getSeconds() const { return getTime(); }
    inline float getMilliseconds() const { return getTime() * 1000.0f; }

public:
    Timestep(float time = 0.0f) : _time(time) {}

    inline operator float() const { return getTime(); }
};

} // namespace core
} // namespace zong
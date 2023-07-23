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
    inline float time() const { return _time; }
    inline float seconds() const { return time(); }
    inline float milliseconds() const { return time() * 1000.0f; }

public:
    Timestep(float time = 0.0f) : _time(time) {}

    inline operator float() const { return time(); }
};

} // namespace core
} // namespace zong
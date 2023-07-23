#pragma once

#include "../event/Event.hpp"
#include "../time/Timestep.hpp"

namespace zong
{
namespace core
{

class Layer
{
private:
    std::string _name;

public:
    inline std::string name() const { return _name; }

public:
    Layer(std::string const& name = "Layer") : _name(name) {}
    virtual ~Layer() = default;

    virtual void onAttach() = 0;
    virtual void onDetach() = 0;

    virtual void onUpdate(Timestep const& ts) {}
    virtual void onImGuiRender() {}
    virtual void onEvent(Event& event) {}
};

} // namespace core
} // namespace zong
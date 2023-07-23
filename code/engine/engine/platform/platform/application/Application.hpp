#pragma once

#include <memory>

#include "../window/Window.hpp"
#include "platform/pch.hpp"

namespace platform
{

class Application : public core::Uncopyable
{
protected:
    std::unique_ptr<Window> m_window;

public:
    Application(int argc, char** argv);
    virtual ~Application() = default;

    virtual void Init() = 0;
    virtual void Run()  = 0;
    virtual void Exit() = 0;
};

extern std::unique_ptr<Application> CreateApplicationPtr(int argc, char** argv);

} // namespace platform
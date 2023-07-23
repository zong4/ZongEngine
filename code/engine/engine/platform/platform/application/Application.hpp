#pragma once

#include <memory>

#include "platform/pch.hpp"
#include "platform/window/Window.hpp"

class Application : public Uncopyable
{
private:
    std::unique_ptr<Window> m_window;

public:
    Application(int argc, char** argv);
    virtual ~Application() = default;

    virtual void Init() = 0;
    virtual void Run()  = 0;
    virtual void Exit() = 0;
};

extern std::unique_ptr<Application> CreateApplication(int argc, char** argv);
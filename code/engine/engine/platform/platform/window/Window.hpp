#pragma once

#include "platform/pch.hpp"

namespace platform
{

class Window : public core::Uncopyable
{
public:
    Window(int argc, char** argv);
    virtual ~Window() = default;

    virtual void run() = 0;

private:
    virtual void init(int argc, char** argv) = 0;
    virtual void exit()                      = 0;
};

extern std::unique_ptr<Window> CreateWindowPtr(int argc, char** argv);

} // namespace platform
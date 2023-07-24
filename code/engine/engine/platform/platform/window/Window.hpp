#pragma once

#include "platform/pch.hpp"

namespace platform
{

class Window : public core::Uncopyable
{
protected:
    uint32_t    _width  = 1280;
    uint32_t    _height = 720;
    std::string _rootPath;

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
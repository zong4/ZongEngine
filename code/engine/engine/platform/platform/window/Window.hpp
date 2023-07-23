#pragma once

namespace platform
{

class Window : public core::Uncopyable
{
public:
    Window(int argc, char** argv);
    virtual ~Window() = default;

    virtual void Init() = 0;
    virtual void Run()  = 0;
    virtual void Exit() = 0;
};

extern std::unique_ptr<Window> CreateWindowPtr(int argc, char** argv);

} // namespace platform
#pragma once

#include "../window/Window.hpp"

namespace zong
{
namespace platform
{

class Application : public core::Uncopyable
{
protected:
    std::unique_ptr<Window> _window = nullptr;

public:
    Application()          = default;
    virtual ~Application() = default;

    virtual void run() = 0;

private:
    virtual void init(int argc, char** argv) = 0;
    virtual void exit()                      = 0;
};

extern Application* CreateApplicationPtr(int argc, char** argv);

} // namespace platform
} // namespace zong
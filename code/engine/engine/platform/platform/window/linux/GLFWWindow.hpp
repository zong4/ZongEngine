#pragma once

#include "../Window.hpp"

namespace platform
{

class GLFWWindow : public Window
{
public:
    GLFWWindow(int argc, char** argv);
    virtual ~GLFWWindow() = default;

    void Init() override;
    void Run() override;
    void Exit() override;
};

} // namespace platform
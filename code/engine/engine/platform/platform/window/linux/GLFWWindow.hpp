#pragma once

#include "../Window.hpp"
#include "platform/pch.hpp"

namespace platform
{

class GLFWWindow : public Window
{
private:
    GLFWwindow* _window = nullptr;
    VkInstance  _instance;

public:
    GLFWWindow(int argc, char** argv);
    virtual ~GLFWWindow();

    void run() override;

private:
    void init(int argc, char** argv) override;
    void exit() override;

private:
    void createWindow(int argc, char** argv);
    void createInstance();
};

} // namespace platform
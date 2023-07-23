#include "GLFWWindow.hpp"

platform::GLFWWindow::GLFWWindow(int argc, char** argv) : Window(argc, argv)
{
}

void platform::GLFWWindow::Init()
{
    ZONG_INFO("GLFWWindow::Init()");
}

void platform::GLFWWindow::Run()
{
    ZONG_INFO("GLFWWindow::Run()");
}

void platform::GLFWWindow::Exit()
{
    ZONG_INFO("GLFWWindow::Exit()");
}

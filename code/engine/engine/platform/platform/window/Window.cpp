#include "Window.hpp"

#include "linux/GLFWWindow.hpp"

platform::Window::Window(int argc, char** argv)
{
    // print the arguments
    for (int i = 0; i < argc; ++i)
        ZONG_INFO("Argv[{0}] = {1}", i, argv[i]);
}

std::unique_ptr<platform::Window> platform::CreateWindowPtr(int argc, char** argv)
{
#ifdef WINDOWS
    return std::make_unique<GLFWWindow>(argc, argv);
#else LINUX
    return std::make_unique<GLFWWindow>(argc, argv);
#endif
}
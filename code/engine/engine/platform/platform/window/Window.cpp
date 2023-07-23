#include "Window.hpp"

Window::Window(int argc, char** argv)
{
    // print the arguments
    for (int i = 0; i < argc; ++i)
        ZONG_INFO("Argv[{0}] = {1}", i, argv[i]);
}
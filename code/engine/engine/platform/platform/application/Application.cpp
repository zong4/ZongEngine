#include "Application.hpp"

platform::Application::Application(int argc, char** argv)
{
    m_window = CreateWindowPtr(argc, argv);
}
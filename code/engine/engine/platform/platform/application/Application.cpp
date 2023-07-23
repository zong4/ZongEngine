#include "Application.hpp"

Application::Application(int argc, char** argv)
{
    m_window = std::make_unique<Window>(argc, argv);
}
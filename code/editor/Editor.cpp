#include "Editor.hpp"

Editor::Editor(int argc, char** argv) : Application(argc, argv)
{
}

void Editor::Init()
{
    ZONG_INFO("Editor::Init()");

    m_window->Init();
}

void Editor::Run()
{
    ZONG_INFO("Editor::Run()");

    m_window->Run();
}

void Editor::Exit()
{
    ZONG_INFO("Editor::Exit()");

    m_window->Exit();
}
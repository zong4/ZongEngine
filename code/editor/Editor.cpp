#include "Editor.hpp"

Editor::Editor(int argc, char** argv) : Application()
{
    // ZONG_PROFILE_FUNCTION();

    init(argc, argv);
}

Editor::~Editor()
{
    // ZONG_PROFILE_FUNCTION();

    exit();
}

void Editor::init(int argc, char** argv)
{
    ZONG_PROFILE_FUNCTION();
    ZONG_INFO("Editor::init()");

    _window = platform::CreateWindowPtr(argc, argv);
}

void Editor::run()
{
    ZONG_PROFILE_FUNCTION();
    ZONG_INFO("Editor::run()");

    _window->run();
}

void Editor::exit()
{
    ZONG_PROFILE_FUNCTION();
    ZONG_INFO("Editor::exit()");
}
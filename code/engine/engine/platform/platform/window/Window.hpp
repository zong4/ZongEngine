#pragma once

class Window
{
public:
    Window(int argc, char** argv);
    virtual ~Window() = default;

    void Init() {}
    void Run() {}
    void Exit() {}
};
#pragma once

#include "engine/engine.hpp"

class Editor : public zong::platform::Application
{
public:
    Editor(int argc, char** argv);
    virtual ~Editor();

    virtual void run() override;

private:
    virtual void init(int argc, char** argv) override;
    virtual void exit() override;
};

zong::platform::Application* zong::platform::CreateApplicationPtr(int argc, char** argv)
{
    return new Editor(argc, argv);
}

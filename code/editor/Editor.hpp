#pragma once

#include "engine/engine.hpp"

class Editor : public platform::Application
{
public:
    Editor(int argc, char** argv);
    virtual ~Editor();

    virtual void run() override;

private:
    virtual void init(int argc, char** argv) override;
    virtual void exit() override;
};

platform::Application* platform::CreateApplicationPtr(int argc, char** argv)
{
    return new Editor(argc, argv);
}

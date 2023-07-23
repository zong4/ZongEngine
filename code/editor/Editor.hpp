#pragma once

#include "engine/engine.hpp"

class Editor : public platform::Application
{
public:
    Editor(int argc, char** argv);
    virtual ~Editor() = default;

    virtual void Init() override;
    virtual void Run() override;
    virtual void Exit() override;
};

std::unique_ptr<platform::Application> platform::CreateApplicationPtr(int argc, char** argv)
{
    return std::make_unique<Editor>(argc, argv);
}

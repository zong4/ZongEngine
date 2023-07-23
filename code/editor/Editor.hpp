#pragma once

#include "engine/engine.hpp"

class Editor : public Application
{
public:
    Editor(int argc, char** argv);
    virtual ~Editor() = default;

    virtual void Init() override;
    virtual void Run() override;
    virtual void Exit() override;
};

std::unique_ptr<Application> CreateApplication(int argc, char** argv)
{
    return std::make_unique<Editor>(argc, argv);
}

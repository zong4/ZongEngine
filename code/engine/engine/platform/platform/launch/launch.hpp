#pragma once

#include "platform/application/Application.hpp"

int main(int argc, char** argv)
{
    std::unique_ptr<Application> app = CreateApplication(argc, argv);

    app->Init();
    app->Run();
    app->Exit();

    return 0;
}
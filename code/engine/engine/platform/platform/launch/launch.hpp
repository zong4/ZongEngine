#pragma once

#include "platform/application/Application.hpp"

int main(int argc, char** argv)
{
    ZONG_PROFILE_BEGIN_SESSION("Startup", "Profile-Startup.json");
    platform::Application* app = platform::CreateApplicationPtr(argc, argv);
    ZONG_PROFILE_END_SESSION();

    ZONG_PROFILE_BEGIN_SESSION("Runtime", "Profile-Runtime.json");
    app->run();
    ZONG_PROFILE_END_SESSION();

    ZONG_PROFILE_BEGIN_SESSION("Shutdown", "Profile-Shutdown.json");
    delete app;
    ZONG_PROFILE_END_SESSION();

    return 0;
}
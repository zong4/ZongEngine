#pragma once

#include "Engine/Core/Application.h"

extern Hazel::Application* Hazel::CreateApplication(int argc, char** argv);
bool g_ApplicationRunning = true;

namespace Hazel {

	int Main(int argc, char** argv)
	{
		while (g_ApplicationRunning)
		{
			InitializeCore();
			Application* app = CreateApplication(argc, argv);
			ZONG_CORE_ASSERT(app, "Client Application is null!");
			app->Run();
			delete app;
			ShutdownCore();
		}
		return 0;
	}

}

#if ZONG_DIST && ZONG_PLATFORM_WINDOWS

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	return Hazel::Main(__argc, __argv);
}

#else

int main(int argc, char** argv)
{
	return Hazel::Main(argc, argv);
}

#endif // ZONG_DIST

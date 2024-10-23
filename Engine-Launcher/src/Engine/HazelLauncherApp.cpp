#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "LauncherLayer.h"

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Engine Launcher";
	spec.Width = 960;
	spec.Height = 670;
	spec.NoTitlebar = true;

	// Fill in these variables with app-specific data
	Hazel::LauncherSpecification launcherSpec;
	launcherSpec.AppExecutableTitle = "Engine App";
	launcherSpec.AppExecutablePath = "HazelApp.exe";
	launcherSpec.TieringSettingsPath = "TieringSettings.yaml";
	launcherSpec.AppSettingsPath = "AppSettings.yaml";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer(std::make_shared<Hazel::LauncherLayer>(launcherSpec));
	return app;
}

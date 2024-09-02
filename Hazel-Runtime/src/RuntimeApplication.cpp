#include <Hazel.h>
#include <Hazel/EntryPoint.h>

#include "RuntimeLayer.h"

#include "Hazel/Renderer/RendererAPI.h"

#include "Hazel/Tiering/TieringSerializer.h"

class RuntimeApplication : public Hazel::Application
{
public:
	RuntimeApplication(const Hazel::ApplicationSpecification& specification, std::string_view projectPath)
		: Application(specification)
		, m_ProjectPath(projectPath)
	{
		s_IsRuntime = true;
	}

	virtual void OnInit() override
	{
		PushLayer(new Hazel::RuntimeLayer(m_ProjectPath));
	}

private:
	std::string m_ProjectPath;
};

Hazel::Application* Hazel::CreateApplication(int argc, char** argv)
{
	// Project path specified as root directory of project
	std::string_view projectPath = "SandboxProject";
	if (argc > 1)
	{
		projectPath = argv[1];
	}

	Hazel::ApplicationSpecification specification;

	/*
	------------------------------
	-- Tiering settings example --
	------------------------------

	Tiering::TieringSettings ts;
	if (TieringSerializer::Deserialize(ts, "Settings.yaml"))
	{
		specification.Fullscreen = !ts.RendererTS.Windowed;
		specification.VSync = ts.RendererTS.VSync;
	}
	*/

#ifndef HZ_DIST
	// NOTE: these overrides are for development, we normally want to take these
	//       settings from launcher/tiering settings as above
	// specification.VSync = false;
	specification.Fullscreen = false;
#endif

	specification.Name = "Hazel Runtime " HZ_VERSION " (" HZ_BUILD_PLATFORM_NAME " " HZ_BUILD_CONFIG_NAME ")";
	specification.WindowWidth = 1920;
	specification.WindowHeight = 1080;
	specification.WindowDecorated = !specification.Fullscreen;
	specification.Resizable = !specification.Fullscreen;
	specification.StartMaximized = false;
	specification.EnableImGui = false;
	specification.IconPath = "Resources/Icon.png";

	// IMPORTANT: Disable for ACTUAL Dist builds
	specification.WorkingDirectory = "../Hazelnut";

	// specification.ScriptConfig.EnableDebugging = false;
	// specification.ScriptConfig.EnableProfiling = false;

	specification.RenderConfig.ShaderPackPath = "Resources/ShaderPack.hsp";
	specification.RenderConfig.FramesInFlight = 3;

	specification.CoreThreadingPolicy = ThreadingPolicy::MultiThreaded;

	return new RuntimeApplication(specification, projectPath);
}

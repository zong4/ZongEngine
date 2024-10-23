#include <Hazel.h>
#include <Engine/EntryPoint.h>

#include "RuntimeLayer.h"

#include "Engine/Renderer/RendererAPI.h"

#include "Engine/Tiering/TieringSerializer.h"

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
	std::string_view projectPath = "SandboxProject/Sandbox.hproj";
	if (argc > 1)
	{
		projectPath = argv[1];
	}

	//projectPath = "LD51/LD51.hproj";
	//projectPath = "LD53/LD53.hproj";

	Hazel::ApplicationSpecification specification;
	specification.Fullscreen = true;

	Tiering::TieringSettings ts;
	if (TieringSerializer::Deserialize(ts, "LD53/Config/Settings.yaml"))
	{
		specification.Fullscreen = !ts.RendererTS.Windowed;
		specification.VSync = ts.RendererTS.VSync;
	}

	// REMOVE
	specification.Fullscreen = false;

	specification.Name = "Saving Captain Cino";
	specification.WindowWidth = 1920;
	specification.WindowHeight = 1080;
	specification.WindowDecorated = !specification.Fullscreen;
	specification.Resizable = !specification.Fullscreen;
	specification.StartMaximized = false;
	specification.EnableImGui = false;
	specification.IconPath = "Resources/Icon.png";

	// IMPORTANT: Disable for ACTUAL Dist builds
#ifndef ZONG_DIST
	//specification.WorkingDirectory = "../Editor";
#endif

	specification.ScriptConfig.CoreAssemblyPath = "Resources/Scripts/Engine-ScriptCore.dll";
	specification.ScriptConfig.EnableDebugging = false;
	specification.ScriptConfig.EnableProfiling = false;

	specification.RenderConfig.ShaderPackPath = "Resources/ShaderPack.hsp";
	specification.RenderConfig.FramesInFlight = 3;

	specification.CoreThreadingPolicy = ThreadingPolicy::MultiThreaded;

	return new RuntimeApplication(specification, projectPath);
}

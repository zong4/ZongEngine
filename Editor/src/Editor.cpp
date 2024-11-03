#include "EditorLayer.h"
#include "Engine/Utilities/FileSystem.h"

#include "Engine/EntryPoint.h"

#ifdef ZONG_PLATFORM_WINDOWS
#include <Shlobj.h>
#endif

class EditorApplication : public Engine::Application
{
public:
	EditorApplication(const Engine::ApplicationSpecification& specification, std::string_view projectPath)
		: Application(specification), m_ProjectPath(projectPath), m_UserPreferences(Engine::Ref<Engine::UserPreferences>::Create())
	{
		if (projectPath.empty())
			m_ProjectPath = "SandboxProject/Sandbox.hproj";
	}

	virtual void OnInit() override
	{
		// Persistent Storage
		{
			m_PersistentStoragePath = Engine::FileSystem::GetPersistentStoragePath() / "Editor";

			if (!std::filesystem::exists(m_PersistentStoragePath))
				std::filesystem::create_directory(m_PersistentStoragePath);
		}

		// User Preferences
		{
			Engine::UserPreferencesSerializer serializer(m_UserPreferences);
			if (!std::filesystem::exists(m_PersistentStoragePath / "UserPreferences.yaml"))
				serializer.Serialize(m_PersistentStoragePath / "UserPreferences.yaml");
			else
				serializer.Deserialize(m_PersistentStoragePath / "UserPreferences.yaml");

			if (!m_ProjectPath.empty())
				m_UserPreferences->StartupProject = m_ProjectPath;
			else if (!m_UserPreferences->StartupProject.empty())
				m_ProjectPath = m_UserPreferences->StartupProject;
		}

		// Update the ENGINE_DIR environment variable every time we launch
		{
			std::filesystem::path workingDirectory = std::filesystem::current_path();

			if (workingDirectory.stem().string() == "Editor")
				workingDirectory = workingDirectory.parent_path();

			Engine::FileSystem::SetEnvironmentVariable("ENGINE_DIR", workingDirectory.string());
		}

		PushLayer(new Engine::EditorLayer(m_UserPreferences));
	}

private:
	std::string m_ProjectPath;
	std::filesystem::path m_PersistentStoragePath;
	Engine::Ref<Engine::UserPreferences> m_UserPreferences;
};

Engine::Application* Engine::CreateApplication(int argc, char** argv)
{
	std::string_view projectPath;
	if (argc > 1)
		projectPath = argv[1];

	Engine::ApplicationSpecification specification;
	specification.Name = "Editor";
	specification.WindowWidth = 1600;
	specification.WindowHeight = 900;
	specification.StartMaximized = true;
	specification.VSync = true;
	//specification.RenderConfig.ShaderPackPath = "Resources/ShaderPack.hsp";

	specification.ScriptConfig.CoreAssemblyPath = "Resources/Scripts/Engine-ScriptCore.dll";

#ifdef ZONG_PLATFORM_WINDOWS
	specification.ScriptConfig.EnableDebugging = true;
	specification.ScriptConfig.EnableProfiling = true;
#else
	specification.ScriptConfig.EnableDebugging = false;
	specification.ScriptConfig.EnableProfiling = false;
#endif

	specification.CoreThreadingPolicy = ThreadingPolicy::SingleThreaded;

	return new EditorApplication(specification, projectPath);
}

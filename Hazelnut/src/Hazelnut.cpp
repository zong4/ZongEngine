#include "EditorLayer.h"
#include "Hazel/Utilities/FileSystem.h"

#include "Hazel/EntryPoint.h"

#ifdef HZ_PLATFORM_WINDOWS
#include <Shlobj.h>
#endif

class HazelnutApplication : public Hazel::Application
{
public:
	HazelnutApplication(const Hazel::ApplicationSpecification& specification, std::string_view projectPath)
		: Application(specification), m_ProjectPath(projectPath), m_UserPreferences(Hazel::Ref<Hazel::UserPreferences>::Create())
	{
		if (projectPath.empty())
			m_ProjectPath = "SandboxProject/Sandbox.hproj";
	}

	virtual void OnInit() override
	{
		// Persistent Storage
		{
			m_PersistentStoragePath = Hazel::FileSystem::GetPersistentStoragePath() / "Hazelnut";

			if (!std::filesystem::exists(m_PersistentStoragePath))
				std::filesystem::create_directory(m_PersistentStoragePath);
		}

		// User Preferences
		{
			Hazel::UserPreferencesSerializer serializer(m_UserPreferences);
			if (!std::filesystem::exists(m_PersistentStoragePath / "UserPreferences.yaml"))
				serializer.Serialize(m_PersistentStoragePath / "UserPreferences.yaml");
			else
				serializer.Deserialize(m_PersistentStoragePath / "UserPreferences.yaml");

			if (!m_ProjectPath.empty())
				m_UserPreferences->StartupProject = m_ProjectPath;
			else if (!m_UserPreferences->StartupProject.empty())
				m_ProjectPath = m_UserPreferences->StartupProject;
		}

		// Update the HAZEL_DIR environment variable every time we launch
		{
			std::filesystem::path workingDirectory = std::filesystem::current_path();

			if (workingDirectory.stem().string() == "Hazelnut")
				workingDirectory = workingDirectory.parent_path();

			Hazel::FileSystem::SetEnvironmentVariable("HAZEL_DIR", workingDirectory.string());
		}

		PushLayer(new Hazel::EditorLayer(m_UserPreferences));
	}

private:
	std::string m_ProjectPath;
	std::filesystem::path m_PersistentStoragePath;
	Hazel::Ref<Hazel::UserPreferences> m_UserPreferences;
};

Hazel::Application* Hazel::CreateApplication(int argc, char** argv)
{
	std::string_view projectPath;
	if (argc > 1)
		projectPath = argv[1];

	Hazel::ApplicationSpecification specification;
	specification.Name = "Hazelnut";
	specification.WindowWidth = 1600;
	specification.WindowHeight = 900;
	specification.StartMaximized = true;
	specification.VSync = true;
	//specification.RenderConfig.ShaderPackPath = "Resources/ShaderPack.hsp";

	specification.ScriptConfig.CoreAssemblyPath = "Resources/Scripts/Hazel-ScriptCore.dll";

#ifdef HZ_PLATFORM_WINDOWS
	specification.ScriptConfig.EnableDebugging = true;
	specification.ScriptConfig.EnableProfiling = true;
#else
	specification.ScriptConfig.EnableDebugging = false;
	specification.ScriptConfig.EnableProfiling = false;
#endif

	specification.CoreThreadingPolicy = ThreadingPolicy::SingleThreaded;

	return new HazelnutApplication(specification, projectPath);
}

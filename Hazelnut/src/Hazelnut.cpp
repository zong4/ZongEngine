#include "EditorLayer.h"
#include "Hazel/Utilities/FileSystem.h"
#include "Hazel/Utilities/CommandLineParser.h"

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

			if (!Hazel::FileSystem::Exists(m_PersistentStoragePath))
				Hazel::FileSystem::CreateDirectory(m_PersistentStoragePath);
		}

		// User Preferences
		{
			Hazel::UserPreferencesSerializer serializer(m_UserPreferences);
			if (!Hazel::FileSystem::Exists(m_PersistentStoragePath / "UserPreferences.yaml"))
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
			auto workingDirectory = Hazel::FileSystem::GetWorkingDirectory();

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
	Hazel::CommandLineParser cli(argc, argv);

	auto raw = cli.GetRawArgs();
	if(raw.size() > 1) {
		HZ_CORE_WARN("More than one project path specified, using `{}'", raw[0]);
	}

	auto cd = cli.GetOpt("C");
	if(!cd.empty()) {
		Hazel::FileSystem::SetWorkingDirectory(cd);
	}

	std::string_view projectPath;
	if(!raw.empty()) projectPath = raw[0];

	Hazel::ApplicationSpecification specification;
	specification.Name = "Hazelnut";
	specification.WindowWidth = 1600;
	specification.WindowHeight = 900;
	specification.StartMaximized = true;
	specification.VSync = true;
	//specification.RenderConfig.ShaderPackPath = "Resources/ShaderPack.hsp";

	/*specification.ScriptConfig.CoreAssemblyPath = "Resources/Scripts/Hazel-ScriptCore.dll";
	specification.ScriptConfig.EnableDebugging = true;
	specification.ScriptConfig.EnableProfiling = true;*/

	specification.CoreThreadingPolicy = ThreadingPolicy::SingleThreaded;

	return new HazelnutApplication(specification, projectPath);
}

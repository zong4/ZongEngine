#include <Engine/EntryPoint.h>
#include <Engine/Utilities/ProcessHelper.h>
#include "LauncherLayer.h"

class EditorLauncherApplication : public Engine::Application
{
public:
	EditorLauncherApplication(const Engine::ApplicationSpecification& specification)
		: Application(specification), m_UserPreferences(Engine::Ref<Engine::UserPreferences>::Create())
	{
	}

	virtual void OnInit() override
	{
		std::filesystem::path persistentStoragePath = Engine::FileSystem::GetPersistentStoragePath();

		// User Preferences
		{
			Engine::UserPreferencesSerializer serializer(m_UserPreferences);
			if (!std::filesystem::exists(persistentStoragePath / "UserPreferences.yaml"))
				serializer.Serialize(persistentStoragePath / "UserPreferences.yaml");
			else
				serializer.Deserialize(persistentStoragePath / "UserPreferences.yaml");
		}

		Engine::LauncherProperties launcherProperties;
		launcherProperties.UserPreferences = m_UserPreferences;
		launcherProperties.ProjectOpenedCallback = std::bind(&EditorLauncherApplication::OnProjectOpened, this, std::placeholders::_1);

		// Installation Path
		{
			if (Engine::FileSystem::HasEnvironmentVariable("ENGINE_DIR"))
				launcherProperties.InstallPath = Engine::FileSystem::GetEnvironmentVariable("ENGINE_DIR");
		}

		SetShowStats(false);
		PushLayer(new Engine::LauncherLayer(launcherProperties));
	}

private:
	void OnProjectOpened(std::string projectPath)
	{
		std::filesystem::path engineDir = Engine::FileSystem::GetEnvironmentVariable("ENGINE_DIR");
		Engine::ProcessInfo editorProcessInfo;

#ifdef ZONG_DEBUG
	#define ZONG_BINDIR_PREFIX "Debug"
#else
	#define ZONG_BINDIR_PREFIX "Release"
#endif

#ifdef ZONG_PLATFORM_WINDOWS
	#define ZONG_BINDIR_PLATFORM "windows"
	#define ZONG_BIN_SUFFIX ".exe"
#elif defined(ZONG_PLATFORM_LINUX)
	#define ZONG_BINDIR_PLATFORM "linux"
	#define ZONG_BIN_SUFFIX ""
#endif

		editorProcessInfo.FilePath = engineDir / "bin" / ZONG_BINDIR_PREFIX "-" ZONG_BINDIR_PLATFORM "-x86_64" / "Editor" / "Editor" ZONG_BIN_SUFFIX;

		editorProcessInfo.CommandLine = projectPath;
		editorProcessInfo.WorkingDirectory = engineDir / "Editor";
		editorProcessInfo.Detached = true;
		Engine::ProcessHelper::CreateProcess(editorProcessInfo);
	}

private:
	Engine::Ref<Engine::UserPreferences> m_UserPreferences;
};

Engine::Application* Engine::CreateApplication(int argc, char** argv)
{
	Engine::ApplicationSpecification specification;
	specification.Name = "Editor Launcher";
	specification.WindowWidth = 1280;
	specification.WindowHeight = 720;
	specification.VSync = true;
	specification.StartMaximized = false;
	specification.Resizable = false;
	specification.WorkingDirectory = FileSystem::HasEnvironmentVariable("ENGINE_DIR") ? FileSystem::GetEnvironmentVariable("ENGINE_DIR") + "/Editor" : "../Editor";

	return new EditorLauncherApplication(specification);
}

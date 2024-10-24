#include <Hazel/EntryPoint.h>
#include <Hazel/Utilities/ProcessHelper.h>
#include "LauncherLayer.h"

class HazelnutLauncherApplication : public Hazel::Application
{
public:
	HazelnutLauncherApplication(const Hazel::ApplicationSpecification& specification)
		: Application(specification), m_UserPreferences(Hazel::Ref<Hazel::UserPreferences>::Create())
	{
	}

	virtual void OnInit() override
	{
		std::filesystem::path persistentStoragePath = Hazel::FileSystem::GetPersistentStoragePath();

		// User Preferences
		{
			Hazel::UserPreferencesSerializer serializer(m_UserPreferences);
			if (!std::filesystem::exists(persistentStoragePath / "UserPreferences.yaml"))
				serializer.Serialize(persistentStoragePath / "UserPreferences.yaml");
			else
				serializer.Deserialize(persistentStoragePath / "UserPreferences.yaml");
		}

		Hazel::LauncherProperties launcherProperties;
		launcherProperties.UserPreferences = m_UserPreferences;
		launcherProperties.ProjectOpenedCallback = std::bind(&HazelnutLauncherApplication::OnProjectOpened, this, std::placeholders::_1);

		// Installation Path
		{
			if (Hazel::FileSystem::HasEnvironmentVariable("HAZEL_DIR"))
				launcherProperties.InstallPath = Hazel::FileSystem::GetEnvironmentVariable("HAZEL_DIR");
		}

		SetShowStats(false);
		PushLayer(new Hazel::LauncherLayer(launcherProperties));
	}

private:
	void OnProjectOpened(std::string projectPath)
	{
		std::filesystem::path hazelDir = Hazel::FileSystem::GetEnvironmentVariable("HAZEL_DIR");
		Hazel::ProcessInfo hazelnutProcessInfo;

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

		hazelnutProcessInfo.FilePath = hazelDir / "bin" / ZONG_BINDIR_PREFIX "-" ZONG_BINDIR_PLATFORM "-x86_64" / "Editor" / "Editor" ZONG_BIN_SUFFIX;

		hazelnutProcessInfo.CommandLine = projectPath;
		hazelnutProcessInfo.WorkingDirectory = hazelDir / "Editor";
		hazelnutProcessInfo.Detached = true;
		Hazel::ProcessHelper::CreateProcess(hazelnutProcessInfo);
	}

private:
	Hazel::Ref<Hazel::UserPreferences> m_UserPreferences;
};

Hazel::Application* Hazel::CreateApplication(int argc, char** argv)
{
	Hazel::ApplicationSpecification specification;
	specification.Name = "Editor Launcher";
	specification.WindowWidth = 1280;
	specification.WindowHeight = 720;
	specification.VSync = true;
	specification.StartMaximized = false;
	specification.Resizable = false;
	specification.WorkingDirectory = FileSystem::HasEnvironmentVariable("HAZEL_DIR") ? FileSystem::GetEnvironmentVariable("HAZEL_DIR") + "/Editor" : "../Editor";

	return new HazelnutLauncherApplication(specification);
}

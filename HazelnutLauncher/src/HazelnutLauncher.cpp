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

#ifdef HZ_DEBUG
	#define HZ_BINDIR_PREFIX "Debug"
#else
	#define HZ_BINDIR_PREFIX "Release"
#endif

#ifdef HZ_PLATFORM_WINDOWS
	#define HZ_BINDIR_PLATFORM "windows"
	#define HZ_BIN_SUFFIX ".exe"
#elif defined(HZ_PLATFORM_LINUX)
	#define HZ_BINDIR_PLATFORM "linux"
	#define HZ_BIN_SUFFIX ""
#endif

		hazelnutProcessInfo.FilePath = hazelDir / "bin" / HZ_BINDIR_PREFIX "-" HZ_BINDIR_PLATFORM "-x86_64" / "Hazelnut" / "Hazelnut" HZ_BIN_SUFFIX;

		hazelnutProcessInfo.CommandLine = projectPath;
		hazelnutProcessInfo.WorkingDirectory = hazelDir / "Hazelnut";
		hazelnutProcessInfo.Detached = true;
		Hazel::ProcessHelper::CreateProcess(hazelnutProcessInfo);
	}

private:
	Hazel::Ref<Hazel::UserPreferences> m_UserPreferences;
};

Hazel::Application* Hazel::CreateApplication(int argc, char** argv)
{
	Hazel::ApplicationSpecification specification;
	specification.Name = "Hazelnut Launcher";
	specification.WindowWidth = 1280;
	specification.WindowHeight = 720;
	specification.VSync = true;
	specification.StartMaximized = false;
	specification.Resizable = false;
	specification.WorkingDirectory = FileSystem::HasEnvironmentVariable("HAZEL_DIR") ? FileSystem::GetEnvironmentVariable("HAZEL_DIR") + "/Hazelnut" : "../Hazelnut";

	return new HazelnutLauncherApplication(specification);
}

#pragma once

#include "Hazel/Asset/AssetManager/EditorAssetManager.h"
#include "Hazel/Asset/AssetManager/RuntimeAssetManager.h"
#include "Hazel/Core/Log.h"
#include "Hazel/Core/Ref.h"
#include "Hazel/Physics/PhysicsAPI.h"

#include <filesystem>
#include <format>


namespace Hazel {

	class AudioCommandRegistry;

	struct ProjectConfig
	{
		std::string Name;

		std::string AssetDirectory = "Assets";
		std::string AssetRegistryPath = "Assets/AssetRegistry.hzr";

		std::string AudioCommandsRegistryPath = "Assets/AudioCommandsRegistry.hzr";

		std::string MeshPath = "Assets/Meshes";
		std::string MeshSourcePath = "Assets/Meshes/Source";

		std::string AnimationPath;

		std::string ScriptModulePath = "Assets/Scripts/Binaries";
		std::string DefaultNamespace;

		std::string StartScene;

		bool AutomaticallyReloadAssembly;

		bool EnableAutoSave = false;
		int AutoSaveIntervalSeconds = 300;

		PhysicsAPIType CurrentPhysicsAPI = PhysicsAPIType::Jolt;

		// Not serialized
		std::string ProjectFileName;
		std::string ProjectDirectory;

		// Runtime only
		AssetHandle StartSceneHandle;
	};

	class Project : public RefCounted
	{
	public:
		Project();
		~Project();

		const ProjectConfig& GetConfig() const { return m_Config; }

		void ReloadScriptEngine();

		static Ref<Project> GetActive() { return s_ActiveProject; }
		static void SetActive(Ref<Project> project);
		static void SetActiveRuntime(Ref<Project> project, Ref<AssetPack> assetPack);

		inline static Ref<AssetManagerBase> GetAssetManager() { return s_AssetManager; }
		inline static Ref<EditorAssetManager> GetEditorAssetManager() { return s_AssetManager.As<EditorAssetManager>(); }
		inline static Ref<RuntimeAssetManager> GetRuntimeAssetManager() { return s_AssetManager.As<RuntimeAssetManager>(); }

		static const std::string& GetProjectName()
		{
			HZ_CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->GetConfig().Name;
		}

		static std::filesystem::path GetProjectDirectory()
		{
			HZ_CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->GetConfig().ProjectDirectory;
		}

		std::filesystem::path GetAssetDirectory() const
		{
			return std::filesystem::path(GetConfig().ProjectDirectory) / GetConfig().AssetDirectory;
		}

		static std::filesystem::path GetActiveAssetDirectory()
		{
			HZ_CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->GetAssetDirectory();
		}

		static std::filesystem::path GetAssetRegistryPath()
		{
			HZ_CORE_ASSERT(s_ActiveProject);
			return std::filesystem::path(s_ActiveProject->GetConfig().ProjectDirectory) / s_ActiveProject->GetConfig().AssetRegistryPath;
		}

		static std::filesystem::path GetMeshPath()
		{
			HZ_CORE_ASSERT(s_ActiveProject);
			return std::filesystem::path(s_ActiveProject->GetConfig().ProjectDirectory) / s_ActiveProject->GetConfig().MeshPath;
		}

		static std::filesystem::path GetAnimationPath()
		{
			HZ_CORE_ASSERT(s_ActiveProject);
			return std::filesystem::path(s_ActiveProject->GetConfig().ProjectDirectory) / s_ActiveProject->GetConfig().AnimationPath;
		}

		static std::filesystem::path GetAudioCommandsRegistryPath()
		{
			HZ_CORE_ASSERT(s_ActiveProject);
			return std::filesystem::path(s_ActiveProject->GetConfig().ProjectDirectory) / s_ActiveProject->GetConfig().AudioCommandsRegistryPath;
		}

		static std::filesystem::path GetScriptModulePath()
		{
			HZ_CORE_ASSERT(s_ActiveProject);
			return std::filesystem::path(s_ActiveProject->GetConfig().ProjectDirectory) / s_ActiveProject->GetConfig().ScriptModulePath;
		}

		static std::filesystem::path GetScriptModuleFilePath()
		{
			HZ_CORE_ASSERT(s_ActiveProject);
			return GetScriptModulePath() / std::format("{0}.dll", GetProjectName());
		}

		std::filesystem::path GetScriptProjectPath() const
		{
			return GetAssetDirectory() / "Scripts" / (GetConfig().Name + ".csproj");
		}

		static std::filesystem::path GetActiveScriptProjectPath()
		{
			HZ_CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->GetScriptProjectPath();
		}

		static std::filesystem::path GetCacheDirectory()
		{
			HZ_CORE_ASSERT(s_ActiveProject);
			return std::filesystem::path(s_ActiveProject->GetConfig().ProjectDirectory) / "Cache";
		}
	private:
		void OnSerialized();
		void OnDeserialized();

	private:
		ProjectConfig m_Config;
		Ref<AudioCommandRegistry> m_AudioCommands;
		inline static Ref<AssetManagerBase> s_AssetManager;

		friend class ProjectSettingsWindow;
		friend class ProjectSerializer;

		inline static Ref<Project> s_ActiveProject;
	};

}

#include "hzpch.h"
#include "EditorAssetManager.h"

#include "Hazel/Asset/AssetExtensions.h"
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Core/Events/EditorEvents.h"
#include "Hazel/Core/Timer.h"
#include "Hazel/Debug/Profiler.h"
#include "Hazel/Project/Project.h"
#include "Hazel/Utilities/StringUtils.h"

#include <yaml-cpp/yaml.h>

namespace Hazel {

	static AssetMetadata s_NullMetadata;

	EditorAssetManager::EditorAssetManager()
	{
#if ASYNC_ASSETS
		m_AssetThread = Ref<EditorAssetSystem>::Create();
#endif

		AssetImporter::Init();

		LoadAssetRegistry();
		ReloadAssets();
	}

	EditorAssetManager::~EditorAssetManager()
	{
		// TODO(Yan): shutdown explicitly?
		Shutdown();
	}

	void EditorAssetManager::Shutdown()
	{
#if ASYNC_ASSETS
		m_AssetThread->StopAndWait();
#endif
		WriteRegistryToFile();
	}

	AssetType EditorAssetManager::GetAssetType(AssetHandle assetHandle)
	{
		if (!IsAssetHandleValid(assetHandle))
			return AssetType::None;

		if (IsMemoryAsset(assetHandle))
			return GetAsset(assetHandle)->GetAssetType();

		const auto& metadata = GetMetadata(assetHandle);
		return metadata.Type;
	}

	Ref<Asset> EditorAssetManager::GetAsset(AssetHandle assetHandle)
	{
		HZ_PROFILE_FUNC();
		HZ_SCOPE_PERF("AssetManager::GetAsset");

		Ref<Asset> asset = GetAssetIncludingInvalid(assetHandle);
		return asset && asset->IsValid() ? asset : nullptr;
	}

	AsyncAssetResult<Asset> EditorAssetManager::GetAssetAsync(AssetHandle assetHandle)
	{
#if ASYNC_ASSETS
		HZ_PROFILE_FUNC();
		HZ_SCOPE_PERF("AssetManager::GetAssetAsync");

		if (auto asset = GetMemoryAsset(assetHandle); asset)
			return { asset, true };

		auto metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
			return { nullptr }; // TODO(Yan): return special error asset

		Ref<Asset> asset = nullptr;
		if (metadata.IsDataLoaded)
		{
			HZ_CORE_VERIFY(m_LoadedAssets.contains(assetHandle));
			return { m_LoadedAssets.at(assetHandle), true };
		}

		// Queue load (if not already) and return placeholder
		if (metadata.Status != AssetStatus::Loading)
		{
			auto metadataLoad = metadata;
			metadataLoad.Status = AssetStatus::Loading;
			SetMetadata(assetHandle, metadataLoad);
			m_AssetThread->QueueAssetLoad(metadata);
		}

		return AssetManager::GetPlaceholderAsset(metadata.Type);
#else
		return { GetAsset(assetHandle), true };
#endif
	}

	void EditorAssetManager::AddMemoryOnlyAsset(Ref<Asset> asset)
	{
		// Memory-only assets are not added to m_AssetRegistry (because that would require full thread synchronization for access to registry, we would like to avoid that)
		std::scoped_lock lock(m_MemoryAssetsMutex);
		m_MemoryAssets[asset->Handle] = asset;
	}

	std::unordered_set<AssetHandle> EditorAssetManager::GetAllAssetsWithType(AssetType type)
	{
		std::unordered_set<AssetHandle> result;

		// loop over memory only assets
		// This needs a lock because asset thread can create memory only assets
		{
			std::shared_lock lock(m_MemoryAssetsMutex);
			for (const auto& [handle, asset] : m_MemoryAssets)
			{
				if (asset->GetAssetType() == type)
					result.insert(handle);
			}
		}

		{
			std::shared_lock lock(m_AssetRegistryMutex);
			for (const auto& [handle, metadata] : m_AssetRegistry)
			{
				if (metadata.Type == type)
					result.insert(handle);
			}
		}
		return result;
	}

	std::unordered_map<AssetHandle, Ref<Asset>> EditorAssetManager::GetMemoryAssets()
	{
		std::shared_lock lock(m_MemoryAssetsMutex);
		return m_MemoryAssets;
	}

	AssetMetadata EditorAssetManager::GetMetadata(AssetHandle handle)
	{
		std::shared_lock lock(m_AssetRegistryMutex);

		if (m_AssetRegistry.Contains(handle))
			return m_AssetRegistry.Get(handle);

		return s_NullMetadata;
	}

	void EditorAssetManager::SetMetadata(AssetHandle handle, const AssetMetadata& metadata)
	{
		std::unique_lock lock(m_AssetRegistryMutex);
		m_AssetRegistry.Set(handle, metadata);
	}

	const AssetMetadata& EditorAssetManager::GetMetadata(const std::filesystem::path& filepath)
	{
		const auto relativePath = GetRelativePath(filepath);

		for (auto& [handle, metadata] : m_AssetRegistry)
		{
			if (metadata.FilePath == relativePath)
				return metadata;
		}

		return s_NullMetadata;
	}

	AssetHandle EditorAssetManager::GetAssetHandleFromFilePath(const std::filesystem::path& filepath)
	{
		return GetMetadata(filepath).Handle;
	}

	AssetType EditorAssetManager::GetAssetTypeFromExtension(const std::string& extension)
	{
		std::string ext = Utils::String::ToLowerCopy(extension);
		if (s_AssetExtensionMap.find(ext) == s_AssetExtensionMap.end())
			return AssetType::None;

		return s_AssetExtensionMap.at(ext.c_str());
	}

	std::string EditorAssetManager::GetDefaultExtensionForAssetType(AssetType type)
	{
		for (const auto& [ext, assetType] : s_AssetExtensionMap)
		{
			if (assetType == type)
				return ext;
		}
		return "";
	}

	AssetType EditorAssetManager::GetAssetTypeFromPath(const std::filesystem::path& path)
	{
		return GetAssetTypeFromExtension(path.extension().string());
	}

	std::filesystem::path EditorAssetManager::GetFileSystemPath(const AssetMetadata& metadata)
	{
		return Project::GetActiveAssetDirectory() / metadata.FilePath;
	}

	std::filesystem::path EditorAssetManager::GetFileSystemPath(AssetHandle handle)
	{
		return GetFileSystemPathString(GetMetadata(handle));
	}

	std::string EditorAssetManager::GetFileSystemPathString(const AssetMetadata& metadata)
	{
		return GetFileSystemPath(metadata).string();
	}

	std::filesystem::path EditorAssetManager::GetRelativePath(const std::filesystem::path& filepath)
	{
		std::filesystem::path relativePath = filepath.lexically_normal();
		std::string temp = filepath.string();
		if (temp.find(Project::GetActiveAssetDirectory().string()) != std::string::npos)
		{
			relativePath = std::filesystem::relative(filepath, Project::GetActiveAssetDirectory());
			if (relativePath.empty())
			{
				relativePath = filepath.lexically_normal();
			}
		}
		return relativePath;
	}

	bool EditorAssetManager::FileExists(AssetMetadata& metadata) const
	{
		return FileSystem::Exists(Project::GetActive()->GetAssetDirectory() / metadata.FilePath);
	}

	bool EditorAssetManager::ReloadData(AssetHandle assetHandle)
	{
		auto metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
		{
			HZ_CORE_ERROR("Trying to reload invalid asset");
			return false;
		}

		Ref<Asset> asset = GetAsset(assetHandle);

		// If the asset is a Mesh, StaticMesh, Skeleton, or Animation, then instead of reloading the mesh we reload
		// the underlying mesh source.
		// (the assumption being that its the mesh source that's likely changed (e.g. via DCC authoring tool) and
		// its that content that the user wishes to reload)
		// The Mesh/StaticMesh/Skeleton/Animation ends up getting reloaded anyway due asset dependencies)
		if (metadata.Type == AssetType::StaticMesh && asset)
		{
			auto mesh = asset.As<StaticMesh>();
			return ReloadData(mesh->GetMeshSource());
		}
		if (metadata.Type == AssetType::Mesh && asset)
		{
			auto mesh = asset.As<Mesh>();
			return ReloadData(mesh->GetMeshSource());
		}
		else if (metadata.Type == AssetType::Skeleton && asset)
		{
			auto skeleton = asset.As<SkeletonAsset>();
			return ReloadData(skeleton->GetMeshSource());
		}
		else if (metadata.Type == AssetType::Animation && asset)
		{
			auto animation = asset.As<AnimationAsset>();
			return ReloadData(animation->GetSkeletonSource()) && ((animation->GetAnimationSource() == animation->GetSkeletonSource()) || ReloadData(animation->GetAnimationSource()));
		}
		else
		{
			HZ_CORE_INFO_TAG("AssetManager", "RELOADING ASSET - {}", metadata.FilePath.string());
			metadata.IsDataLoaded = AssetImporter::TryLoadData(metadata, asset);
			if (metadata.IsDataLoaded)
			{
				auto absolutePath = GetFileSystemPath(metadata);
				metadata.FileLastWriteTime = FileSystem::GetLastWriteTime(absolutePath);
				m_LoadedAssets[assetHandle] = asset;
				SetMetadata(assetHandle, metadata);
				HZ_CORE_INFO_TAG("AssetManager", "Finished reloading asset {}", metadata.FilePath.string());
				UpdateDependencies(assetHandle);
				Application::Get().DispatchEvent<AssetReloadedEvent, /*DispatchImmediately=*/true>(assetHandle);
			}
			else
			{
				HZ_CORE_ERROR_TAG("AssetManager", "Failed to reload asset {}", metadata.FilePath.string());
			}
		}

		return metadata.IsDataLoaded;
	}

	void EditorAssetManager::ReloadDataAsync(AssetHandle assetHandle)
	{
#if ASYNC_ASSETS
		// Queue load (if not already)
		auto metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
		{
			HZ_CORE_ERROR("Trying to reload invalid asset");
			return;
		}

		if (metadata.Status != AssetStatus::Loading)
		{
			m_AssetThread->QueueAssetLoad(metadata);
			metadata.Status = AssetStatus::Loading;
			SetMetadata(assetHandle, metadata);
		}
#else
		ReloadData(assetHandle);
#endif
	}

	// Returns true if asset was reloaded
	bool EditorAssetManager::EnsureCurrent(AssetHandle assetHandle)
	{
		const auto& metadata = GetMetadata(assetHandle);
		auto absolutePath = GetFileSystemPath(metadata);

		if (!FileSystem::Exists(absolutePath))
			return false;

		uint64_t actualLastWriteTime = FileSystem::GetLastWriteTime(absolutePath);
		uint64_t recordedLastWriteTime = metadata.FileLastWriteTime;

		if (actualLastWriteTime == recordedLastWriteTime)
			return false;

		return ReloadData(assetHandle);
	}

	bool EditorAssetManager::EnsureAllLoadedCurrent()
	{
		HZ_PROFILE_FUNC();

		bool loaded = false;
		for (const auto& [handle, asset] : m_LoadedAssets)
		{
			loaded |= EnsureCurrent(handle);
		}
		return loaded;
	}

	Ref<Hazel::Asset> EditorAssetManager::GetMemoryAsset(AssetHandle handle)
	{
		std::shared_lock lock(m_MemoryAssetsMutex);
		if (auto it = m_MemoryAssets.find(handle); it != m_MemoryAssets.end())
			return it->second;

		return nullptr;
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle handle)
	{
		return m_LoadedAssets.contains(handle);
	}

	bool EditorAssetManager::IsAssetValid(AssetHandle handle)
	{
		HZ_PROFILE_FUNC();
		HZ_SCOPE_PERF("AssetManager::IsAssetValid");

		auto asset = GetAssetIncludingInvalid(handle);
		return asset && asset->IsValid();
	}

	bool EditorAssetManager::IsAssetMissing(AssetHandle handle)
	{
		HZ_PROFILE_FUNC();
		HZ_SCOPE_PERF("AssetManager::IsAssetMissing");

		if(GetMemoryAsset(handle)) 
			return false;

		auto metadata = GetMetadata(handle);
		return !FileSystem::Exists(Project::GetActive()->GetAssetDirectory() / metadata.FilePath);
	}

	bool EditorAssetManager::IsMemoryAsset(AssetHandle handle)
	{
		std::scoped_lock lock(m_MemoryAssetsMutex);
		return m_MemoryAssets.contains(handle);
	}

	bool EditorAssetManager::IsPhysicalAsset(AssetHandle handle)
	{
		return !IsMemoryAsset(handle);
	}

	void EditorAssetManager::RemoveAsset(AssetHandle handle)
	{
		{
			std::scoped_lock lock(m_MemoryAssetsMutex);
			if (m_MemoryAssets.contains(handle))
				m_MemoryAssets.erase(handle);
		}

		if (m_LoadedAssets.contains(handle))
			m_LoadedAssets.erase(handle);

		{
			std::scoped_lock lock(m_AssetRegistryMutex);
			if (m_AssetRegistry.Contains(handle))
				m_AssetRegistry.Remove(handle);
		}
	}

	void EditorAssetManager::RegisterDependency(AssetHandle handle, AssetHandle dependency)
	{
		HZ_CORE_VERIFY(IsAssetHandleValid(handle));  // IsAssetHandleValid() is relatively expensive. This might be better as an ASSERT
		//HZ_CORE_VERIFY(IsAssetHandleValid(dependency));  // dependency may not actually be in the manager yet (e.g. we are still in the middle of setting it up)

		std::scoped_lock lock(m_AssetDependenciesMutex);
		m_AssetDependencies[handle].insert(dependency);
	}

	void EditorAssetManager::UpdateDependencies(AssetHandle handle)
	{
		std::unordered_set<AssetHandle> dependencies;
		{
			std::shared_lock lock(m_AssetDependenciesMutex);
			if (auto it = m_AssetDependencies.find(handle); it != m_AssetDependencies.end())
				dependencies = it->second;
		}
		for (AssetHandle dependency : dependencies)
		{
			Ref<Asset> asset = GetAsset(dependency);
			if (asset)
			{
				asset->OnDependencyUpdated(handle);
			}
		}
	}

	void EditorAssetManager::SyncWithAssetThread()
	{
#if ASYNC_ASSETS
		std::vector<EditorAssetLoadResponse> freshAssets;

		m_AssetThread->RetrieveReadyAssets(freshAssets);
		for (auto& alr : freshAssets)
		{
			HZ_CORE_ASSERT(alr.Asset->Handle == alr.Metadata.Handle, "AssetHandle mismatch in AssetLoadResponse");
			m_LoadedAssets[alr.Metadata.Handle] = alr.Asset;
			alr.Metadata.Status = AssetStatus::Ready;
			alr.Metadata.IsDataLoaded = true;
			SetMetadata(alr.Metadata.Handle, alr.Metadata);
		}

		m_AssetThread->UpdateLoadedAssetList(m_LoadedAssets);

		// Update dependencies after syncing everything
		for (const auto& alr : freshAssets)
		{
			UpdateDependencies(alr.Metadata.Handle);
		}
#else
		Application::Get().SyncEvents();
#endif
	}

	AssetHandle EditorAssetManager::ImportAsset(const std::filesystem::path& filepath)
	{
		std::filesystem::path path = GetRelativePath(filepath);

		if (auto& metadata = GetMetadata(path); metadata.IsValid())
			return metadata.Handle;

		AssetType type = GetAssetTypeFromPath(path);
		if (type == AssetType::None)
			return 0;

		AssetMetadata metadata;
		metadata.Handle = AssetHandle();
		metadata.FilePath = path;
		metadata.Type = type;

		auto absolutePath = GetFileSystemPath(metadata);
		metadata.FileLastWriteTime = FileSystem::GetLastWriteTime(absolutePath);
		SetMetadata(metadata.Handle, metadata);

		return metadata.Handle;
	}

	Ref<Asset> EditorAssetManager::GetAssetIncludingInvalid(AssetHandle assetHandle)
	{
		if (auto asset = GetMemoryAsset(assetHandle); asset)
			return asset;

		Ref<Asset> asset = nullptr;
		auto metadata = GetMetadata(assetHandle);
		if (metadata.IsValid())
		{
			if (metadata.IsDataLoaded)
			{
				asset = m_LoadedAssets[assetHandle];
			}
			else
			{
				if (Application::IsMainThread())
				{
					// If we're main thread, we can just try loading the asset as normal
					HZ_CORE_INFO_TAG("AssetManager", "LOADING ASSET - {}", metadata.FilePath.string());
					if (AssetImporter::TryLoadData(metadata, asset))
					{
						auto metadataLoaded = metadata;
						metadataLoaded.IsDataLoaded = true;
						auto absolutePath = GetFileSystemPath(metadata);
						metadataLoaded.FileLastWriteTime = FileSystem::GetLastWriteTime(absolutePath);
						m_LoadedAssets[assetHandle] = asset;
						SetMetadata(assetHandle, metadataLoaded);
						HZ_CORE_INFO_TAG("AssetManager", "Finished loading asset {}", metadata.FilePath.string());
					}
					else
					{
						HZ_CORE_ERROR_TAG("AssetManager", "Failed to load asset {}", metadata.FilePath.string());
					}
				}
				else
				{
					// Not main thread -> ask AssetThread for the asset
					// If the asset needs to be loaded, this will load the asset.
					// The load will happen on this thread (which is probably asset thread, but occasionally might be audio thread).
					// The asset will get synced into main thread at next asset sync point.
					asset = m_AssetThread->GetAsset(metadata);
				}
			}
		}
		return asset;
	}

	void EditorAssetManager::LoadAssetRegistry()
	{
		HZ_CORE_INFO("[AssetManager] Loading Asset Registry");

		const auto& assetRegistryPath = Project::GetAssetRegistryPath();
		if (!FileSystem::Exists(assetRegistryPath))
			return;

		std::ifstream stream(assetRegistryPath);
		HZ_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		auto handles = data["Assets"];
		if (!handles)
		{
			HZ_CORE_ERROR("[AssetManager] Asset Registry appears to be corrupted!");
			HZ_CORE_VERIFY(false);
			return;
		}

		for (auto entry : handles)
		{
			std::string filepath = entry["FilePath"].as<std::string>();

			AssetMetadata metadata;
			metadata.Handle = entry["Handle"].as<uint64_t>();
			metadata.FilePath = filepath;
			metadata.Type = (AssetType)Utils::AssetTypeFromString(entry["Type"].as<std::string>());

			if (metadata.Type == AssetType::None)
				continue;

			if (metadata.Type != GetAssetTypeFromPath(filepath))
			{
				HZ_CORE_WARN_TAG("AssetManager", "Mismatch between stored AssetType and extension type when reading asset registry!");
				metadata.Type = GetAssetTypeFromPath(filepath);
			}

			if (!FileSystem::Exists(GetFileSystemPath(metadata)))
			{
				HZ_CORE_WARN("[AssetManager] Missing asset '{0}' detected in registry file, trying to locate...", metadata.FilePath);

				std::string mostLikelyCandidate;
				uint32_t bestScore = 0;

				for (auto& pathEntry : std::filesystem::recursive_directory_iterator(Project::GetActiveAssetDirectory()))
				{
					const std::filesystem::path& path = pathEntry.path();

					if (path.filename() != metadata.FilePath.filename())
						continue;

					if (bestScore > 0)
						HZ_CORE_WARN("[AssetManager] Multiple candidates found...");

					std::vector<std::string> candiateParts = Utils::SplitString(path.string(), "/\\");

					uint32_t score = 0;
					for (const auto& part : candiateParts)
					{
						if (filepath.find(part) != std::string::npos)
							score++;
					}

					HZ_CORE_WARN("'{0}' has a score of {1}, best score is {2}", path.string(), score, bestScore);

					if (bestScore > 0 && score == bestScore)
					{
						// TODO: How do we handle this?
						// Probably prompt the user at this point?
					}

					if (score <= bestScore)
						continue;

					bestScore = score;
					mostLikelyCandidate = path.string();
				}

				if (mostLikelyCandidate.empty() && bestScore == 0)
				{
					HZ_CORE_ERROR("[AssetManager] Failed to locate a potential match for '{0}'", metadata.FilePath);
					continue;
				}

				std::replace(mostLikelyCandidate.begin(), mostLikelyCandidate.end(), '\\', '/');
				metadata.FilePath = std::filesystem::relative(mostLikelyCandidate, Project::GetActive()->GetAssetDirectory());
				HZ_CORE_WARN("[AssetManager] Found most likely match '{0}'", metadata.FilePath);
			}

			if (metadata.Handle == 0)
			{
				HZ_CORE_WARN("[AssetManager] AssetHandle for {0} is 0, this shouldn't happen.", metadata.FilePath);
				continue;
			}

			SetMetadata(metadata.Handle, metadata);
		}

		HZ_CORE_INFO("[AssetManager] Loaded {0} asset entries", m_AssetRegistry.Count());
	}

	void EditorAssetManager::ProcessDirectory(const std::filesystem::path& directoryPath)
	{
		for (auto entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
				ProcessDirectory(entry.path());
			else
				ImportAsset(entry.path());
		}
	}

	void EditorAssetManager::ReloadAssets()
	{
		ProcessDirectory(Project::GetActiveAssetDirectory().string());
		WriteRegistryToFile();
	}

	void EditorAssetManager::WriteRegistryToFile()
	{
		// Sort assets by UUID to make project management easier
		struct AssetRegistryEntry
		{
			std::string FilePath;
			AssetType Type;
		};
		std::map<UUID, AssetRegistryEntry> sortedMap;
		for (auto& [filepath, metadata] : m_AssetRegistry)
		{
			if (!FileSystem::Exists(GetFileSystemPath(metadata)))
				continue;

			std::string pathToSerialize = metadata.FilePath.string();
			// NOTE(Yan): if Windows
			std::replace(pathToSerialize.begin(), pathToSerialize.end(), '\\', '/');
			sortedMap[metadata.Handle] = { pathToSerialize, metadata.Type };
		}

		HZ_CORE_INFO("[AssetManager] serializing asset registry with {0} entries", sortedMap.size());

		YAML::Emitter out;
		out << YAML::BeginMap;

		out << YAML::Key << "Assets" << YAML::BeginSeq;
		for (auto& [handle, entry] : sortedMap)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Handle" << YAML::Value << handle;
			out << YAML::Key << "FilePath" << YAML::Value << entry.FilePath;
			out << YAML::Key << "Type" << YAML::Value << Utils::AssetTypeToString(entry.Type);
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		const std::string& assetRegistryPath = Project::GetAssetRegistryPath().string();
		std::ofstream fout(assetRegistryPath);
		fout << out.c_str();
	}

	void EditorAssetManager::OnAssetRenamed(AssetHandle assetHandle, const std::filesystem::path& newFilePath)
	{
		AssetMetadata metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
			return;

		metadata.FilePath = GetRelativePath(newFilePath);
		SetMetadata(assetHandle, metadata);
		WriteRegistryToFile();
	}

	void EditorAssetManager::OnAssetDeleted(AssetHandle assetHandle)
	{
		RemoveAsset(assetHandle);
		WriteRegistryToFile();
	}

}

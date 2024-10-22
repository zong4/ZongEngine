#include "hzpch.h"
#include "EditorAssetManager.h"

#include "yaml-cpp/yaml.h"

#include "Hazel/Asset/AssetExtensions.h"
#include "Hazel/Utilities/StringUtils.h"

#include "Hazel/Project/Project.h"

#include "Hazel/Debug/Profiler.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Core/Timer.h"

namespace Hazel {

	static AssetMetadata s_NullMetadata;

	EditorAssetManager::EditorAssetManager()
	{
		AssetImporter::Init();

		LoadAssetRegistry();
		ReloadAssets();
	}

	EditorAssetManager::~EditorAssetManager()
	{
		WriteRegistryToFile();
	}

	AssetType EditorAssetManager::GetAssetType(AssetHandle assetHandle)
	{
		if (!IsAssetHandleValid(assetHandle))
			return AssetType::None;

		const auto& metadata = GetMetadata(assetHandle);
		return metadata.Type;
	}

	Ref<Asset> EditorAssetManager::GetAsset(AssetHandle assetHandle)
	{
		HZ_PROFILE_FUNC();
		HZ_SCOPE_PERF("AssetManager::GetAsset");

		if (IsMemoryAsset(assetHandle))
			return m_MemoryAssets[assetHandle];

		auto& metadata = GetMetadataInternal(assetHandle);
		if (!metadata.IsValid())
			return nullptr;

		Ref<Asset> asset = nullptr;
		if (!metadata.IsDataLoaded)
		{
			metadata.IsDataLoaded = AssetImporter::TryLoadData(metadata, asset);
			if (!metadata.IsDataLoaded)
				return nullptr;

			m_LoadedAssets[assetHandle] = asset;
		}
		else
		{
			asset = m_LoadedAssets[assetHandle];
		}

		return asset;
	}

	void EditorAssetManager::AddMemoryOnlyAsset(Ref<Asset> asset)
	{
		AssetMetadata metadata;
		metadata.Handle = asset->Handle;
		metadata.IsDataLoaded = true;
		metadata.Type = asset->GetAssetType();
		metadata.IsMemoryAsset = true;
		m_AssetRegistry[metadata.Handle] = metadata;

		m_MemoryAssets[asset->Handle] = asset;
	}

	std::unordered_set<AssetHandle> EditorAssetManager::GetAllAssetsWithType(AssetType type)
	{
		std::unordered_set<AssetHandle> result;
		for (const auto& [handle, metadata] : m_AssetRegistry)
		{
			if (metadata.Type == type)
				result.insert(handle);
		}
		return result;
	}

	const AssetMetadata& EditorAssetManager::GetMetadata(AssetHandle handle)
	{
		if (m_AssetRegistry.Contains(handle))
			return m_AssetRegistry[handle];

		return s_NullMetadata;
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

	const AssetMetadata& EditorAssetManager::GetMetadata(const Ref<Asset>& asset)
	{
		return GetMetadata(asset->Handle);
	}

	AssetMetadata& EditorAssetManager::GetMutableMetadata(AssetHandle handle)
	{
		if (m_AssetRegistry.Contains(handle))
			return m_AssetRegistry[handle];

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

	AssetType EditorAssetManager::GetAssetTypeFromPath(const std::filesystem::path& path)
	{
		return GetAssetTypeFromExtension(path.extension().string());
	}

	std::filesystem::path EditorAssetManager::GetFileSystemPath(const AssetMetadata& metadata)
	{
		return Project::GetAssetDirectory() / metadata.FilePath;
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
		if (temp.find(Project::GetAssetDirectory().string()) != std::string::npos)
		{
			relativePath = std::filesystem::relative(filepath, Project::GetAssetDirectory());
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
		auto& metadata = GetMetadataInternal(assetHandle);
		if (!metadata.IsValid())
		{
			HZ_CORE_ERROR("Trying to reload invalid asset");
			return false;
		}

		Ref<Asset> asset;
		metadata.IsDataLoaded = AssetImporter::TryLoadData(metadata, asset);
		if (metadata.IsDataLoaded)
		{
			m_LoadedAssets[assetHandle] = asset;
		}
		return metadata.IsDataLoaded;
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle handle)
	{
		return m_LoadedAssets.contains(handle);
	}

	void EditorAssetManager::RemoveAsset(AssetHandle handle)
	{
		if (m_LoadedAssets.contains(handle))
			m_LoadedAssets.erase(handle);

		if (m_MemoryAssets.contains(handle))
			m_MemoryAssets.erase(handle);

		if (m_AssetRegistry.Contains(handle))
			m_AssetRegistry.Remove(handle);
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
		m_AssetRegistry[metadata.Handle] = metadata;

		return metadata.Handle;
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

				for (auto& pathEntry : std::filesystem::recursive_directory_iterator(Project::GetAssetDirectory()))
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

			m_AssetRegistry[metadata.Handle] = metadata;
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
		ProcessDirectory(Project::GetAssetDirectory().string());
		WriteRegistryToFile();
	}

	void EditorAssetManager::WriteRegistryToFile()
	{
		// Sort assets by UUID to make project managment easier
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

			if (metadata.IsMemoryAsset)
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

	AssetMetadata& EditorAssetManager::GetMetadataInternal(AssetHandle handle)
	{
		if (m_AssetRegistry.Contains(handle))
			return m_AssetRegistry[handle];

		return s_NullMetadata; // make sure to check return value before you go changing it!
	}

	void EditorAssetManager::OnAssetRenamed(AssetHandle assetHandle, const std::filesystem::path& newFilePath)
	{
		AssetMetadata& metadata = m_AssetRegistry[assetHandle];
		if (!metadata.IsValid())
			return;

		metadata.FilePath = GetRelativePath(newFilePath);
		WriteRegistryToFile();
	}

	void EditorAssetManager::OnAssetDeleted(AssetHandle assetHandle)
	{
		AssetMetadata metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
			return;

		m_AssetRegistry.Remove(assetHandle);
		m_LoadedAssets.erase(assetHandle);
		WriteRegistryToFile();
	}

}

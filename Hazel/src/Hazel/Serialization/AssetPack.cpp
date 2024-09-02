#include "hzpch.h"
#include "AssetPack.h"

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Audio/SoundObject.h"
#include "Hazel/Core/Platform.h"
#include "Hazel/Scene/Scene.h"
#include "Hazel/Scene/SceneSerializer.h"
#include "Hazel/Scene/Prefab.h"

#include "Hazel/Audio/AudioEvents/AudioCommandRegistry.h"
#include "Hazel/Editor/NodeGraphEditor/SoundGraph/SoundGraphAsset.h"

namespace Hazel {
	
	AssetPack::AssetPack(const std::filesystem::path& path)
		: m_Path(path)
	{
	}

	void AssetPack::AddAsset(Ref<Asset> asset)
	{
#if 0
		if (!asset)
			return;
		HZ_CORE_ASSERT(asset);

		AssetHandle handle = asset->Handle;

		auto& assetMap = m_File.Index.Assets;
		if (assetMap.find(handle) != assetMap.end())
		{
			HZ_CORE_WARN_TAG("AssetPack", "Asset already present in asset pack");
			return;
		}

		AssetPackFile::AssetInfo& info = assetMap[handle];
		info.Type = (uint16_t)asset->GetAssetType();
#endif
	}

	void AssetPack::Serialize()
	{
		//m_Serializer.Serialize(m_Path, m_File);
	}

	Ref<Scene> AssetPack::LoadScene(AssetHandle sceneHandle)
	{
		auto it = m_File.Index.Scenes.find(sceneHandle);
		if (it == m_File.Index.Scenes.end())
			return nullptr;

		const AssetPackFile::SceneInfo& sceneInfo = it->second;

		FileStreamReader stream(m_Path);
		Ref<Scene> scene = AssetImporter::DeserializeSceneFromAssetPack(stream, sceneInfo);
		scene->Handle = sceneHandle;
		return scene;
	}

	Ref<Asset> AssetPack::LoadAsset(AssetHandle sceneHandle, AssetHandle assetHandle)
	{
		const AssetPackFile::AssetInfo* assetInfo = nullptr;

		bool foundAsset = false;
		if (sceneHandle)
		{
			// Fast(er) path
			auto it = m_File.Index.Scenes.find(sceneHandle);
			if (it != m_File.Index.Scenes.end())
			{
				const AssetPackFile::SceneInfo& sceneInfo = it->second;
				auto assetIt = sceneInfo.Assets.find(assetHandle);
				if (assetIt != sceneInfo.Assets.end())
				{
					foundAsset = true;
					assetInfo = &assetIt->second;
				}
			}
		}

		if (!foundAsset)
		{
			// Slow(er) path
			for (const auto& [handle, sceneInfo] : m_File.Index.Scenes)
			{
				auto assetIt = sceneInfo.Assets.find(assetHandle);
				if (assetIt != sceneInfo.Assets.end())
				{
					assetInfo = &assetIt->second;
					break;
				}
			}

			if (!assetInfo)
				return nullptr;
		}

		FileStreamReader stream(m_Path);
		Ref<Asset> asset = AssetImporter::DeserializeFromAssetPack(stream, *assetInfo);
		//HZ_CORE_VERIFY(asset);
		if (!asset)
			return nullptr;

		asset->Handle = assetHandle;
		return asset;
	}

	bool AssetPack::IsAssetHandleValid(AssetHandle assetHandle) const
	{
		return m_AssetHandleIndex.find(assetHandle) != m_AssetHandleIndex.end();
	}

	bool AssetPack::IsAssetHandleValid(AssetHandle sceneHandle, AssetHandle assetHandle) const
	{
		auto sceneIterator = m_File.Index.Scenes.find(sceneHandle);
		if (sceneIterator == m_File.Index.Scenes.end())
			return false;

		const auto& sceneInfo = sceneIterator->second;
		return sceneInfo.Assets.find(assetHandle) != sceneInfo.Assets.end();
	}

	Buffer AssetPack::ReadAppBinary()
	{
		FileStreamReader stream(m_Path);
		stream.SetStreamPosition(m_File.Index.PackedAppBinaryOffset);
		Buffer buffer;
		stream.ReadBuffer(buffer);
		HZ_CORE_VERIFY(m_File.Index.PackedAppBinarySize == (buffer.Size + sizeof(uint64_t)));
		return buffer;
	}

	uint64_t AssetPack::GetBuildVersion()
	{
		return m_File.Header.BuildVersion;
	}

	AssetType AssetPack::GetAssetType(AssetHandle sceneHandle, AssetHandle assetHandle) const
	{
		const AssetPackFile::AssetInfo* assetInfo = nullptr;

		bool foundAsset = false;
		if (sceneHandle)
		{
			// Fast(er) path
			auto it = m_File.Index.Scenes.find(sceneHandle);
			if (it != m_File.Index.Scenes.end())
			{
				const AssetPackFile::SceneInfo& sceneInfo = it->second;
				auto assetIt = sceneInfo.Assets.find(assetHandle);
				if (assetIt != sceneInfo.Assets.end())
				{
					foundAsset = true;
					assetInfo = &assetIt->second;
				}
			}
		}

		if (!foundAsset)
		{
			// Slow(er) path
			for (const auto& [handle, sceneInfo] : m_File.Index.Scenes)
			{
				auto assetIt = sceneInfo.Assets.find(assetHandle);
				if (assetIt != sceneInfo.Assets.end())
				{
					assetInfo = &assetIt->second;
					break;
				}
			}

			if (!assetInfo)
				return AssetType::None;
		}

		return (AssetType)assetInfo->Type;
	}

	Ref<AssetPack> AssetPack::CreateFromActiveProject(std::atomic<float>& progress)
	{
#define DEBUG_PRINT 1

		// Need to find all scenes and see which assets they use

		AssetPackFile assetPackFile;
		assetPackFile.Header.BuildVersion = Platform::GetCurrentDateTimeU64();

		progress = 0.0f;

		std::unordered_set<AssetHandle> fullAssetList;

		// Note: user could create more scenes on main thread while asset pack thread is busy serializing these ones!
		std::unordered_set<AssetHandle> sceneHandles = AssetManager::GetAllAssetsWithType<Scene>();
		uint32_t sceneCount = (uint32_t)sceneHandles.size();

		if (sceneCount == 0)
		{
			HZ_CONSOLE_LOG_ERROR("There are no scenes in the project.  Nothing to serialize to asset pack!");
			return nullptr;
		}

		float progressIncrement = 0.5f / (float)sceneCount;

		// Audio Command Registry
		std::unordered_set<AssetHandle> audioAssets = AudioCommandRegistry::GetAllAssets(); // Note: not thread-safe!
		fullAssetList.insert(audioAssets.begin(), audioAssets.end());

		// Sound Graphs
		std::unordered_set<AssetHandle> soundGraphs = AssetManager::GetAllAssetsWithType<SoundGraphAsset>();
		fullAssetList.insert(soundGraphs.begin(), soundGraphs.end());
		
		// Audio "files"
		std::unordered_set<AssetHandle> audioFiles = AssetManager::GetAllAssetsWithType<AudioFile>();
		fullAssetList.insert(audioFiles.begin(), audioFiles.end());

		for (const auto sceneHandle : sceneHandles)
		{
			const auto metadata = Project::GetEditorAssetManager()->GetMetadata(sceneHandle);

			Ref<Scene> scene = Ref<Scene>::Create("AssetPack", true, false);
			SceneSerializer serializer(scene);
			HZ_CORE_TRACE("Deserializing Scene: {}", metadata.FilePath);
			if (serializer.Deserialize(Project::GetActiveAssetDirectory() / metadata.FilePath))
			{
				std::unordered_set<AssetHandle> sceneAssetList = scene->GetAssetList();
				HZ_CORE_TRACE("  Scene has {} used assets", sceneAssetList.size());

				std::unordered_set<AssetHandle> sceneAssetListWithoutPrefabs = sceneAssetList;
				for (AssetHandle assetHandle : sceneAssetListWithoutPrefabs)
				{
					AssetType type = AssetManager::GetAssetType(assetHandle);
					if (type == AssetType::Prefab)
					{
						Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(assetHandle);
						std::unordered_set<AssetHandle> childPrefabAssetList = prefab->GetAssetList(true);  // Note: not thread-safe!
						sceneAssetList.insert(childPrefabAssetList.begin(), childPrefabAssetList.end());
					}
				}

				sceneAssetList.insert(audioAssets.begin(), audioAssets.end());
				sceneAssetList.insert(soundGraphs.begin(), soundGraphs.end());
				sceneAssetList.insert(audioFiles.begin(), audioFiles.end());

				AssetPackFile::SceneInfo& sceneInfo = assetPackFile.Index.Scenes[sceneHandle];
				for (AssetHandle assetHandle : sceneAssetList)
				{
					AssetPackFile::AssetInfo& assetInfo = sceneInfo.Assets[assetHandle];
					assetInfo.Type = (uint16_t)AssetManager::GetAssetType(assetHandle);
				}

				fullAssetList.insert(sceneAssetList.begin(), sceneAssetList.end());
			}
			else
			{
				HZ_CONSOLE_LOG_ERROR("Failed to deserialize Scene: {} ({})", metadata.FilePath, sceneHandle);
			}
			progress = progress + progressIncrement;
		}

#if 0
		// Make sure all Prefab-referenced assets are included
		for (AssetHandle handle : fullAssetList)
		{
			const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(handle);
			if (metadata.Type == AssetType::Prefab)
			{
				Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(handle);
				std::unordered_set<AssetHandle> childPrefabAssetList = prefab->GetAssetList(true);
				fullAssetList.insert(childPrefabAssetList.begin(), childPrefabAssetList.end());
			}
		}
#endif

		HZ_CONSOLE_LOG_INFO("Project contains {} used assets", fullAssetList.size());

#if DEBUG_PRINT
		HZ_CORE_TRACE("Complete AssetPack:");

		for (AssetHandle handle : fullAssetList)
		{
			const auto metadata = Project::GetEditorAssetManager()->GetMetadata(handle);
			
			bool isMemory = AssetManager::IsMemoryAsset(handle);
			HZ_CORE_TRACE("{}: {} ({}{})", Utils::AssetTypeToString(AssetManager::GetAssetType(handle)), handle,
				isMemory ? "Memory" : "Physical: ", isMemory ? "" : metadata.FilePath);
		}
#endif

		Buffer appBinary;
		
		if (std::filesystem::exists(Project::GetScriptModuleFilePath()))
			appBinary = FileSystem::ReadBytes(Project::GetScriptModuleFilePath());

		AssetPackSerializer::Serialize(Project::GetActiveAssetDirectory() / "AssetPack.hap", assetPackFile, appBinary, progress);
		progress = 1.0f;

		std::unordered_map<AssetHandle, AssetPackFile::AssetInfo> serializedAssets;
		for (auto& [sceneHandle, sceneInfo] : assetPackFile.Index.Scenes)
		{
			for (auto& [assetHandle, assetInfo] : sceneInfo.Assets)
			{
				if (serializedAssets.find(assetHandle) == serializedAssets.end())
				{
					serializedAssets[assetHandle] = assetInfo;
				}
			}
		}

		HZ_CORE_TRACE_TAG("Asset Pack", "Serialized Assets:");
		for (const auto& [handle, info] : serializedAssets)
		{
			const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(handle);
			HZ_CORE_TRACE_TAG("Asset Pack", "{}: {} (offset = {}, size = {})", Utils::AssetTypeToString(metadata.Type), metadata.FilePath, info.PackedOffset, info.PackedSize);
		}

		return nullptr;
	}

	Ref<AssetPack> AssetPack::LoadActiveProject()
	{
		return Load(Project::GetActiveAssetDirectory() / "AssetPack.hap");
	}

	Ref<AssetPack> AssetPack::Load(const std::filesystem::path& path)
	{
		Ref<AssetPack> assetPack = Ref<AssetPack>::Create();
		assetPack->m_Path = path;
		bool success = AssetPackSerializer::DeserializeIndex(assetPack->m_Path, assetPack->m_File);
		HZ_CORE_VERIFY(success);
		if (!success)
			return nullptr;

		// Populate asset handle index
		const auto& index = assetPack->m_File.Index;
		for (const auto& [sceneHandle, sceneInfo] : index.Scenes)
		{
			assetPack->m_AssetHandleIndex.insert(sceneHandle);
			for (const auto& [assetHandle, assetInfo] : sceneInfo.Assets)
			{
				assetPack->m_AssetHandleIndex.insert(assetHandle);
			}
		}

		// Debug log
#ifndef HZ_DIST
		{
			HZ_CORE_INFO_TAG("Asset Pack", "-----------------------------------------------------");
			HZ_CORE_INFO_TAG("Asset Pack", "AssetPack Dump {}", assetPack->m_Path);
			HZ_CORE_INFO_TAG("Asset Pack", "-----------------------------------------------------");
			std::unordered_map<AssetType, uint32_t> typeCounts;
			std::unordered_set<AssetHandle> duplicatePreventionSet;
			for (const auto& [sceneHandle, sceneInfo] : index.Scenes)
			{
				HZ_CORE_INFO_TAG("Asset Pack", "Scene {}:", sceneHandle);
				for (const auto& [assetHandle, assetInfo] : sceneInfo.Assets)
				{
					HZ_CORE_INFO_TAG("Asset Pack", "  {} - {}", Utils::AssetTypeToString((AssetType)assetInfo.Type), assetHandle);

					if (duplicatePreventionSet.find(assetHandle) == duplicatePreventionSet.end())
					{
						duplicatePreventionSet.insert(assetHandle);
						typeCounts[(AssetType)assetInfo.Type]++;
					}
				}
			}
			HZ_CORE_INFO_TAG("Asset Pack", "-----------------------------------------------------");
			HZ_CORE_INFO_TAG("Asset Pack", "Summary:");
			for (const auto& [type, count] : typeCounts)
			{
				HZ_CORE_INFO_TAG("Asset Pack", "  {} {}", count, Utils::AssetTypeToString(type));
			}
			HZ_CORE_INFO_TAG("Asset Pack", "-----------------------------------------------------");
		}
#endif

		return assetPack;
	}

}

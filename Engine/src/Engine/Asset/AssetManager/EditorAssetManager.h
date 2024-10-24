#pragma once

#include "Engine/Core/Hash.h"
#include "AssetManagerBase.h"

#include "Engine/Asset/AssetImporter.h"
#include "Engine/Asset/AssetRegistry.h"

#include "Engine/Utilities/FileSystem.h"

namespace Engine {

	class EditorAssetManager : public AssetManagerBase
	{
	public:
		EditorAssetManager();
		virtual ~EditorAssetManager();

		virtual AssetType GetAssetType(AssetHandle assetHandle) override;
		virtual Ref<Asset> GetAsset(AssetHandle assetHandle) override;
		virtual void AddMemoryOnlyAsset(Ref<Asset> asset) override;

		virtual std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type) override;
		virtual const std::unordered_map<AssetHandle, Ref<Asset>>& GetLoadedAssets() override { return m_LoadedAssets; }
		virtual const std::unordered_map<AssetHandle, Ref<Asset>>& GetMemoryOnlyAssets() override { return m_MemoryAssets; }

		// Editor-only
		const AssetMetadata& GetMetadata(AssetHandle handle);
		AssetMetadata& GetMutableMetadata(AssetHandle handle);
		const AssetMetadata& GetMetadata(const std::filesystem::path& filepath);
		const AssetMetadata& GetMetadata(const Ref<Asset>& asset);

		AssetHandle ImportAsset(const std::filesystem::path& filepath);

		AssetHandle GetAssetHandleFromFilePath(const std::filesystem::path& filepath);

		AssetType GetAssetTypeFromExtension(const std::string& extension);
		AssetType GetAssetTypeFromPath(const std::filesystem::path& path);

		std::filesystem::path GetFileSystemPath(AssetHandle handle);
		std::filesystem::path GetFileSystemPath(const AssetMetadata& metadata);
		std::string GetFileSystemPathString(const AssetMetadata& metadata);
		std::filesystem::path GetRelativePath(const std::filesystem::path& filepath);

		bool FileExists(AssetMetadata& metadata) const;

		virtual bool ReloadData(AssetHandle assetHandle) override;
		virtual bool IsAssetHandleValid(AssetHandle assetHandle) override { return IsMemoryAsset(assetHandle) || GetMetadata(assetHandle).IsValid(); }
		virtual bool IsMemoryAsset(AssetHandle handle) override { return m_MemoryAssets.find(handle) != m_MemoryAssets.end(); }
		virtual bool IsAssetLoaded(AssetHandle handle) override;
		virtual void RemoveAsset(AssetHandle handle) override;

		const AssetRegistry& GetAssetRegistry() const { return m_AssetRegistry; }

		template<typename T, typename... Args>
		Ref<T> CreateNewAsset(const std::string& filename, const std::string& directoryPath, Args&&... args)
		{
			static_assert(std::is_base_of<Asset, T>::value, "CreateNewAsset only works for types derived from Asset");

			AssetMetadata metadata;
			metadata.Handle = AssetHandle();
			if (directoryPath.empty() || directoryPath == ".")
				metadata.FilePath = filename;
			else
				metadata.FilePath = GetRelativePath(directoryPath + "/" + filename);
			metadata.IsDataLoaded = true;
			metadata.Type = T::GetStaticType();

			/*if (FileExists(metadata))
			{
				bool foundAvailableFileName = false;
				int current = 1;

				while (!foundAvailableFileName)
				{
					std::string nextFilePath = directoryPath + "/" + metadata.FilePath.stem().string();

					if (current < 10)
						nextFilePath += " (0" + std::to_string(current) + ")";
					else
						nextFilePath += " (" + std::to_string(current) + ")";
					nextFilePath += metadata.FilePath.extension().string();

					if (!FileSystem::Exists(Project::GetAssetDirectory() / GetRelativePath(nextFilePath)))
					{
						foundAvailableFileName = true;
						metadata.FilePath = GetRelativePath(nextFilePath);
						break;
					}

					current++;
				}
			}*/

			m_AssetRegistry[metadata.Handle] = metadata;

			WriteRegistryToFile();

			Ref<T> asset = Ref<T>::Create(std::forward<Args>(args)...);
			asset->Handle = metadata.Handle;
			m_LoadedAssets[asset->Handle] = asset;
			AssetImporter::Serialize(metadata, asset);

			return asset;
		}

		template<typename TAsset, typename... TArgs>
		AssetHandle CreateNamedMemoryOnlyAsset(const std::string& name, TArgs&&... args)
		{
			static_assert(std::is_base_of<Asset, TAsset>::value, "CreateMemoryOnlyAsset only works for types derived from Asset");

			Ref<TAsset> asset = Ref<TAsset>::Create(std::forward<TArgs>(args)...);
			asset->Handle = Hash::GenerateFNVHash(name);

			AssetMetadata metadata;
			metadata.Handle = asset->Handle;
			metadata.FilePath = name;
			metadata.IsDataLoaded = true;
			metadata.Type = TAsset::GetStaticType();
			metadata.IsMemoryAsset = true;

			m_AssetRegistry[metadata.Handle] = metadata;

			m_MemoryAssets[asset->Handle] = asset;
			return asset->Handle;
		}

		template<typename T>
		Ref<T> GetAsset(const std::string& filepath)
		{
			Ref<Asset> asset = GetAsset(GetAssetHandleFromFilePath(filepath));
			return asset.As<T>();
		}
	private:
		void LoadAssetRegistry();
		void ProcessDirectory(const std::filesystem::path& directoryPath);
		void ReloadAssets();
		void WriteRegistryToFile();

		AssetMetadata& GetMetadataInternal(AssetHandle handle);

		void OnAssetRenamed(AssetHandle assetHandle, const std::filesystem::path& newFilePath);
		void OnAssetDeleted(AssetHandle assetHandle);
	private:
		std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;
		std::unordered_map<AssetHandle, Ref<Asset>> m_MemoryAssets;
		AssetRegistry m_AssetRegistry;

		friend class ContentBrowserPanel;
		friend class ContentBrowserAsset;
		friend class ContentBrowserDirectory;
	};
}

#pragma once

#include "AssetManagerBase.h"

#include "Hazel/Asset/AssetImporter.h"
#include "Hazel/Asset/AssetRegistry.h"
#include "Hazel/Asset/AssetSystem/EditorAssetSystem.h"
#include "Hazel/Utilities/FileSystem.h"

#include <shared_mutex>

namespace Hazel {

	class EditorAssetManager : public AssetManagerBase
	{
	public:
		EditorAssetManager();
		virtual ~EditorAssetManager();

		virtual void Shutdown() override;

		virtual AssetType GetAssetType(AssetHandle assetHandle) override;
		virtual Ref<Asset> GetAsset(AssetHandle assetHandle) override;
		virtual AsyncAssetResult<Asset> GetAssetAsync(AssetHandle assetHandle) override;

		virtual void AddMemoryOnlyAsset(Ref<Asset> asset) override;

		virtual bool ReloadData(AssetHandle assetHandle) override;
		virtual void ReloadDataAsync(AssetHandle assetHandle) override;
		virtual bool EnsureCurrent(AssetHandle assetHandle) override;
		virtual bool EnsureAllLoadedCurrent() override;
		virtual bool IsAssetHandleValid(AssetHandle assetHandle) override { return GetMemoryAsset(assetHandle) || GetMetadata(assetHandle).IsValid(); }
		virtual Ref<Asset> GetMemoryAsset(AssetHandle handle) override;
		virtual bool IsAssetLoaded(AssetHandle handle) override;
		virtual bool IsAssetValid(AssetHandle handle) override;
		virtual bool IsAssetMissing(AssetHandle handle) override;
		virtual bool IsMemoryAsset(AssetHandle handle) override;
		virtual bool IsPhysicalAsset(AssetHandle handle) override;
		virtual void RemoveAsset(AssetHandle handle) override;

		virtual void RegisterDependency(AssetHandle handle, AssetHandle dependency) override;

		virtual void SyncWithAssetThread() override;

		virtual std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type) override;
		virtual const std::unordered_map<AssetHandle, Ref<Asset>>& GetLoadedAssets() override { return m_LoadedAssets; }

		// ------------- Editor-only ----------------

		const AssetRegistry& GetAssetRegistry() const { return m_AssetRegistry; }

		// Get all memory-only assets.
		// Returned by value so that caller need not hold a lock on m_MemoryAssetsMutex
		std::unordered_map<AssetHandle, Ref<Asset>> GetMemoryAssets();

		// note: GetMetadata(AssetHandle) is the ONLY EditorAssetManager function that it is safe to call
		//       from any thread.
		//       All other methods on EditorAssetManager are thread-unsafe and should only be called from the main thread.
		//       SetMetadata() must only be called from main-thread, otherwise it will break safety of all the other
		//       unsynchronized EditorAssetManager functions.
		//
		// thread-safe access to metadata
		// This function returns an AssetMetadata (specifically not a reference) as with references there is no guarantee
		// that the referred to data doesn't get modified (or even destroyed) by another thread
		AssetMetadata GetMetadata(AssetHandle handle);
		// note: do NOT add non-const version of GetMetadata().  For thread-safety you must modify through SetMetaData()

		// thread-safe modification of metadata
		// TODO (0x): don't really need the handle parameter since handle is in metadata anyway
		void SetMetadata(AssetHandle handle, const AssetMetadata& metadata);

		// non-thread safe access to metadata
		const AssetMetadata& GetMetadata(const std::filesystem::path& filepath);

		AssetHandle ImportAsset(const std::filesystem::path& filepath);

		AssetHandle GetAssetHandleFromFilePath(const std::filesystem::path& filepath);

		AssetType GetAssetTypeFromExtension(const std::string& extension);
		std::string GetDefaultExtensionForAssetType(AssetType type);
		AssetType GetAssetTypeFromPath(const std::filesystem::path& path);

		std::filesystem::path GetFileSystemPath(AssetHandle handle);
		std::filesystem::path GetFileSystemPath(const AssetMetadata& metadata);
		std::string GetFileSystemPathString(const AssetMetadata& metadata);
		std::filesystem::path GetRelativePath(const std::filesystem::path& filepath);

		bool FileExists(AssetMetadata& metadata) const;

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

					if (!FileSystem::Exists(Project::GetActiveAssetDirectory() / GetRelativePath(nextFilePath)))
					{
						foundAvailableFileName = true;
						metadata.FilePath = GetRelativePath(nextFilePath);
						break;
					}

					current++;
				}
			}*/

			SetMetadata(metadata.Handle, metadata);

			WriteRegistryToFile();

			Ref<T> asset = Ref<T>::Create(std::forward<Args>(args)...);
			asset->Handle = metadata.Handle;
			m_LoadedAssets[asset->Handle] = asset;
			AssetImporter::Serialize(metadata, asset);

			// Read serialized timestamp
			auto absolutePath = GetFileSystemPath(metadata);
			metadata.FileLastWriteTime = FileSystem::GetLastWriteTime(absolutePath);
			SetMetadata(metadata.Handle, metadata);

			return asset;
		}

		void ReplaceLoadedAsset(AssetHandle handle, Ref<Asset> newAsset)
		{
			m_LoadedAssets[handle] = newAsset;
		}

		template<typename T>
		Ref<T> GetAsset(const std::string& filepath)
		{
			// TODO(Emily): Does this path need to exist for Windows or is it
			// 				Vestigial?
#ifdef HZ_PLATFORM_WINDOWS
			Ref<Asset> asset = GetAsset(GetAssetHandleFromFilePath(filepath), false);
#else
			Ref<Asset> asset = GetAsset(GetAssetHandleFromFilePath(filepath));
#endif
			return asset.As<T>();
		}

	private:
		Ref<Asset> GetAssetIncludingInvalid(AssetHandle assetHandle);

		void LoadAssetRegistry();
		void ProcessDirectory(const std::filesystem::path& directoryPath);
		void ReloadAssets();
		void WriteRegistryToFile();

		void OnAssetRenamed(AssetHandle assetHandle, const std::filesystem::path& newFilePath);
		void OnAssetDeleted(AssetHandle assetHandle);

		void UpdateDependencies(AssetHandle handle);

	private:
		// TODO(Yan): move to AssetSystem
		// NOTE (0x): this collection is accessed only from the main thread, and so does not need
		//            any synchronization
		std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;

		// NOTE (0x): this collection is accessed and modified from both the main thread and
		//            the asset thread, and so requires synchronization
		std::unordered_map<AssetHandle, Ref<Asset>> m_MemoryAssets;
		std::shared_mutex m_MemoryAssetsMutex;
		
		std::unordered_map<AssetHandle, std::unordered_set<AssetHandle>> m_AssetDependencies;
		std::shared_mutex m_AssetDependenciesMutex;

		Ref<EditorAssetSystem> m_AssetThread;

		// Asset registry is accessed from multiple threads.
		// Access requires synchronization through m_AssetRegistryMutex
		// It is _written to_ only by main thread, so reading in main thread can be done without mutex
		AssetRegistry m_AssetRegistry;
		std::shared_mutex m_AssetRegistryMutex;

		friend class ContentBrowserPanel;
		friend class ContentBrowserAsset;
		friend class ContentBrowserDirectory;
		friend class EditorAssetSystem;
	};
}

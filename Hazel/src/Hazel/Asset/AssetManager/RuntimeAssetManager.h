#pragma once

#include "AssetManagerBase.h"

#include "Hazel/Asset/AssetSystem/RuntimeAssetSystem.h"
#include "Hazel/Serialization/AssetPack.h"

#include <shared_mutex>

namespace Hazel {

	// AssetPack
	class RuntimeAssetManager : public AssetManagerBase
	{
	public:
		RuntimeAssetManager();
		virtual ~RuntimeAssetManager();

		virtual void Shutdown() override {}

		virtual AssetType GetAssetType(AssetHandle assetHandle) override;
		virtual Ref<Asset> GetAsset(AssetHandle assetHandle) override;
		virtual AsyncAssetResult<Asset> GetAssetAsync(AssetHandle assetHandle) override;

		virtual void AddMemoryOnlyAsset(Ref<Asset> asset) override;
		virtual bool ReloadData(AssetHandle assetHandle) override;
		virtual void ReloadDataAsync(AssetHandle assetHandle) override;
		virtual bool EnsureAllLoadedCurrent() override;
		virtual bool EnsureCurrent(AssetHandle assetHandle) override;
		virtual bool IsAssetHandleValid(AssetHandle assetHandle) override;
		virtual Ref<Asset> GetMemoryAsset(AssetHandle handle) override;
		virtual bool IsAssetLoaded(AssetHandle handle) override;
		virtual bool IsAssetValid(AssetHandle handle) override;
		virtual bool IsAssetMissing(AssetHandle handle) override;
		virtual bool IsMemoryAsset(AssetHandle handle) override;
		virtual bool IsPhysicalAsset(AssetHandle handle) override;
		virtual void RemoveAsset(AssetHandle handle) override;

		virtual void RegisterDependency(AssetHandle handle, AssetHandle dependency) override {}

		virtual void SyncWithAssetThread() override;
		
		virtual std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type) override;
		virtual const std::unordered_map<AssetHandle, Ref<Asset>>& GetLoadedAssets() override { return m_LoadedAssets; }

		// ------------- Runtime-only ----------------

		Ref<Scene> LoadScene(AssetHandle handle);

		void SetAssetPack(Ref<AssetPack> assetPack);

	private:
		void UpdateDependencies(AssetHandle handle) {}

	private:
		// NOTE (0x): these collections are accessed only from the main thread, and so do not need
		//            any synchronization
		std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;
		std::unordered_set<AssetHandle> m_PendingAssets;

		// NOTE (0x): this collection is accessed and modified from both the main thread and
		//            the asset thread, and so requires synchronization
		std::unordered_map<AssetHandle, Ref<Asset>> m_MemoryAssets;
		std::shared_mutex m_MemoryAssetsMutex;

		// TODO(Yan): support multiple asset packs maybe? Or at least multiple volumes
		Ref<AssetPack> m_AssetPack;
		AssetHandle m_ActiveScene = 0;

		Ref<RuntimeAssetSystem> m_AssetThread;
	};

}

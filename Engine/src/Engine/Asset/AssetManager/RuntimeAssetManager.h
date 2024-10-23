#pragma once

#include "AssetManagerBase.h"

#include "Engine/Serialization/AssetPack.h"

namespace Hazel {

	// AssetPack
	class RuntimeAssetManager : public AssetManagerBase
	{
	public:
		RuntimeAssetManager();
		virtual ~RuntimeAssetManager();

		virtual AssetType GetAssetType(AssetHandle assetHandle) override;
		virtual Ref<Asset> GetAsset(AssetHandle assetHandle) override;
		virtual void AddMemoryOnlyAsset(Ref<Asset> asset) override;
		virtual bool ReloadData(AssetHandle assetHandle) override;
		virtual bool IsAssetHandleValid(AssetHandle assetHandle) override;
		virtual bool IsMemoryAsset(AssetHandle handle) override;
		virtual bool IsAssetLoaded(AssetHandle handle) override;
		virtual void RemoveAsset(AssetHandle handle) override;

		virtual std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type) override;
		virtual const std::unordered_map<AssetHandle, Ref<Asset>>& GetLoadedAssets() override { return m_LoadedAssets; }
		virtual const std::unordered_map<AssetHandle, Ref<Asset>>& GetMemoryOnlyAssets() override { return m_MemoryAssets; }
		
		// Loads Scene and makes active
		Ref<Scene> LoadScene(AssetHandle handle);

		void SetAssetPack(Ref<AssetPack> assetPack) { m_AssetPack = assetPack; }
	private:
		std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;
		std::unordered_map<AssetHandle, Ref<Asset>> m_MemoryAssets;

		// TODO(Yan): support multiple asset packs maybe? Or at least multiple volumes
		Ref<AssetPack> m_AssetPack;
		AssetHandle m_ActiveScene = 0;
	};

}

#pragma once

#include "Hazel/Asset/Asset.h"
#include "Hazel/Asset/AssetTypes.h"

#include <unordered_set>
#include <unordered_map>

namespace Hazel {

	//////////////////////////////////////////////////////////////////
	// AssetManagerBase //////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////
	// Implementation in RuntimeAssetManager and EditorAssetManager //
	// Static wrapper in AssetManager ////////////////////////////////
	//////////////////////////////////////////////////////////////////
	class AssetManagerBase : public RefCounted
	{
	public:
		AssetManagerBase() = default;
		virtual ~AssetManagerBase() = default;

		virtual AssetType GetAssetType(AssetHandle assetHandle) = 0;
		virtual Ref<Asset> GetAsset(AssetHandle assetHandle) = 0;
		virtual void AddMemoryOnlyAsset(Ref<Asset> asset) = 0;
		virtual bool ReloadData(AssetHandle assetHandle) = 0;
		virtual bool IsAssetHandleValid(AssetHandle assetHandle) = 0;
		virtual bool IsMemoryAsset(AssetHandle handle) = 0;
		virtual bool IsAssetLoaded(AssetHandle handle) = 0;
		virtual void RemoveAsset(AssetHandle handle) = 0;

		virtual std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type) = 0;
		virtual const std::unordered_map<AssetHandle, Ref<Asset>>& GetLoadedAssets() = 0;
		virtual const std::unordered_map<AssetHandle, Ref<Asset>>& GetMemoryOnlyAssets() = 0;
	};

}

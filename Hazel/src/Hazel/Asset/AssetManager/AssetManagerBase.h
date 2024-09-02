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

		virtual void Shutdown() = 0;

		virtual AssetType GetAssetType(AssetHandle assetHandle) = 0;
		virtual Ref<Asset> GetAsset(AssetHandle assetHandle) = 0;
		virtual AsyncAssetResult<Asset> GetAssetAsync(AssetHandle assetHandle) = 0;

		virtual void AddMemoryOnlyAsset(Ref<Asset> asset) = 0;
		virtual bool ReloadData(AssetHandle assetHandle) = 0;
		virtual void ReloadDataAsync(AssetHandle assetHandle) = 0;
		virtual bool EnsureCurrent(AssetHandle assetHandle) = 0;
		virtual bool EnsureAllLoadedCurrent() = 0;
		virtual bool IsAssetHandleValid(AssetHandle assetHandle) = 0; // the asset handle is valid (this says nothing about the asset itself)
		virtual Ref<Asset> GetMemoryAsset(AssetHandle handle) = 0;    // if exists in memory only (i.e. there is no backing file) return it otherwise return nullptr (this is more efficient than IsMemoryAsset() followed by GetAsset())
		virtual bool IsAssetLoaded(AssetHandle handle) = 0;           // asset has been loaded from file (it could still be invalid)
		virtual bool IsAssetValid(AssetHandle handle) = 0;            // asset file was loaded, but is invalid for some reason (e.g. corrupt file)
		virtual bool IsAssetMissing(AssetHandle handle) = 0;          // asset file is missing
		virtual bool IsMemoryAsset(AssetHandle handle) = 0;
		virtual bool IsPhysicalAsset(AssetHandle handle) = 0;
		virtual void RemoveAsset(AssetHandle handle) = 0;

		virtual void RegisterDependency(AssetHandle handle, AssetHandle dependency) = 0;

		virtual void SyncWithAssetThread() = 0;

		virtual std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type) = 0;
		virtual const std::unordered_map<AssetHandle, Ref<Asset>>& GetLoadedAssets() = 0;
	};

}

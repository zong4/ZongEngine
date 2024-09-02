#pragma once

#include "Hazel/Asset/Asset.h"
#include "Hazel/Asset/AssetTypes.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Project/Project.h"
#include "Hazel/Utilities/FileSystem.h"

#include <functional>
#include <unordered_map>
#include <vector>

// Asynchronous asset loading can be disabled by setting this to 0
// If you do this, then assets will not be automatically reloaded if/when they are changed by some external tool,
// and you will have to manually reload them via content browser panel.
#define ASYNC_ASSETS 1

namespace Hazel {

	class AssetManager
	{
	public:
		// Returns true if assetHandle could potentially be valid.
		static bool IsAssetHandleValid(AssetHandle assetHandle) { return Project::GetAssetManager()->IsAssetHandleValid(assetHandle); }

		// Returns true if the asset referred to by assetHandle is valid.
		// Note that this will attempt to load the asset if it is not already loaded.
		// An asset is invalid if any of the following are true:
		// - The asset handle is invalid
		// - The file referred to by asset meta data is missing
		// - The asset could not be loaded from file
		static bool IsAssetValid(AssetHandle assetHandle) { return Project::GetAssetManager()->IsAssetValid(assetHandle); }

		// Returns true if the asset referred to by assetHandle is missing.
		// Note that this checks for existence of file, but makes no attempt to load the asset from file
		// A memory-only asset cannot be missing.
		static bool IsAssetMissing(AssetHandle assetHandle) { return Project::GetAssetManager()->IsAssetMissing(assetHandle); }

		static bool IsMemoryAsset(AssetHandle handle) { return Project::GetAssetManager()->IsMemoryAsset(handle); }
		static bool IsPhysicalAsset(AssetHandle handle) { return Project::GetAssetManager()->IsPhysicalAsset(handle); }

		static bool ReloadData(AssetHandle assetHandle) { return Project::GetAssetManager()->ReloadData(assetHandle); }
		static bool EnsureCurrent(AssetHandle assetHandle) { return Project::GetAssetManager()->EnsureCurrent(assetHandle); }
		static bool EnsureAllLoadedCurrent() { return Project::GetAssetManager()->EnsureAllLoadedCurrent(); }

		static AssetType GetAssetType(AssetHandle assetHandle) { return Project::GetAssetManager()->GetAssetType(assetHandle); }

		static void SyncWithAssetThread() { return Project::GetAssetManager()->SyncWithAssetThread(); }
		
		static Ref<Asset> GetPlaceholderAsset(AssetType type);

		template<typename T>
		static Ref<T> GetAsset(AssetHandle assetHandle)
		{
			//static std::mutex mutex;
			//std::scoped_lock<std::mutex> lock(mutex);

			Ref<Asset> asset = Project::GetAssetManager()->GetAsset(assetHandle);
			return asset.As<T>();
		}

		template<typename T>
		static AsyncAssetResult<T> GetAssetAsync(AssetHandle assetHandle)
		{
#if ASYNC_ASSETS
			AsyncAssetResult<Asset> result = Project::GetAssetManager()->GetAssetAsync(assetHandle);
			return AsyncAssetResult<T>(result);
#else
			return { GetAsset<T>(assetHandle), true };
#endif
		}

		template<typename T>
		static std::unordered_set<AssetHandle> GetAllAssetsWithType()
		{
			return Project::GetAssetManager()->GetAllAssetsWithType(T::GetStaticType());
		}

		static const std::unordered_map<AssetHandle, Ref<Asset>>& GetLoadedAssets() { return Project::GetAssetManager()->GetLoadedAssets(); }

		// Note (0x): The memory-only asset must be fully initialised before you AddMemoryOnlyAsset()
		//            Assets are not themselves thread-safe, but can potentially be accessed from multiple
		//            threads.  Thread safety therefore depends on the assets being immutable once they've been
		//            added to the asset manager.
		template<typename TAsset>
		static AssetHandle AddMemoryOnlyAsset(Ref<TAsset> asset)
		{
			static_assert(std::is_base_of<Asset, TAsset>::value, "AddMemoryOnlyAsset only works for types derived from Asset");
			asset->Handle = AssetHandle(); // NOTE(Yan): should handle generation happen here?

			Project::GetAssetManager()->AddMemoryOnlyAsset(asset);
			return asset->Handle;
		}

		static Ref<Asset> GetMemoryAsset(AssetHandle handle) { return Project::GetAssetManager()->GetMemoryAsset(handle); }

		static void RegisterDependency(AssetHandle handle, AssetHandle dependency) { return Project::GetAssetManager()->RegisterDependency(handle, dependency); }

		static void RemoveAsset(AssetHandle handle)
		{
			Project::GetAssetManager()->RemoveAsset(handle);
		}

	};

}

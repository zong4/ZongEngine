#pragma once

#include "Engine/Asset/Asset.h"
#include "Engine/Asset/AssetTypes.h"
#include "Engine/Project/Project.h"
#include "Engine/Utilities/FileSystem.h"

#include <functional>
#include <unordered_map>
#include <vector>

namespace Hazel {

	class AssetManager
	{
	public:
		static bool IsAssetHandleValid(AssetHandle assetHandle) { return Project::GetAssetManager()->IsAssetHandleValid(assetHandle); }

		static bool ReloadData(AssetHandle assetHandle) { return Project::GetAssetManager()->ReloadData(assetHandle); }

		static AssetType GetAssetType(AssetHandle assetHandle) { return Project::GetAssetManager()->GetAssetType(assetHandle); }

		template<typename T>
		static Ref<T> GetAsset(AssetHandle assetHandle)
		{
			//static std::mutex mutex;
			//std::scoped_lock<std::mutex> lock(mutex);

			Ref<Asset> asset = Project::GetAssetManager()->GetAsset(assetHandle);
			return asset.As<T>();
		}

		template<typename T>
		static std::unordered_set<AssetHandle> GetAllAssetsWithType()
		{
			return Project::GetAssetManager()->GetAllAssetsWithType(T::GetStaticType());
		}

		static const std::unordered_map<AssetHandle, Ref<Asset>>& GetLoadedAssets() { return Project::GetAssetManager()->GetLoadedAssets(); }
		static const std::unordered_map<AssetHandle, Ref<Asset>>& GetMemoryOnlyAssets() { return Project::GetAssetManager()->GetMemoryOnlyAssets(); }

		template<typename TAsset, typename... TArgs>
		static AssetHandle CreateMemoryOnlyAsset(TArgs&&... args)
		{
			static_assert(std::is_base_of<Asset, TAsset>::value, "CreateMemoryOnlyAsset only works for types derived from Asset");

			Ref<TAsset> asset = Ref<TAsset>::Create(std::forward<TArgs>(args)...);
			asset->Handle = AssetHandle(); // NOTE(Yan): should handle generation happen here?

			Project::GetAssetManager()->AddMemoryOnlyAsset(asset);
			return asset->Handle;
		}

		template<typename TAsset, typename... TArgs>
		static AssetHandle CreateMemoryOnlyRendererAsset(TArgs&&... args)
		{
			static_assert(std::is_base_of<Asset, TAsset>::value, "CreateMemoryOnlyAsset only works for types derived from Asset");

			Ref<TAsset> asset = TAsset::Create(std::forward<TArgs>(args)...);
			asset->Handle = AssetHandle();

			Project::GetAssetManager()->AddMemoryOnlyAsset(asset);
			return asset->Handle;
		}

		template<typename TAsset, typename... TArgs>
		static AssetHandle CreateMemoryOnlyAssetWithHandle(AssetHandle handle, TArgs&&... args)
		{
			static_assert(std::is_base_of<Asset, TAsset>::value, "CreateMemoryOnlyAsset only works for types derived from Asset");

			Ref<TAsset> asset = Ref<TAsset>::Create(std::forward<TArgs>(args)...);
			asset->Handle = handle;

			Project::GetAssetManager()->AddMemoryOnlyAsset(asset);
			return asset->Handle;
		}

		template<typename TAsset>
		static AssetHandle AddMemoryOnlyAsset(Ref<TAsset> asset)
		{
			static_assert(std::is_base_of<Asset, TAsset>::value, "AddMemoryOnlyAsset only works for types derived from Asset");
			asset->Handle = AssetHandle(); // NOTE(Yan): should handle generation happen here?

			Project::GetAssetManager()->AddMemoryOnlyAsset(asset);
			return asset->Handle;
		}

		static bool IsMemoryAsset(AssetHandle handle) { return Project::GetAssetManager()->IsMemoryAsset(handle); }

		static void RemoveAsset(AssetHandle handle)
		{
			Project::GetAssetManager()->RemoveAsset(handle);
		}
	};

}

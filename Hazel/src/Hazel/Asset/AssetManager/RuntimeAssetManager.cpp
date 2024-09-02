#include "hzpch.h"
#include "RuntimeAssetManager.h"

#include "Hazel/Asset/AssetImporter.h"
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Core/Timer.h"
#include "Hazel/Debug/Profiler.h"

namespace Hazel {

	RuntimeAssetManager::RuntimeAssetManager()
	{
#if ASYNC_ASSETS
		m_AssetThread = Ref<RuntimeAssetSystem>::Create();
#endif

		AssetImporter::Init();
	}

	RuntimeAssetManager::~RuntimeAssetManager()
	{
	}

	AssetType RuntimeAssetManager::GetAssetType(AssetHandle assetHandle)
	{
		Ref<Asset> asset = GetAsset(assetHandle);
		if (!asset)
			return AssetType::None;

		return asset->GetAssetType();
	}

	Ref<Asset> RuntimeAssetManager::GetAsset(AssetHandle assetHandle)
	{
		HZ_PROFILE_FUNC();
		HZ_SCOPE_PERF("AssetManager::GetAsset");

		if (auto asset = GetMemoryAsset(assetHandle); asset)
			return asset;

		Ref<Asset> asset = nullptr;
		if(IsAssetLoaded(assetHandle))
		{
			asset = m_LoadedAssets[assetHandle];
		}
		else
		{
			if (Application::IsMainThread())
			{
				// If we're main thread, we can just loading the asset as normal
				asset = m_AssetPack->LoadAsset(m_ActiveScene, assetHandle);
				if (asset)
					m_LoadedAssets[assetHandle] = asset;
			}
			else
			{
				// Not main thread -> ask AssetThread for the asset
				// If the asset needs to be loaded, this will load the asset.
				// The load will happen on this thread (which is probably asset thread, but occasionally might be audio thread).
				// The asset will get synced into main thread at next asset sync point.
				asset = m_AssetThread->GetAsset(m_ActiveScene, assetHandle);
			}
		}
		return asset;
	}

	AsyncAssetResult<Asset> RuntimeAssetManager::GetAssetAsync(AssetHandle assetHandle)
	{
#if ASYNC_ASSETS
		HZ_PROFILE_FUNC();
		HZ_SCOPE_PERF("AssetManager::GetAsset");

		if (auto asset = GetMemoryAsset(assetHandle); asset)
			return { asset, true };

		if(IsAssetLoaded(assetHandle))
			return { m_LoadedAssets.at(assetHandle), true };

		// Queue load (if not already) and return placeholder
		if (!m_PendingAssets.contains(assetHandle))
		{
			m_PendingAssets.insert(assetHandle);
			RuntimeAssetLoadRequest alr(m_ActiveScene, assetHandle);
			m_AssetThread->QueueAssetLoad(alr);
		}

		AssetType assetType = m_AssetPack->GetAssetType(m_ActiveScene, assetHandle);
		return { AssetManager::GetPlaceholderAsset(assetType), false };
#else
		return { GetAsset(assetHandle), true };
#endif
	}

	void RuntimeAssetManager::AddMemoryOnlyAsset(Ref<Asset> asset)
	{
		std::scoped_lock lock(m_MemoryAssetsMutex);
		m_MemoryAssets[asset->Handle] = asset;
	}

	bool RuntimeAssetManager::ReloadData(AssetHandle assetHandle)
	{
		Ref<Asset> asset = m_AssetPack->LoadAsset(m_ActiveScene, assetHandle);
		if (asset)
			m_LoadedAssets[assetHandle] = asset;
		
		return asset;
	}

	void RuntimeAssetManager::ReloadDataAsync(AssetHandle assetHandle)
	{
#if ASYNC_ASSETS
		// Queue load (if not already)
		if (!m_PendingAssets.contains(assetHandle))
		{
			m_PendingAssets.insert(assetHandle);
			m_AssetThread->QueueAssetLoad({ m_ActiveScene, assetHandle });
		}
#else
		ReloadData(assetHandle);
#endif
	}

	bool RuntimeAssetManager::EnsureAllLoadedCurrent()
	{
		HZ_CORE_VERIFY(false);
		return false;
	}

	bool RuntimeAssetManager::EnsureCurrent(AssetHandle assetHandle)
	{
		HZ_CORE_VERIFY(false);
		return false;
	}

	bool RuntimeAssetManager::IsAssetHandleValid(AssetHandle assetHandle)
	{
		if (assetHandle == 0)
			return false;

		return GetMemoryAsset(assetHandle) || (m_AssetPack && m_AssetPack->IsAssetHandleValid(assetHandle));
	}

	Ref<Asset> RuntimeAssetManager::GetMemoryAsset(AssetHandle handle)
	{
		std::shared_lock lock(m_MemoryAssetsMutex);
		if (auto it = m_MemoryAssets.find(handle); it != m_MemoryAssets.end())
			return it->second;

		return nullptr;
	}

	bool RuntimeAssetManager::IsAssetLoaded(AssetHandle handle)
	{
		return m_LoadedAssets.contains(handle);
	}

	bool RuntimeAssetManager::IsAssetValid(AssetHandle handle)
	{
		HZ_PROFILE_FUNC();
		HZ_SCOPE_PERF("AssetManager::IsAssetValid");

		return GetAsset(handle) != nullptr;
	}

	bool RuntimeAssetManager::IsAssetMissing(AssetHandle handle)
	{
		return !IsAssetValid(handle);
	}

	bool RuntimeAssetManager::IsMemoryAsset(AssetHandle handle)
	{
		std::scoped_lock lock(m_MemoryAssetsMutex);
		return m_MemoryAssets.contains(handle);
	}

	bool RuntimeAssetManager::IsPhysicalAsset(AssetHandle handle)
	{
		return !IsMemoryAsset(handle);
	}

	void RuntimeAssetManager::RemoveAsset(AssetHandle handle)
	{
		{
			std::scoped_lock lock(m_MemoryAssetsMutex);
			if (m_MemoryAssets.contains(handle))
				m_MemoryAssets.erase(handle);
		}

		if (m_LoadedAssets.contains(handle))
			m_LoadedAssets.erase(handle);

	}

	std::unordered_set<Hazel::AssetHandle> RuntimeAssetManager::GetAllAssetsWithType(AssetType type)
	{
		std::unordered_set<AssetHandle> result;
		HZ_CORE_VERIFY(false, "Not implemented");
		return result;
	}

	void RuntimeAssetManager::SyncWithAssetThread()
	{
#if ASYNC_ASSETS
		std::vector<Ref<Asset>> freshAssets;

		m_AssetThread->RetrieveReadyAssets(freshAssets);
		for (const auto& asset : freshAssets)
		{
			m_LoadedAssets[asset->Handle] = asset;
			HZ_CORE_VERIFY(m_PendingAssets.contains(asset->Handle));
			m_PendingAssets.erase(asset->Handle);
		}

		m_AssetThread->UpdateLoadedAssetList(m_LoadedAssets);

		// Update dependencies after syncing everything
		for (const auto& asset : freshAssets)
		{
			UpdateDependencies(asset->Handle);
		}

#else
		Application::Get().SyncEvents();
#endif
	}

	Ref<Scene> RuntimeAssetManager::LoadScene(AssetHandle handle)
	{
		Ref<Scene> scene = m_AssetPack->LoadScene(handle);
		if (scene)
			m_ActiveScene = handle;

		return scene;
	}

	void RuntimeAssetManager::SetAssetPack(Ref<AssetPack> assetPack)
	{
		m_AssetPack = assetPack;
#if ASYNC_ASSETS
		m_AssetThread->SetAssetPack(assetPack);
#endif
	}

}

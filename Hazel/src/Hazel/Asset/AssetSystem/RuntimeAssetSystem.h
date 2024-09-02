#pragma once

#include "Hazel/Asset/AssetMetadata.h"
#include "Hazel/Core/Thread.h"
#include "Hazel/Serialization/AssetPack.h"

#include <atomic>
#include <mutex>
#include <queue>

namespace Hazel {

	class RuntimeAssetSystem : public RefCounted
	{
	public:
		RuntimeAssetSystem();
		virtual ~RuntimeAssetSystem();

		// Queue an asset for later loading (on asset thread)
		void QueueAssetLoad(const RuntimeAssetLoadRequest& request);

		// Get an asset immediately (on asset thread).
		// If the asset needs to be loaded, it will be loaded into "ready assets" and transfered back to main thread
		// at next asset sync.
		Ref<Asset> GetAsset(const AssetHandle sceneHandle, const AssetHandle assetHandle);

		// Retrieve assets that have been loaded (from and earlier request)
		bool RetrieveReadyAssets(std::vector<Ref<Asset>>& outAssetList);

		// Replace the currently loaded asset collection with the given loadedAssets.
		// This is effectively takes a "thread local" snapshot of the asset manager's loaded assets.
		void UpdateLoadedAssetList(const std::unordered_map<AssetHandle, Ref<Asset>>& loadedAssets);

		void SetAssetPack(Ref<AssetPack> assetPack) { m_AssetPack = assetPack; }

		void Stop();
		void StopAndWait();

	private:
		// The asset thread's mainline
		void AssetThreadFunc();

		Ref<Asset> TryLoadData(const RuntimeAssetLoadRequest& request);

	private:
		Thread m_Thread;
		std::atomic<bool> m_Running = true; // not false. This ensures that if Stop() is called after the thread is dispatched but before it actually starts running, then the thread is correctly stopped.

		Ref<AssetPack> m_AssetPack;

		std::queue<RuntimeAssetLoadRequest> m_AssetLoadingQueue;
		std::mutex m_AssetLoadingQueueMutex;
		std::condition_variable m_AssetLoadingQueueCV;

		std::vector<Ref<Asset>> m_LoadedAssets; // Assets that have been loaded asynchronously and are waiting for sync back to Asset Manager
		std::mutex m_LoadedAssetsMutex;

		std::unordered_map<AssetHandle, Ref<Asset>> m_AMLoadedAssets;  // All currently loaded assets (synced from Asset Manager)
		std::mutex m_AMLoadedAssetsMutex;
	};

}

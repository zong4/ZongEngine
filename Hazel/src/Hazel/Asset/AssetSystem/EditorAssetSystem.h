#pragma once

#include "Hazel/Asset/AssetMetadata.h"
#include "Hazel/Core/Thread.h"

#include <atomic>
#include <mutex>
#include <queue>

namespace Hazel {

	class EditorAssetSystem : public RefCounted
	{
	public:
		EditorAssetSystem();
		~EditorAssetSystem();

		// Queue an asset to be loaded on asset thread later
		void QueueAssetLoad(const AssetMetadata& request);

		// Get an asset immediately (on asset thread).
		// If the asset needs to be loaded, it will be loaded into "ready assets" and transfered back to main thread
		// at next asset sync.
		Ref<Asset> GetAsset(const AssetMetadata& request);

		// Retrieve assets that have been loaded (from and earlier request)
		bool RetrieveReadyAssets(std::vector<EditorAssetLoadResponse>& outAssetList);

		// Replace the currently loaded asset collection with the given loadedAssets.
		// This effectively takes a "thread local" snapshot of the asset manager's loaded assets.
		void UpdateLoadedAssetList(const std::unordered_map<AssetHandle, Ref<Asset>>& loadedAssets);

		void Stop();
		void StopAndWait();

		// Monitor for updated assets, and if any are found queue them for reload
		void AssetMonitorUpdate();

	private:
		// The asset thread's mainline
		void AssetThreadFunc();

		std::filesystem::path GetFileSystemPath(const AssetMetadata& metadata);

		void EnsureAllLoadedCurrent();
		void EnsureCurrent(AssetHandle assetHandle);
		Ref<Asset> TryLoadData(AssetMetadata metadata);

	private:
		Thread m_Thread;
		std::atomic<bool> m_Running = true;  // not false. This ensures that if Stop() is called after the thread is dispatched but before it actually starts running, then the thread is correctly stopped.

		std::queue<AssetMetadata> m_AssetLoadingQueue;
		std::mutex m_AssetLoadingQueueMutex;
		std::condition_variable m_AssetLoadingQueueCV;

		std::vector<EditorAssetLoadResponse> m_LoadedAssets; // Assets that have been loaded asynchronously and are waiting for sync back to Asset Manager
		std::mutex m_LoadedAssetsMutex;

		std::unordered_map<AssetHandle, Ref<Asset>> m_AMLoadedAssets;  // All currently loaded assets (synced from Asset Manager)
		std::mutex m_AMLoadedAssetsMutex;

		// Asset Monitoring
		float m_AssetUpdatePerf = 0.0f;
	};

}

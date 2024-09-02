#include "hzpch.h"
#include "RuntimeAssetSystem.h"

#include "Hazel/Debug/Profiler.h"
#include "Hazel/Project/Project.h"

namespace Hazel {

	RuntimeAssetSystem::RuntimeAssetSystem()
		: m_Thread("Asset Thread")
	{
		m_Thread.Dispatch([this]() { AssetThreadFunc(); });
	}

	RuntimeAssetSystem::~RuntimeAssetSystem()
	{
		StopAndWait();
	}

	void RuntimeAssetSystem::Stop()
	{
		m_Running = false;
		m_AssetLoadingQueueCV.notify_one();
	}

	void RuntimeAssetSystem::StopAndWait()
	{
		Stop();
		m_Thread.Join();
	}

	void RuntimeAssetSystem::AssetThreadFunc()
	{
		HZ_PROFILE_THREAD("Asset Thread");

		while (m_Running)
		{
			HZ_PROFILE_SCOPE("Asset Thread Queue");

			// Go through queue and see what needs loading
			bool queueEmptyOrStop = false;
			while (!queueEmptyOrStop)
			{
				RuntimeAssetLoadRequest alr;
				{
					std::scoped_lock<std::mutex> lock(m_AssetLoadingQueueMutex);
					if (m_AssetLoadingQueue.empty() || !m_Running)
					{
						queueEmptyOrStop = true;
					}
					else
					{
						alr = m_AssetLoadingQueue.front();
						m_AssetLoadingQueue.pop();
					}
				}

				// If queueEmptyOrStop then request will be invalid (Handle == 0)
				// We check handles here (instead of just breaking straight away on queueEmptyOrStop)
				// to deal with the edge case that other thread might queue requests for invalid assets.
				// This way, we just pop those requests and ignore them.
				if (alr.Handle != 0 && alr.SceneHandle != 0)
				{
					TryLoadData(alr);
				}
			}

			std::unique_lock<std::mutex> lock(m_AssetLoadingQueueMutex);
			// need to check conditions again, since other thread could have changed them between releasing the lock (in the while loop above)
			// and re-acquiring the lock here
			if (m_AssetLoadingQueue.empty() && m_Running)
			{
				m_AssetLoadingQueueCV.wait(lock, [this] {
					return !m_Running || !m_AssetLoadingQueue.empty();
				});
			}
		}
	}

	Ref<Asset> RuntimeAssetSystem::TryLoadData(const RuntimeAssetLoadRequest& alr)
	{
		HZ_CORE_INFO_TAG("AssetSystem", "Loading asset {}", alr.Handle);
		Ref<Asset> asset;
		if (asset = m_AssetPack->LoadAsset(alr.SceneHandle, alr.Handle); asset)
		{
			std::scoped_lock<std::mutex> lock(m_LoadedAssetsMutex);
			m_LoadedAssets.emplace_back(asset);
			HZ_CORE_INFO_TAG("AssetSystem", "Finished loading asset {}", alr.Handle);
		}
		else
		{
			HZ_CORE_ERROR_TAG("AssetSystem", "Failed to load asset {}", alr.Handle);
		}
		return asset;
	}

	void RuntimeAssetSystem::QueueAssetLoad(const RuntimeAssetLoadRequest& request)
	{
		{
			std::scoped_lock<std::mutex> lock(m_AssetLoadingQueueMutex);
			m_AssetLoadingQueue.push(request);
		}
		m_AssetLoadingQueueCV.notify_one();
	}

	Ref<Asset> RuntimeAssetSystem::GetAsset(const AssetHandle sceneHandle, const AssetHandle assetHandle)
	{
		{
			std::scoped_lock<std::mutex> lock(m_AMLoadedAssetsMutex);
			if (auto it = m_AMLoadedAssets.find(assetHandle); it != m_AMLoadedAssets.end())
				return it->second;
		}
		return TryLoadData({ sceneHandle, assetHandle });
	}

	bool RuntimeAssetSystem::RetrieveReadyAssets(std::vector<Ref<Asset>>& outAssetList)
	{
		if (m_LoadedAssets.empty())
			return false;

		std::scoped_lock<std::mutex> lock(m_LoadedAssetsMutex);
		outAssetList = m_LoadedAssets;
		m_LoadedAssets.clear();
		return true;
	}

	void RuntimeAssetSystem::UpdateLoadedAssetList(const std::unordered_map<AssetHandle, Ref<Asset>>& loadedAssets)
	{
		std::scoped_lock<std::mutex> lock(m_AMLoadedAssetsMutex);
		m_AMLoadedAssets = loadedAssets;
	}

}

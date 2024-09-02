#include "hzpch.h"
#include "EditorAssetSystem.h"

#include "Hazel/Asset/AssetImporter.h"
#include "Hazel/Project/Project.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Core/Events/EditorEvents.h"

#include "Hazel/Debug/Profiler.h"

namespace Hazel {

	EditorAssetSystem::EditorAssetSystem()
		: m_Thread("Asset Thread")
	{
		m_Thread.Dispatch([this]() { AssetThreadFunc(); });
	}

	EditorAssetSystem::~EditorAssetSystem()
	{
		StopAndWait();
	}

	void EditorAssetSystem::Stop()
	{
		m_Running = false;
		m_AssetLoadingQueueCV.notify_one();
	}

	void EditorAssetSystem::StopAndWait()
	{
		Stop();
		m_Thread.Join();
	}

	void EditorAssetSystem::AssetMonitorUpdate()
	{
		Timer timer;
		EnsureAllLoadedCurrent();
		m_AssetUpdatePerf = timer.ElapsedMillis();
	}

	void EditorAssetSystem::AssetThreadFunc()
	{
		HZ_PROFILE_THREAD("Asset Thread");

		while (m_Running)
		{
			HZ_PROFILE_SCOPE("Asset Thread Queue");

			AssetMonitorUpdate();

			bool queueEmptyOrStop = false;
			while (!queueEmptyOrStop)
			{
				AssetMetadata metadata;
				{
					std::scoped_lock<std::mutex> lock(m_AssetLoadingQueueMutex);
					if (m_AssetLoadingQueue.empty() || !m_Running)
					{
						queueEmptyOrStop = true;
					}
					else
					{
						metadata = m_AssetLoadingQueue.front();
						m_AssetLoadingQueue.pop();
					}
				}

				// If queueEmptyOrStop then metadata will be invalid (Handle == 0)
				// We check metadata here (instead of just breaking straight away on queueEmptyOrStop)
				// to deal with the edge case that other thread might queue requests for invalid assets.
				// This way, we just pop those requests and ignore them.
				if (metadata.IsValid())
				{
					TryLoadData(metadata);
				}
			}

			std::unique_lock<std::mutex> lock(m_AssetLoadingQueueMutex);
			// need to check conditions again, since other thread could have changed them between releasing the lock (in the while loop above)
			// and re-acquiring the lock here
			if (m_AssetLoadingQueue.empty() && m_Running)
			{
				// need to wake periodically (here 100ms) so that AssetMonitorUpdate() is called regularly to check for updated file timestamps
				// (which kinda makes condition variable redundant. may as well just sleep(100ms))
				m_AssetLoadingQueueCV.wait_for(lock, std::chrono::milliseconds(100), [this] {
					return !m_Running || !m_AssetLoadingQueue.empty();
				});
			}
		}
	}

	void EditorAssetSystem::QueueAssetLoad(const AssetMetadata& request)
	{
		{
			std::scoped_lock<std::mutex> lock(m_AssetLoadingQueueMutex);
			m_AssetLoadingQueue.push(request);
		}
		m_AssetLoadingQueueCV.notify_one();
	}

	Ref<Asset> EditorAssetSystem::GetAsset(const AssetMetadata& request)
	{
		{
			std::scoped_lock<std::mutex> lock(m_AMLoadedAssetsMutex);
			if (auto it = m_AMLoadedAssets.find(request.Handle); it != m_AMLoadedAssets.end())
				return it->second;
		}
		return TryLoadData(request);
	}

	bool EditorAssetSystem::RetrieveReadyAssets(std::vector<EditorAssetLoadResponse>& outAssetList)
	{
		std::vector<EditorAssetLoadResponse> loadedAssets;
		{
			std::scoped_lock lock(m_LoadedAssetsMutex);
			std::swap(loadedAssets, m_LoadedAssets);

			// Now that we've sync'd assets, any events that were dispatched from TryLoadData() are safe to be processed
			Application::Get().SyncEvents();
		}

		if (loadedAssets.empty())
			return false;

		outAssetList = loadedAssets;
		return true;
	}

	void EditorAssetSystem::UpdateLoadedAssetList(const std::unordered_map<AssetHandle, Ref<Asset>>& loadedAssets)
	{
		std::scoped_lock<std::mutex> lock(m_AMLoadedAssetsMutex);
		m_AMLoadedAssets = loadedAssets;
	}

	std::filesystem::path EditorAssetSystem::GetFileSystemPath(const AssetMetadata& metadata)
	{
		// TODO (0x): This is not safe.  Project asset directory can be modified by other threads
		return Project::GetActiveAssetDirectory() / metadata.FilePath;
	}

	void EditorAssetSystem::EnsureAllLoadedCurrent()
	{
		HZ_PROFILE_FUNC();

		// This is a long time to hold a lock.
		// However, copying the list of assets to iterate (so that we could then release the lock before iterating) is also expensive, so hard to tell which is better.
		std::scoped_lock<std::mutex> lock(m_AMLoadedAssetsMutex);
		for (const auto& [handle, asset] : m_AMLoadedAssets)
		{
			EnsureCurrent(handle);
		}
	}

	void EditorAssetSystem::EnsureCurrent(AssetHandle assetHandle)
	{
		auto metadata = Project::GetEditorAssetManager()->GetMetadata(assetHandle);

		// other thread could have deleted the asset since our asset list was last sync'd
		if(!metadata.IsValid()) return;

		auto absolutePath = GetFileSystemPath(metadata);

		if (!FileSystem::Exists(absolutePath))
			return;

		uint64_t actualLastWriteTime = FileSystem::GetLastWriteTime(absolutePath);
		uint64_t recordedLastWriteTime = metadata.FileLastWriteTime;

		if (actualLastWriteTime == recordedLastWriteTime)
			return;

		if (actualLastWriteTime == 0 || recordedLastWriteTime == 0)
			return;

		// Queue here rather than TryLoad(), as we are holding a lock (in EnsureAllLoadedCurrent()) and want this function to return as quickly as possible
		return QueueAssetLoad(metadata);
	}


//	// TODO (0x): delete once dependent assets are handled properly using asset dependencies
//	bool EditorAssetSystem::ReloadData(AssetHandle handle)
//	{
//		auto metadata = Project::GetEditorAssetManager()->GetMetadata(handle);
//		return metadata.IsValid() && ReloadData(metadata);
//	}


	Ref<Asset> EditorAssetSystem::TryLoadData(AssetMetadata metadata)
	{
		Ref<Asset> asset;
		if (!metadata.IsValid())
		{
			HZ_CORE_ERROR("Trying to load invalid asset");
			return asset;
		}

		HZ_CORE_INFO_TAG("AssetSystem", "{}LOADING ASSET - {}", metadata.IsDataLoaded? "RE" : "", metadata.FilePath.string());

		// ASSUMPTION: AssetImporter serializers are immutable and re-entrant
		if (AssetImporter::TryLoadData(metadata, asset))
		{
			metadata.IsDataLoaded = true;
			auto absolutePath = GetFileSystemPath(metadata);

			// Note (0x): There's a small hole here.  Other thread could start writing to asset's file in the exact instant that TryLoadData() has finished with it.
			//            GetLastWriteTime() then blocks until the write has finished, but now we have a new write time - not the one that was relevent for TryLoadData()
			//            To resolve this, you bascially need to lock the metadata until both the TryLoadData() _and_ the GetLastWriteTime() have completed.
			//            Or you need to update the last write time while you still have the file locked during TryLoadData()
			metadata.FileLastWriteTime = FileSystem::GetLastWriteTime(absolutePath);
			{
				std::scoped_lock<std::mutex> lock(m_LoadedAssetsMutex);
				m_LoadedAssets.emplace_back(metadata, asset);

				// be careful:
				// 1) DispatchEvent() is only thread-safe when DispatchImmediately is false.
				// 2) Events must be handled carefully so that we are sure that the assets have been synched back to main thread _before_ this event is processed.
				//    That's why we are dispatching event while we hold a lock on m_LoadedAssetsMutex (see RetrieveReadyAssets())
				Application::Get().DispatchEvent<AssetReloadedEvent, /*DispatchImmediately=*/false>(metadata.Handle);
			}

			HZ_CORE_INFO_TAG("AssetSystem", "Finished loading asset {}", metadata.FilePath.string());
		}
		else
		{
			HZ_CORE_ERROR_TAG("AssetSystem", "Failed to load asset {} ({})", metadata.Handle, metadata.FilePath);
		}

		return asset;
	}

}

#include "hzpch.h"
#include "SoundGraphCache.h"
#include "Hazel/Utilities/FileSystem.h"

namespace Hazel
{
	SoundGraphCache::SoundGraphCache(std::filesystem::path cacheFolder, uint32_t maxNumFilesToCache)
		:m_Folder(std::move(cacheFolder)), m_MaxNumFiles(maxNumFilesToCache)
	{
		if (!FileSystem::Exists(m_Folder) || !FileSystem::IsDirectory(m_Folder))
			FileSystem::CreateDirectory(m_Folder);

		PurgeOldestFiles(m_MaxNumFiles);
	}

	SoundGraphCache::SoundGraphCache(uint32_t maxNumFilesToCache)
		: m_MaxNumFiles(maxNumFilesToCache)
	{
	}

	void SoundGraphCache::SetCacheDirectory(const std::filesystem::path& newCacheFolder)
	{
		if (newCacheFolder == m_Folder)
			return;

		{
			std::scoped_lock sl(m_Lock);
			m_Folder = newCacheFolder;
			if (!FileSystem::Exists(m_Folder) || !FileSystem::IsDirectory(m_Folder))
				FileSystem::CreateDirectory(m_Folder);
		}

		PurgeOldestFiles(m_MaxNumFiles);
	}

	void SoundGraphCache::StorePrototype(const char* key, const SoundGraph::Prototype& prototype)
	{
		auto file = GetFileForKey(key);
		{
			std::scoped_lock sl(m_Lock);
			FileStreamWriter writer(file);
			
			SoundGraph::Prototype::Serialize(&writer, prototype);
		}

		PurgeOldestFiles(m_MaxNumFiles);
	}

	Ref<SoundGraph::Prototype> SoundGraphCache::ReadPtototype(const char* key)
	{
		std::scoped_lock sl(m_Lock);

		auto file = GetFileForKey(key);
		if (!std::filesystem::exists(file))
			return nullptr;

		FileStreamReader reader(file);

		auto prototype = Ref<SoundGraph::Prototype>::Create();
		SoundGraph::Prototype::Deserialize(&reader, *prototype);

		return prototype;
	}


} // namespace Hazel

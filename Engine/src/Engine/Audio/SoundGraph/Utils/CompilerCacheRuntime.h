#pragma once
#include "CompilerCache.h"

#include "Engine/Core/Assert.h"

namespace Hazel {
	//==============================================================================
	/**
		Implements a simple CompilerCache that retrives cached object code chunks
		from files in a folder.
	*/
	struct CompilerCacheRuntime final : public CompilerCacheBase
	{
		CompilerCacheRuntime(const CompilerCacheRuntime&) = delete;
		CompilerCacheRuntime& operator=(const CompilerCacheRuntime&) = delete;

		/** Creates a cache in the given folder (which must exist!) */
		CompilerCacheRuntime(std::filesystem::path cacheFolder)
			: m_Folder(cacheFolder)
		{
		}

		~CompilerCacheRuntime() = default;

		/** Runtime CompilierCache does not store new items in cache. */
		void StoreItemInCache(const char* key, const void* sourceData, uint64_t size) override
		{
			// TODO: might still want to store items, e.g. if compiled while loading the Runtime
			HZ_CORE_ASSERT(false);
			return;
		}

		/** Runtime CompilierCache looks for cached object code chunk in filesystem. */
		uint64_t ReadItemFromCache(const char* key, void* destAddress, uint64_t destSize) override
		{
			std::scoped_lock sl(m_Lock);

			auto file = GetFileForKey(key);
			uint64_t fileSize = std::filesystem::exists(file) ? (uint64_t)std::filesystem::file_size(file) : 0;

			if (fileSize == 0)
			{
				StoreCacheResult({ key, EError::FailedToFindFile });
				HZ_CORE_ASSERT(false);
				return 0;
			}

			if (destAddress == nullptr || destSize < fileSize)
				return fileSize;

			auto readEntireFile = [&]() -> bool
			{
				std::ifstream fin(file.c_str(), std::ios::binary | std::ios::ate);

				if (fin.good())
				{
					std::streampos end = fin.tellg();
					fin.seekg(0, std::ios::beg);
					uint32_t size = end - fin.tellg();
					HZ_CORE_ASSERT(size != 0);

					if (destSize != size)
					{
						StoreCacheResult({ key, EError::DifferenSizeOfDestinationAndCache });
						return false;
					}

					if (fin.read((char*)destAddress, destSize))
					{
						return true;
					}
					else
					{
						StoreCacheResult({ key, EError::FailedToReadFile });
						return false;
					}
				}
				else
				{
					StoreCacheResult({ key, EError::FailedToReadFile });
					return false;
				}
			};

			if (!readEntireFile())
			{
				HZ_CORE_ASSERT(false);
				return 0;
			}

			return fileSize;
		}

		std::filesystem::path GetFileForKey(const char* cacheKey) const final { return m_Folder / GetFileName(cacheKey); }

		void SetCacheDirectoryIfNotSet(const std::filesystem::path& newDirectory)
		{
			HZ_CORE_ASSERT(!newDirectory.empty());
			if (m_Folder.empty())
				m_Folder = newDirectory;
		}

	private:
		std::filesystem::path m_Folder = "";
	};

} //namespace Hazel

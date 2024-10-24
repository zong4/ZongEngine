#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>

namespace Engine {
	//==============================================================================
	/**
		CompilerCache base layer that implements cache error handling.
	*/
	struct CompilerCacheBase : public RefCounted
	{
		CompilerCacheBase(const CompilerCacheBase&) = delete;
		CompilerCacheBase& operator=(const CompilerCacheBase&) = delete;
		CompilerCacheBase() = default;
		virtual ~CompilerCacheBase() = default;

		virtual std::string GetFilePrefix() const = 0;
		std::string GetFileName(const char* cacheKey) const { return GetFilePrefix() + cacheKey; }

		enum class EError
		{
			None = 0,
			FailedToFindFile,
			DifferenSizeOfDestinationAndCache,
			FailedToReadFile
		};

		struct ReadResult
		{
			const char* Key = nullptr;
			EError Error = EError::None;
		};

		std::vector<ReadResult> GetCacheErrors()
		{
			std::scoped_lock sl(m_Lock);

			std::vector<ReadResult> copy = m_Results;
			m_Results.clear();
			return copy;
		}

		virtual void StoreItemInCache(const char* key, const void* sourceData, uint64_t size) = 0;
		virtual uint64_t ReadItemFromCache(const char* key, void* destAddress, uint64_t destSize) = 0;
		virtual std::filesystem::path GetFileForKey(const char* cacheKey) const = 0;

	protected:
		void StoreCacheResult(const ReadResult& result)
		{
			m_Results.push_back(result);
		}

	protected:
		mutable std::mutex m_Lock;
		std::vector<ReadResult> m_Results;
	};


	//==============================================================================
	/**
		Implements a simple CompilerCache that stores the cached object code chunks
		as files in a folder.
	*/
	struct CompilerCacheFolder : public CompilerCacheBase
	{
		CompilerCacheFolder(const CompilerCacheFolder&) = delete;
		CompilerCacheFolder& operator=(const CompilerCacheFolder&) = delete;

		/** Creates a cache in the given folder (which must exist!) */
		CompilerCacheFolder(std::filesystem::path cacheFolder, uint32_t maxNumFilesToCache)
			:m_Folder(std::move(cacheFolder)), m_MaxNumFiles(maxNumFilesToCache)
		{
			PurgeOldestFiles(m_MaxNumFiles);
		}

		~CompilerCacheFolder() = default;

		std::string GetFilePrefix() const override { return "sound_graph_cache_"; }

		void StoreItemInCache(const char* key, const void* sourceData, uint64_t size) override
		{
			{
				std::scoped_lock sl(m_Lock);
				auto file = GetFileForKey(key);
				//file.replaceWithData(sourceData, (size_t)size);
				//std::filesystem::path tempFile = file / "_temp";

				std::ofstream stream(file, std::ios::binary | std::ios::trunc);

				if (!stream)
				{
					stream.close();
					ZONG_CORE_ASSERT(false);
					return;
				}

				stream.write((char*)sourceData, size);
				stream.close();
			}

			//if (std::filesystem::exists(file))
			//    remove(file);
			//rename(tempFile.string().c_str(), file.string().c_str());

			PurgeOldestFiles(m_MaxNumFiles);
		}

		uint64_t ReadItemFromCache(const char* key, void* destAddress, uint64_t destSize) override
		{
			std::scoped_lock sl(m_Lock);
			auto file = GetFileForKey(key);
			uint64_t fileSize = std::filesystem::exists(file) ? (uint64_t)std::filesystem::file_size(file) : 0;

			if (fileSize == 0)
			{
				StoreCacheResult({ key, EError::FailedToFindFile });
				return 0;
			}

			if (destAddress == nullptr || destSize < fileSize)
				return fileSize;

			auto readEntireFile = [&]() -> bool
			{
				std::ifstream fin(file.c_str(), std::ios::binary | std::ios::ate);

				if (fin.good())
				{
					auto end = fin.tellg();
					fin.seekg(0, std::ios::beg);
					auto size = end - fin.tellg();
					ZONG_CORE_ASSERT(size != 0);

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
				return 0;

			std::filesystem::last_write_time(file, std::filesystem::file_time_type::clock::now());

			return fileSize;
		}

		bool PurgeOldestFiles(uint32_t maxNumFilesToRetain) const
		{
			using filepath = std::filesystem::path;
			std::vector<filepath> files;

			bool anyFailed = false;

			{
				std::scoped_lock sl(m_Lock);

				for (const auto& dirEntry : std::filesystem::directory_iterator(m_Folder))
				{
                    if (dirEntry.path().filename().string().rfind(GetFilePrefix(), 0) == 0)
						files.push_back(dirEntry.path());
				}

				if (files.size() > maxNumFilesToRetain)
				{
					std::sort(files.begin(), files.end(), [](const filepath& lhs, const filepath& rhs)
					{
						return std::filesystem::last_write_time(lhs) < std::filesystem::last_write_time(rhs);
					});

					for (size_t i = 0; i < files.size() - maxNumFilesToRetain; ++i)
						anyFailed |= !remove(files[i]);
				}
			}

			return !anyFailed;
		}

		std::filesystem::path GetFileForKey(const char* cacheKey) const final { return m_Folder / GetFileName(cacheKey); }

	private:
		uint32_t m_MaxNumFiles;
		std::filesystem::path m_Folder;
	};

} // namespace Engine

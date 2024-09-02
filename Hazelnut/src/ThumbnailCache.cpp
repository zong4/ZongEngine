#include "hzpch.h"
#include "ThumbnailCache.h"

#include "Hazel/Project/Project.h"

#include "Hazel/Serialization/FileStream.h"

#include "Hazel/Renderer/Renderer.h"

namespace Hazel {

	namespace Utils {

		static std::filesystem::path GetCacheDirectory()
		{
			return "Resources/Cache/Thumbnails";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::filesystem::path cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);
		}

		static std::filesystem::path GetThumbnailFilepath(AssetHandle assetHandle)
		{
			return Utils::GetCacheDirectory() / std::format("{}.thumbnail", assetHandle);
		}
	}

	ThumbnailCache::ThumbnailCache()
	{
	}

	ThumbnailCache::~ThumbnailCache()
	{
	}


	bool ThumbnailCache::HasThumbnail(AssetHandle assetHandle)
	{
		bool isInMemoryCache = m_ThumbnailImages.contains(assetHandle);
		if (isInMemoryCache)
			return true;

		std::filesystem::path filepath = Utils::GetThumbnailFilepath(assetHandle);
		return FileSystem::Exists(filepath);
	}

	bool ThumbnailCache::IsThumbnailCurrent(AssetHandle assetHandle)
	{
		if (!HasThumbnail(assetHandle))
			return false;

		uint64_t thumbnailTimestamp = 0;
		if (m_ThumbnailImages.contains(assetHandle))
			thumbnailTimestamp = m_ThumbnailImages.at(assetHandle).LastWriteTime;
		else
			thumbnailTimestamp = ReadTimestampFromDisk(assetHandle);

		// TODO(Yan): possibly move this to asset system - it should be aware of unloaded assets on disk too
		uint64_t lastWriteTime = FileSystem::GetLastWriteTime(Project::GetEditorAssetManager()->GetFileSystemPath(assetHandle));
		return thumbnailTimestamp == lastWriteTime;
	}

	Ref<Image2D> ThumbnailCache::GetThumbnailImage(AssetHandle assetHandle)
	{
		if (!HasThumbnail(assetHandle))
			return nullptr;

		if (!m_ThumbnailImages.contains(assetHandle))
			LoadFromDiskToMemoryCache(assetHandle);
			
		return m_ThumbnailImages.at(assetHandle).Image;
	}

	void ThumbnailCache::SetThumbnailImage(AssetHandle assetHandle, Ref<Image2D> image)
	{
		uint64_t LastWriteTime = Project::GetEditorAssetManager()->GetMetadata(assetHandle).FileLastWriteTime;
		m_ThumbnailImages[assetHandle] = { image, LastWriteTime };
		WriteThumbnailFile(assetHandle, image, LastWriteTime);
	}

	void ThumbnailCache::Clear()
	{
		m_ThumbnailImages.clear();
	}

	bool ThumbnailCache::LoadFromDiskToMemoryCache(AssetHandle assetHandle)
	{
		std::filesystem::path filepath = Utils::GetThumbnailFilepath(assetHandle);
		if (!FileSystem::Exists(filepath))
			return false;

		FileStreamReader stream(filepath);

		ThumbnailFileHeader header;
		stream.ReadRaw(header);

		Buffer imageDataBuffer;
		stream.ReadBuffer(imageDataBuffer);

		// NOTE(Yan): What if thumbnail size preference differs from cached image?
		ImageSpecification spec;
		spec.DebugName = "ThumbnailCache-Image";
		spec.Width = header.Width;
		spec.Height = header.Height;
		spec.Transfer = true;
		Ref<Image2D> image = Image2D::Create(spec);
		image->Invalidate();
		image->SetData(imageDataBuffer);
		imageDataBuffer.Release();
		m_ThumbnailImages[assetHandle] = { image, header.LastWriteTime };
		return true;
	}

	uint64_t ThumbnailCache::ReadTimestampFromDisk(AssetHandle assetHandle)
	{
		std::filesystem::path filepath = Utils::GetThumbnailFilepath(assetHandle);
		if (!FileSystem::Exists(filepath))
			return 0;
		
		FileStreamReader stream(filepath);
		ThumbnailFileHeader header;
		stream.ReadRaw(header);
		return header.LastWriteTime;
	}

	void ThumbnailCache::WriteThumbnailFile(AssetHandle assetHandle, Ref<Image2D> image, uint64_t lastWriteTime)
	{
		Renderer::Submit([assetHandle, image, lastWriteTime]()
		{
			Utils::CreateCacheDirectoryIfNeeded();
			std::filesystem::path filepath = Utils::GetThumbnailFilepath(assetHandle);

			// Get image data from GPU
			Buffer imageDataBuffer;
			image->CopyToHostBuffer(imageDataBuffer);

			ThumbnailFileHeader header;
			header.LastWriteTime = lastWriteTime;
			header.Width = image->GetWidth();
			header.Height = image->GetHeight();

			FileStreamWriter stream(filepath);
			stream.WriteRaw(header);
			stream.WriteBuffer(imageDataBuffer);

			imageDataBuffer.Release();
		});
	}

}

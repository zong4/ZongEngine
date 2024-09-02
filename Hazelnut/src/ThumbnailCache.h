#pragma once

#include "Hazel/Renderer/Image.h"
#include "Hazel/Asset/Asset.h"

#include "ThumbnailGenerator.h"

namespace Hazel {

	class ThumbnailCache : public RefCounted
	{
	public:
		ThumbnailCache();
		virtual ~ThumbnailCache();

		bool HasThumbnail(AssetHandle assetHandle);
		bool IsThumbnailCurrent(AssetHandle assetHandle);
		Ref<Image2D> GetThumbnailImage(AssetHandle assetHandle);
		void SetThumbnailImage(AssetHandle assetHandle, Ref<Image2D> image);

		void Clear();
	private:
		bool LoadFromDiskToMemoryCache(AssetHandle assetHandle);
		uint64_t ReadTimestampFromDisk(AssetHandle assetHandle);
		void WriteThumbnailFile(AssetHandle assetHandle, Ref<Image2D> image, uint64_t lastWriteTime);
	private:
		std::map<AssetHandle, ThumbnailImage> m_ThumbnailImages;
	private:
		struct ThumbnailFileHeader
		{
			const char HEADER[4] = { 'H', 'Z', 'T', 'N' };
			uint16_t Format = 0; // unused
			uint16_t Version = 0; // unused
			uint16_t Width = 0;
			uint16_t Height = 0;
			uint32_t Padding = 0;
			uint64_t LastWriteTime = 0;
			// ImageDataBuffer
		};
	};

}

/*

<AssetHandle>.thumbnail

HEADER
  HZTN - 4 byte magic const
  Format - 2 bytes (compressed? what format?)
  Version - 2 bytes
  Width - 2, Height - 2
  Timestamp - 8 bytes
  ImgDataSize - 8 bytes
  ImgData - variable

*/

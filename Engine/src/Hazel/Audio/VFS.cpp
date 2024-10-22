#include "hzpch.h"
#include "VFS.h"

#include "Hazel/Serialization/StreamReader.h"

namespace Hazel::Audio
{
	struct VFSFile
	{
		StreamReader* Reader = nullptr;
		size_t FileSize = 0;

		~VFSFile()
		{
			delete Reader;
		}
	};

	namespace Callbacks
	{
		ma_result Open(ma_vfs* pVFS, const char* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile)
		{
			if (!pFilePath || !pFile)
				return MA_INVALID_ARGS;

			if (openMode == MA_OPEN_MODE_WRITE)
				return MA_NOT_IMPLEMENTED;

			auto* vfs = (VFS*)pVFS;

			auto* reader = vfs->onCreateReader(pFilePath);
			if (!reader)
				return -1;
			
			size_t filesize = vfs->onGetFileSize(pFilePath);

			*pFile = new VFSFile{ reader, filesize };

			return MA_SUCCESS;
		}

		ma_result OpenW(ma_vfs* pVFS, const wchar_t* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile)
		{
			HZ_CORE_ASSERT(false);
			return MA_NOT_IMPLEMENTED;
		}

		ma_result Close(ma_vfs* pVFS, ma_vfs_file file)
		{
			if (!file)
				return MA_INVALID_ARGS;

			delete (VFSFile*)(file);

			return MA_SUCCESS;
		}

		ma_result Read(ma_vfs* pVFS, ma_vfs_file file, void* pDst, size_t sizeInBytes, size_t* pBytesRead)
		{
			if (!file || !pDst)
				return MA_INVALID_ARGS;

			auto* vfsFile = (VFSFile*)file;
			auto* reader = vfsFile->Reader;

			const size_t toRead = std::min(sizeInBytes, vfsFile->FileSize - reader->GetStreamPosition());
			reader->ReadData((char*)pDst, toRead);

			if (pBytesRead)
				*pBytesRead = toRead;

			if (toRead != sizeInBytes)
				return MA_AT_END;
			else
				return MA_SUCCESS;
		}

		ma_result Write(ma_vfs* pVFS, ma_vfs_file file, const void* pSrc, size_t sizeInBytes, size_t* pBytesWritten)
		{
			HZ_CORE_ASSERT(false);
			return MA_NOT_IMPLEMENTED;
		}

		ma_result Seek(ma_vfs* pVFS, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin)
		{
			if (!file)
				return MA_INVALID_ARGS;

			auto* vfsFile = (VFSFile*)file;
			auto* reader = vfsFile->Reader;

			size_t position = 0;

			switch (origin)
			{
				case ma_seek_origin_start:		position = offset; break;
				case ma_seek_origin_current:	position = reader->GetStreamPosition() + offset; break;
				case ma_seek_origin_end:		position = vfsFile->FileSize + offset; break;
				default:						position = offset; break;
			}

			reader->SetStreamPosition(position);

			return MA_SUCCESS;
		}

		ma_result Tell(ma_vfs* pVFS, ma_vfs_file file, ma_int64* pCursor)
		{
			if (!file || !pCursor)
				return MA_INVALID_ARGS;

			auto* vfsFile = (VFSFile*)file;
			*pCursor = vfsFile->Reader->GetStreamPosition();

			return MA_SUCCESS;
		}

		ma_result Info(ma_vfs* pVFS, ma_vfs_file file, ma_file_info* pInfo)
		{
			if (!file || !pInfo)
				return MA_INVALID_ARGS;

			auto* vfsFile = (VFSFile*)file;
			pInfo->sizeInBytes = vfsFile->FileSize;

			return MA_SUCCESS;
		}
	} // namespace Callbacks

	ma_result copy_callbacks(ma_allocation_callbacks* pDst, const ma_allocation_callbacks* pSrc)
	{
		if (pDst == NULL || pSrc == NULL)
		{
			return MA_INVALID_ARGS;
		}

		if (pSrc->pUserData == NULL && pSrc->onFree == NULL && pSrc->onMalloc == NULL && pSrc->onRealloc == NULL)
		{
			return MA_INVALID_ARGS;
		}
		else
		{
			if (pSrc->onFree == NULL || (pSrc->onMalloc == NULL && pSrc->onRealloc == NULL))
			{
				return MA_INVALID_ARGS;    /* Invalid allocation callbacks. */
			}
			else
			{
				*pDst = *pSrc;
			}
		}

		return MA_SUCCESS;
	}

	ma_result ma_custom_vfs_init(VFS* pVFS, const ma_allocation_callbacks* pAllocationCallbacks)
	{
		if (pVFS == NULL)
		{
			return MA_INVALID_ARGS;
		}

		pVFS->cb.onOpen = Callbacks::Open;
		pVFS->cb.onOpenW = Callbacks::OpenW;
		pVFS->cb.onClose = Callbacks::Close;
		pVFS->cb.onRead = Callbacks::Read;
		pVFS->cb.onWrite = Callbacks::Write;
		pVFS->cb.onSeek = Callbacks::Seek;
		pVFS->cb.onTell = Callbacks::Tell;
		pVFS->cb.onInfo = Callbacks::Info;

		copy_callbacks(&pVFS->allocationCallbacks, pAllocationCallbacks);

		return MA_SUCCESS;
	}

} // namespace Hazel::Audio

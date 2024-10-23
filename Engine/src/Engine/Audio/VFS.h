#pragma once

#include "miniaudio_incl.h"

namespace Hazel
{
	class StreamReader;
}

namespace Hazel::Audio
{
	/** Our VFS used by miniaudio. 
	*/
	struct VFS
	{
		ma_vfs_callbacks cb;
		ma_allocation_callbacks allocationCallbacks;    /* Only used for the wchar_t version of open() on non-Windows platforms. */
		std::function<StreamReader*(const char* filepath)> onCreateReader;
		std::function<size_t(const char* filepath)> onGetFileSize;
	};

	ma_result ma_custom_vfs_init(VFS* pVFS, const ma_allocation_callbacks* pAllocationCallbacks);

} // namespace Hazel::Audio

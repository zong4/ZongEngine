#pragma once

#include "Hazel/Renderer/Texture.h"

#include <filesystem>

namespace Hazel {

	class TextureImporter
	{
	public:
		static Buffer ToBufferFromFile(const std::filesystem::path& path, ImageFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight);
		static Buffer ToBufferFromMemory(Buffer buffer, ImageFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight);
	private:
		const std::filesystem::path m_Path;
	};
}

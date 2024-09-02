#pragma once

#include "FileStream.h"

#include "Hazel/Renderer/Texture.h"

namespace Hazel {

	class TextureRuntimeSerializer
	{
	public:
		struct Texture2DMetadata
		{
			uint32_t Width, Height;
			uint16_t Format;
			uint8_t Mips;
		};
	public:
		static uint64_t SerializeToFile(Ref<TextureCube> textureCube, FileStreamWriter& stream);
		static Ref<TextureCube> DeserializeTextureCube(FileStreamReader& stream);

		static uint64_t SerializeTexture2DToFile(const std::filesystem::path& filepath, FileStreamWriter& stream);
		static uint64_t SerializeTexture2DToFile(Ref<Texture2D> texture, FileStreamWriter& stream);
		static uint64_t SerializeTexture2DToFile(Buffer imageBuffer, const Texture2DMetadata& metadata, FileStreamWriter& stream);
		static Ref<Texture2D> DeserializeTexture2D(FileStreamReader& stream);
	};

}

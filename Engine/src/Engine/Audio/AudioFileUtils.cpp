#include <pch.h>
#include "AudioFileUtils.h"

#include "Engine/Asset/AssetManager.h"
#include "Engine/Utilities/FileSystem.h"
#include "Engine/Utilities/StringUtils.h"

#include "miniaudio.h"
#include "dr_wav.h"
#include "stb_vorbis.c"

namespace Hazel::AudioFileUtils
{
	static std::optional<AudioFileInfo> GetFileInfoWav(const char* filepath)
	{
		drwav wav;
		if (drwav_init_file(&wav, filepath, nullptr))
		{
			ma_uint16 bitDepth = wav.bitsPerSample;
			ma_uint16 channels = wav.channels;
			ma_uint32 sampleRate = wav.sampleRate;
			double duration = (double)wav.totalPCMFrameCount / (double)sampleRate;

			auto dataSize = wav.dataChunkDataSize;
			auto pos = wav.dataChunkDataPos;
			auto fileSize = dataSize + pos;
			//auto sizestr = Utils::BytesToString(fileSize);
			drwav_uninit(&wav);
		
			return AudioFileInfo{ duration, sampleRate, bitDepth, channels, fileSize };
		}
		
		return std::optional<AudioFileInfo>();
	}

	static std::optional<AudioFileInfo> GetFileInfoVorbis(const char* filepath)
	{
		STBVorbisError error;
		if (stb_vorbis* vorbis = stb_vorbis_open_filename(filepath, (int*)&error, nullptr))
		{
			ma_uint16 bitDepth = 0; // ogg does not have bit-depth
			ma_uint16 channels = vorbis->channels;
			ma_uint32 sampleRate = vorbis->sample_rate;
			if (vorbis->total_samples == 0)
				stb_vorbis_stream_length_in_samples(vorbis);

			double duration = (double)vorbis->total_samples / (double)sampleRate;

			const uint64_t fileSize = std::filesystem::file_size(filepath);

			//auto sizestr = Utils::BytesToString(fileSize);
			stb_vorbis_close(vorbis);

			return AudioFileInfo{ duration, sampleRate, bitDepth, channels, fileSize };
		}

		return std::optional<AudioFileInfo>();
	}

	std::optional<AudioFileInfo> GetFileInfo(const AssetMetadata& metadata)
	{
		std::string filepath = Project::GetEditorAssetManager()->GetFileSystemPathString(metadata);
		if (Utils::String::EqualsIgnoreCase(metadata.FilePath.extension().string(), ".wav"))
			return GetFileInfoWav(filepath.c_str());
		else if (Utils::String::EqualsIgnoreCase(metadata.FilePath.extension().string(), ".ogg"))
			return GetFileInfoVorbis(filepath.c_str());
		else
			return std::optional<AudioFileInfo>();
	}

	std::optional<AudioFileInfo> GetFileInfo(const std::filesystem::path& filepath)
	{
		if (Utils::String::EqualsIgnoreCase(filepath.extension().string(), ".wav"))
			return GetFileInfoWav(filepath.string().c_str());
		else if (Utils::String::EqualsIgnoreCase(filepath.extension().string(), ".ogg"))
			return GetFileInfoVorbis(filepath.string().c_str());
		else
			return std::optional<AudioFileInfo>();
	}

	bool IsValidAudioFile(const std::filesystem::path& filepath)
	{
		// TODO: 1. get list of supported formats; 2. optionally open the file and read metadata

		if (Utils::String::EqualsIgnoreCase(filepath.extension().string(), ".wav"))
			return true;
		else if (Utils::String::EqualsIgnoreCase(filepath.extension().string(), ".ogg"))
			return true;

		return false;
	}

	std::string ChannelsToLayoutString(uint16_t numChannels)
	{
		// TODO: should be able to get actual channel layout from some file formats
		//		and should add a function to get the output speaker layout of the audio device

		std::string str;
		switch (numChannels)
		{
		case 1: str = "Mono"; break;
		case 2: str = "Stereo"; break;
		case 3: str = "3.0"; break;
		case 4: str = "Quad"; break;
		case 5: str = "5.0"; break;
		case 6: str = "5.1"; break;
		//case 7: str = "Unknown"; break;
		case 8: str = "7.1"; break;
		default: str = "Unknown layout"; break;
		}

		return str;
	}
}

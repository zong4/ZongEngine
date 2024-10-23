#pragma once

#include "Engine/Asset/Asset.h"
#include "Engine/Serialization/StreamReader.h"
#include "AudioFileUtils.h"

#include <filesystem>

namespace Hazel
{
	// SoundBankFile version history
	// 1 - initial version
	// 2 - changed IndexTable map to use AssetHandle as a Key instead of hashed filepath
	// 
	//==============================================================================
	struct SoundBankFile
	{
		static constexpr uint32_t CurrentVersion = 1;

		struct FileHeader
		{
			char HEADER[4] = { 'H','Z','S','B' };
			uint32_t Version = 2;
			uint32_t AudioFileCount;
			uint32_t _padding;
		};

		struct AudioFileInfo
		{
			uint64_t DataOffset;
			uint64_t PackedSize; // size of data only
			AudioFileUtils::AudioFileInfo Info;
		};

		struct IndexTable
		{
			using Map = std::unordered_map<AssetHandle, AudioFileInfo>;
			Map AudioFiles; //x Hashed audio file name/path

			static uint64_t CalculateSizeRequirements(uint32_t audioFileCount)
			{
				return sizeof(Map::value_type) * audioFileCount;
				//return (sizeof(uint32_t) + sizeof(AudioFileInfo)) * audioFileCount;
			}
		};

		FileHeader Header;
		IndexTable Index;
	};

	//==============================================================================
	class SoundBank : public RefCounted
	{
	public:
		// Tries to load SoundBankFile from existing file
		explicit SoundBank(const std::filesystem::path& path);

		bool IsLoaded() const { return m_Loaded; }
		const std::filesystem::path& GetSoundBankFilePath() const { return m_Path; }

		bool Contains(AssetHandle audioFileAsset) const;
	
		size_t GetFileSize(AssetHandle audioFileAsset) const;
		const AudioFileUtils::AudioFileInfo* GetFileInfo(AssetHandle audioFileAsset) const;

		[[nodiscard]] StreamReader* CreateReaderFor(AssetHandle audioFileAsset) const;

		// Package wave assets (audio files) into a SoundBankFile with data at the end
		static Ref<SoundBank> Create(const std::vector<AssetHandle>& waveAssets, const std::filesystem::path& path);

	private:
		// Default constructor used in 'Create' function
		SoundBank() = default;

	private:
		// 'true' if SoundBank has valid index table and is able to CreateReader for files
		bool m_Loaded = false;
		SoundBankFile m_File;
		std::filesystem::path m_Path;
	};
} // namespace Hazel

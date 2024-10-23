#include "hzpch.h"
#include "SoundBank.h"

#include "Engine/Asset/AssetManager.h"
#include "Engine/Core/Application.h"
#include "Engine/Core/Hash.h"
#include "Engine/Serialization/FileStream.h"

namespace Hazel
{
	//==============================================================================
	class RangedFileReader : public FileStreamReader
	{
	public:
		RangedFileReader(const Ref<const SoundBank>& source, const std::filesystem::path& path, size_t start, size_t size)
			: FileStreamReader(path), m_Start(start), m_End(start + size), m_Source(source)
		{
			FileStreamReader::SetStreamPosition(m_Start);
		}

		uint64_t GetStreamPosition() override
		{
			return FileStreamReader::GetStreamPosition() - m_Start;
		}

		void SetStreamPosition(uint64_t position) override
		{
			FileStreamReader::SetStreamPosition(m_Start + std::min(position, m_End));
		}

		bool ReadData(char* destination, size_t size) override
		{
			const size_t sizeToRead = std::min(size, m_End - GetStreamPosition());
			return FileStreamReader::ReadData(destination, sizeToRead);
		}
	private:
		size_t m_Start, m_End;
		// Storing a reference of the source SoundBank
		// to keep it alive while reader might be using it
		Ref<const SoundBank> m_Source;
	};

	//==============================================================================
	SoundBank::SoundBank(const std::filesystem::path& path)
		: m_Path(path)
	{
		// Read index table
		FileStreamReader reader(path);
		if (!reader)
			return;

		reader.ReadRaw(m_File.Header);
		if (memcmp(m_File.Header.HEADER, "HZSB", 4) != 0)
			return;

		if (m_File.Header.Version != SoundBankFile::CurrentVersion)
			return;

		m_Loaded = true;
		for (uint32_t i = 0; i < m_File.Header.AudioFileCount; ++i)
		{
			AssetHandle handle;
			reader.ReadRaw(handle);
			auto& audioFileInfo = m_File.Index.AudioFiles[handle];
			reader.ReadRaw(audioFileInfo);
		}

		HZ_CONSOLE_LOG_TRACE("Audio: SoundBank: Successfully loaded SoundBank. Number of files in the SoundBank - {}", m_File.Header.AudioFileCount);
	}

	bool SoundBank::Contains(AssetHandle audioFileAsset) const
	{
		return m_File.Index.AudioFiles.count(audioFileAsset);
	}

	size_t SoundBank::GetFileSize(AssetHandle audioFileAsset) const
	{
		if (!Contains(audioFileAsset))
			return 0;

		return m_File.Index.AudioFiles.at(audioFileAsset).PackedSize;
	}

	const AudioFileUtils::AudioFileInfo* SoundBank::GetFileInfo(AssetHandle audioFileAsset) const
	{
		if (!Contains(audioFileAsset))
			return {};

		return &m_File.Index.AudioFiles.at(audioFileAsset).Info;
	}

	StreamReader* SoundBank::CreateReaderFor(AssetHandle audioFileAsset) const
	{
		if (!m_Loaded)
			return nullptr;

		if (!Contains(audioFileAsset))
		{
			if (!Application::IsRuntime())
			{
				return nullptr;
			}
			else
			{
				HZ_CORE_VERIFY(false);
			}
		}

		const auto& audioFileInfo = m_File.Index.AudioFiles.at(audioFileAsset);

		// Custom reader that reads only a section of a file
		auto reader = new RangedFileReader(this, m_Path, audioFileInfo.DataOffset, audioFileInfo.PackedSize);
		if (!(*reader))
		{
			delete reader;
			reader = nullptr;
		}

		return reader;
	}

	Ref<SoundBank> SoundBank::Create(const std::vector<AssetHandle>& waveAssets, const std::filesystem::path& path)
	{
		HZ_CORE_VERIFY(!Application::IsRuntime());

		// Error checking
		{
			std::string errorMsg;

			if (waveAssets.empty())
				errorMsg = "No Wave Assets were found reference by 'Play' audio trigger events.";
			else if (path.empty())
				errorMsg = "Specified Sound Bank path is empty.";
			else if (std::filesystem::exists(path) && std::filesystem::is_directory(path))
				errorMsg = "Specified Sound Bank path is an existing directory.";

			if (!errorMsg.empty())
			{
				HZ_CONSOLE_LOG_ERROR("Failed to create Sound Bank. {}", errorMsg);
				return nullptr;
			}
		}

		if (std::filesystem::exists(path))
			std::filesystem::remove(path);

		auto soundBank = Ref<SoundBank>(new SoundBank());
		soundBank->m_Path = path;

		auto& soundBankFile = soundBank->m_File;

		soundBankFile.Header.Version = SoundBankFile::CurrentVersion;
		soundBankFile.Header.AudioFileCount = (uint32_t)waveAssets.size();

		FileStreamWriter serializer(path);

		// Write header
		serializer.WriteRaw<SoundBankFile::FileHeader>(soundBankFile.Header);

		// ===============
		// Write index
		// ===============
		// Write dummy data for shader programs
		uint64_t indexTableSize = SoundBankFile::IndexTable::CalculateSizeRequirements(soundBankFile.Header.AudioFileCount);
		
		uint64_t indexTablePos = serializer.GetStreamPosition();
		serializer.WriteZero(indexTableSize);

		// Write Audio File Infos
		for (const auto& handle : waveAssets)
		{
			auto audioFile = AssetManager::GetAsset<AudioFile>(handle);
			const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(audioFile);

			auto fileSystemPath = Project::GetEditorAssetManager()->GetFileSystemPath(metadata);
		
			if (!std::filesystem::exists(fileSystemPath))
			{
				HZ_CORE_ASSERT(false);
				continue;
			}	
			
			// Create file info entry
			{
				SoundBankFile::AudioFileInfo info;
				info.DataOffset = serializer.GetStreamPosition();
				info.PackedSize = audioFile->FileSize;

				auto optInfo = AudioFileUtils::GetFileInfo(fileSystemPath.string());
				HZ_CORE_ASSERT(optInfo);
				info.Info = *optInfo;
				
				// Print filepath relative to Assets dir path
				auto serializedPath = std::filesystem::relative(fileSystemPath, Project::GetAssetDirectory());
				const std::string serializedPathString = serializedPath.string();
				HZ_CORE_WARN("SoundBank - {}", serializedPathString);

				soundBankFile.Index.AudioFiles[handle] = info;
			}

			// Read audio file from the audio file and write into SoundBankFile
			Buffer encodedFile;
			FileStreamReader reader(fileSystemPath);
			encodedFile.Allocate((uint32_t)audioFile->FileSize);
			reader.ReadBuffer(encodedFile, (uint32_t)audioFile->FileSize);

			serializer.WriteBuffer(encodedFile, false);
		}

		// Write program index
		serializer.SetStreamPosition(indexTablePos);
		uint64_t begin = indexTablePos;
		for (const auto& [assetHandle, audioFileInfo] : soundBankFile.Index.AudioFiles)
		{
			serializer.WriteRaw(assetHandle);
			serializer.WriteRaw(audioFileInfo);
		}
		uint64_t end = serializer.GetStreamPosition();
		uint64_t s = end - begin;

		soundBank->m_Loaded = true;
		return soundBank;
	}
} // namespace Hazel

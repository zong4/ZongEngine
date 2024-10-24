#include "pch.h"
#include "ResourceManager.h"

#include "AudioEngine.h"
#include "AudioEvents/AudioCommandRegistry.h"

#include "Engine/Asset/AssetManager.h"
#include "Engine/Core/Application.h"
#include "Engine/Editor/NodeGraphEditor/SoundGraph/SoundGraphAsset.h"
#include "Engine/Project/Project.h"
#include "Engine/Serialization/FileStream.h"
#include "Engine/Utilities/StringUtils.h"

#include <filesystem>
#include <unordered_set>
#include <vector>

namespace Engine::Audio
{
	ResourceManager::ResourceManager(MiniAudioEngine& audioEngine)
		: m_AudioEngine(audioEngine)
	{
	}

	ResourceManager::~ResourceManager()
	{
		ReleaseResources();
	}

	void ResourceManager::Initialize()
	{
		if (m_SoundBank || m_LoadedFiles.size())
			ReleaseResources();

		// Try to load previously built Sound Bank
		const std::filesystem::path bankPath(Project::GetAssetDirectory() / "SoundBank.hsb");
		if (std::filesystem::exists(bankPath) && std::filesystem::is_regular_file(bankPath))
		{
			m_SoundBank = Ref<SoundBank>::Create(bankPath);
			ZONG_CORE_ASSERT(m_SoundBank->IsLoaded());
		}
	}

	void ResourceManager::ReleaseResources()
	{
		const std::string bankFilepath = m_SoundBank ? m_SoundBank->GetSoundBankFilePath().string() : "";

		m_SoundBank = nullptr;

		for (auto& [handle, data] : m_LoadedFiles)
		{
			const std::string sourceFileKey = std::to_string(handle);
			ma_result result = ma_resource_manager_unregister_data(m_AudioEngine.m_Engine.pResourceManager, sourceFileKey.c_str());
			ZONG_CORE_ASSERT(result == MA_SUCCESS);

			data.Release();
		}

		const bool anyUnloaded = m_LoadedFiles.size();
		m_LoadedFiles.clear();

		if (!bankFilepath.empty())	ZONG_CONSOLE_LOG_INFO("Sound Bank '{0}' has been unloaded.", bankFilepath);
		else if (anyUnloaded)		ZONG_CONSOLE_LOG_INFO("Sounds loaded with Sound Bank has been unloaded.");
		else						ZONG_CONSOLE_LOG_INFO("No sounds were unloaded with Sound Bank because none has been loaded.");
	}

	std::vector<AssetHandle> ResourceManager::CollectWaveHandlesForPlayAction(const TriggerAction& playAction) const
	{
		ZONG_CORE_ASSERT(playAction.Type == EActionType::Play);

		std::vector<AssetHandle> waveAssets;

		const AssetHandle sourceHandle = playAction.Target->DataSourceAsset;
		const AssetType type = AssetManager::GetAssetType(sourceHandle);

		if (type == AssetType::Audio)
		{
			waveAssets.push_back(sourceHandle);
		}
		else if (type == AssetType::SoundGraphSound)
		{
			if (auto soundGraphAsset = AssetManager::GetAsset<SoundGraphAsset>(sourceHandle))
				waveAssets.insert(waveAssets.end(), soundGraphAsset->WaveSources.begin(), soundGraphAsset->WaveSources.end());
		}

		// Remove duplicates
		{
			std::unordered_set<AssetHandle> set;
			for (const auto& handle : waveAssets)
				set.insert(handle);

			waveAssets.assign(set.begin(), set.end());
		}

		return waveAssets;
	}

	Ref<SoundBank> ResourceManager::BuildSoundBank()
	{
		auto project = Project::GetActive();
		if (!project)
			return nullptr;

		std::vector<AssetHandle> waveAssets;

		// 1. Collect audio file asset references
		const auto& triggers = AudioCommandRegistry::GetRegistry<TriggerCommand>();
		for (const auto& [id, trigger] : triggers)
		{
			for (const auto& action : trigger.Actions)
			{
				if (action.Type == EActionType::Play)
				{
					const auto actionWaves = CollectWaveHandlesForPlayAction(action);
					waveAssets.insert(waveAssets.end(), actionWaves.begin(), actionWaves.end());
				}
			}
		}

		if (waveAssets.empty())
		{
			ZONG_CONSOLE_LOG_INFO("No referenced audio source assets detected, Sound Bank generation aborted.");
			return nullptr;
		}
		
		// 2. Remove duplicates
		{
			std::unordered_set<AssetHandle> set;
			for (const auto& handle : waveAssets)
				set.insert(handle);

			waveAssets.assign(set.begin(), set.end());
			//? DBG
			//std::sort(waveAssets.begin(), waveAssets.end());
		}

		// 3. Package data into a Sound Bank
		const std::filesystem::path bankPath(Project::GetAssetDirectory() / "SoundBank.hsb");

		m_SoundBank = SoundBank::Create(waveAssets, bankPath);
		
		if (m_SoundBank)
		{
			const auto soundBankFileSize = std::filesystem::file_size(m_SoundBank->GetSoundBankFilePath());
			ZONG_CONSOLE_LOG_INFO("Successfully packaged Sound Bank. Number of audio files packaged {0}, Sound Bank size {1}, file path {2}",
				waveAssets.size(), Utils::BytesToString(soundBankFileSize), bankPath.string());
		}

		return m_SoundBank;
	}

	StreamReader* ResourceManager::CreateReaderFor(AssetHandle sourceFile) const
	{
		StreamReader* reader = nullptr;

		// TODO: maybe if not Runtime, try to load from soundbank only if a user preference set to do so,
		//		because SoundBank may not have updated assets that the user is working on
		//		ALTHOUGH, this is what UnloadCurrentSoundBank editor command is for.
		if (m_SoundBank)
		{
			reader = m_SoundBank->CreateReaderFor(sourceFile);
		}

		if (!reader && !Application::IsRuntime())
		{
			const auto filepath = Project::GetEditorAssetManager()->GetFileSystemPath(sourceFile);

			reader = new FileStreamReader(filepath);
			if (!(*reader))
			{
				delete reader;
				reader = nullptr;
			}
		}

		return reader;
	}

	size_t ResourceManager::GetFileSize(AssetHandle sourceFile) const
	{
		size_t size = 0;
		if (m_SoundBank)
			size = m_SoundBank->GetFileSize(sourceFile);

		if (size == 0 && !Application::IsRuntime())
		{
			const auto filepath = Project::GetEditorAssetManager()->GetFileSystemPath(sourceFile);
			size = std::filesystem::file_size(filepath);
		}
		
		return size;
	}

	StreamReader* ResourceManager::CreateReaderFor(const std::string& sourceFile) const
	{
		ZONG_CORE_ASSERT(!std::filesystem::exists(sourceFile));

		// We access audio files only by AssetHandle, stringified for miniaudio
		const AssetHandle handle = std::stoull(sourceFile);
		return CreateReaderFor(handle);
	}

	size_t ResourceManager::GetFileSize(const std::string& sourceFile) const
	{
		ZONG_CORE_ASSERT(!std::filesystem::exists(sourceFile));

		// We access audio files only by AssetHandle, stringified for miniaudio
		const AssetHandle handle = std::stoull(sourceFile);
		return GetFileSize(handle);
	}

	bool ResourceManager::PreloadAudioFile(AssetHandle sourceFile)
	{
		if (!m_SoundBank)
			return false;

		if (m_LoadedFiles.count(sourceFile))
			return true;

		if (!m_SoundBank->Contains(sourceFile))
			return false;

		const auto* fileInfo = m_SoundBank->GetFileInfo(sourceFile);
		if (!fileInfo)
			return false;

		// We don't preload streaming files
		if (fileInfo->Duration >= MiniAudioEngine::Get().GetUserConfiguration().FileStreamingDurationThreshold)
			return true;

		// Create special reader that reads from SoundBank
		StreamReader* reader = m_SoundBank->CreateReaderFor(sourceFile);
		if (!reader || !(*reader))
			return false;

		// Load full file into a buffer
		Buffer encodedFile;
		encodedFile.Allocate((uint32_t)fileInfo->FileSize);
		reader->ReadBuffer(encodedFile, (uint32_t)fileInfo->FileSize);
		delete reader;

		const std::string sourceFileKey = std::to_string(sourceFile);

		// Regiter our buffer with miniaudio resource manager
		ma_result result = ma_resource_manager_register_encoded_data(m_AudioEngine.m_Engine.pResourceManager, sourceFileKey.data(), encodedFile.Data, fileInfo->FileSize);
		if (result != MA_SUCCESS)
		{
			ZONG_CORE_ASSERT(false);
			return false;
		}

		// Store our loaded file buffer in a map
		m_LoadedFiles.emplace(sourceFile, encodedFile);

		return true;
	}

	bool ResourceManager::ReleaseAudioFile(AssetHandle sourceFile)
	{
		if (!m_LoadedFiles.count(sourceFile))
			return true;

		const std::string sourceFileKey = std::to_string(sourceFile);

		ma_result result = ma_resource_manager_unregister_data(m_AudioEngine.m_Engine.pResourceManager, sourceFileKey.c_str());
		if (result != MA_SUCCESS)
		{
			ZONG_CORE_ASSERT(false);
			return false;
		}

		m_LoadedFiles.at(sourceFile).Release();
		m_LoadedFiles.erase(sourceFile);

		return true;
	}

	void ResourceManager::PreloadEventSources(Audio::CommandID eventID)
	{
		const auto& triggers = AudioCommandRegistry::GetRegistry<TriggerCommand>();
		if (!triggers.count(eventID))
			return;

		for (const auto& action : triggers.at(eventID).Actions)
		{
			if (!action.Target)
				continue;

			if (action.Type == EActionType::Play)
			{
				const auto actionWaves = CollectWaveHandlesForPlayAction(action);

				for (const auto& waveHandle : actionWaves)
				{
					PreloadAudioFile(waveHandle);
				}
			}
		}
	}

	void ResourceManager::UnloadEventSources(Audio::CommandID eventID)
	{
		const auto& triggers = AudioCommandRegistry::GetRegistry<TriggerCommand>();
		if (!triggers.count(eventID))
			return;

		for (const auto& action : triggers.at(eventID).Actions)
		{
			if (action.Type == EActionType::Play)
			{
				const auto actionWaves = CollectWaveHandlesForPlayAction(action);

				for (const auto& waveHandle : actionWaves)
				{
					ReleaseAudioFile(waveHandle);
				}
			}
		}
	}

	bool ResourceManager::IsStreaming(AssetHandle sourceFile) const
	{
		if (m_SoundBank)
		{
			if (const auto* fileInfo = m_SoundBank->GetFileInfo(sourceFile))
				return fileInfo->Duration >= MiniAudioEngine::Get().GetUserConfiguration().FileStreamingDurationThreshold;
		}
		else if (!Application::IsRuntime())
		{
			// Long path - opeing file with decoder and reading file info
			// TODO: in the future we may want to store this information in AudioFile asset
			const auto filepath = Project::GetEditorAssetManager()->GetFileSystemPath(sourceFile);
			if (auto info = AudioFileUtils::GetFileInfo(filepath))
				return info->Duration >= MiniAudioEngine::Get().GetUserConfiguration().FileStreamingDurationThreshold;
		}

		return false;
	}

} // namespace Engine::Audio

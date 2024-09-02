#pragma once

#include "AudioEngine.h"
#include "SoundBank.h"

#include "Hazel/Core/Buffer.h"

namespace Hazel
{
	class StreamReader;
}

namespace Hazel::Audio
{
	class ResourceManager
	{
	public:
		explicit ResourceManager(MiniAudioEngine& audioEngine);
		~ResourceManager();

		void Initialize();
		void ReleaseResources();

		Ref<SoundBank> BuildSoundBank(const std::filesystem::path& bankPath);
		Ref<SoundBank> GetSoundBank() { return m_SoundBank; };

		//? JP. Should we just pass Ref<AudioFile> to ensure the correct type and have all the data ready?
		StreamReader* CreateReaderFor(AssetHandle sourceFile) const;
		size_t GetFileSize(AssetHandle sourceFile) const;

		/** Must only be used with stringified AssetHandle. */
		StreamReader* CreateReaderFor(const std::string& sourceFile) const;
		/** Must only be used with stringified AssetHandle. */
		size_t GetFileSize(const std::string& sourceFile) const;
		
		bool PreloadAudioFile(AssetHandle sourceFile);
		bool ReleaseAudioFile(AssetHandle sourceFile); //? For now just 'unregistres' the audio file data with miniaudio and clears our managed buffer.
		
		void PreloadEventSources(Audio::CommandID eventID);
		void UnloadEventSources(Audio::CommandID eventID);

		bool IsStreaming(AssetHandle sourceFile) const;

	private:
		std::vector<AssetHandle> CollectWaveHandlesForPlayAction(const TriggerAction& playAction) const;

	private:
		MiniAudioEngine& m_AudioEngine;
		Ref<SoundBank> m_SoundBank = nullptr;
		// It is important to keep this map alive while we read
		// any audio data from the SoundBank
		std::unordered_map<AssetHandle, Buffer> m_LoadedFiles;
	};
} // namespace Hazel::Audio

#pragma once

#include "Hazel/Core/Application.h"
#include "Hazel/Core/Buffer.h"
#include "Hazel/Asset/Asset.h"

#include "ResourceManager.h"

#include "miniaudio.h"
#include "miniaudio_engine.h"
#include <unordered_map>
#include <filesystem>

namespace Hazel::Audio
{
	static constexpr auto s_ConversionBufferSize = 4096; // TODO: get this value from somewhere more sensible

	struct DataSourceContext
	{
		struct Reader
		{
			ma_resource_manager_data_source Source;
			int64_t TotalFrames;
			uint32_t NumChannels;
			ma_channel_converter Converter;
			BufferSafe Buffer;

			// Returns number of frames read
			uint64_t Read(float* destination, uint64_t numFramesToRead, uint64_t readPositionInSource, uint64_t startFrameInSource)
			{
				HZ_CORE_ASSERT(destination != nullptr);

				if (numFramesToRead == 0) return 0;

				ma_result result;
				ma_uint64 framesRead = 0;

				result = ma_resource_manager_data_source_seek_to_pcm_frame(&Source, readPositionInSource);
				HZ_CORE_ASSERT(result == MA_SUCCESS);

				result = ma_resource_manager_data_source_read_pcm_frames(&Source, destination, numFramesToRead, &framesRead);
				HZ_CORE_ASSERT(result == MA_SUCCESS ||
							   result == MA_AT_END && (Source.flags & MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_DECODE));

				//! If data source set to DECODE, miniaudio doesn't handle looping, it just stopps at the end,
				//! assuming the operation was successful. In which case we try again to read the rest.
				const uint32_t leftToRead = (uint32_t)(numFramesToRead - framesRead);
				if (leftToRead > 0 && (Source.flags & MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_DECODE))
				{
					// Wrapp around start position
					result = ma_resource_manager_data_source_seek_to_pcm_frame(&Source, startFrameInSource);
					HZ_CORE_ASSERT(result == MA_SUCCESS);

					ma_uint64 secondRead = 0;
					result = ma_resource_manager_data_source_read_pcm_frames(&Source, destination + framesRead * NumChannels, leftToRead, &secondRead);
					HZ_CORE_ASSERT(result == MA_SUCCESS);

					framesRead += secondRead;

					HZ_CORE_ASSERT(framesRead == numFramesToRead);
				}

				// Up-convert mono input source to stereo output
				if (NumChannels == 1)
				{
					Buffer.Write(destination, (uint32_t)(numFramesToRead * sizeof(float)));
					result = ma_channel_converter_process_pcm_frames(&Converter, destination, Buffer.Data, numFramesToRead);
					HZ_CORE_ASSERT(result == MA_SUCCESS);
				}

				return framesRead;
			}

			bool IsAtEnd()
			{
				ma_uint64 cursor, length;
				ma_result r = ma_resource_manager_data_source_get_cursor_in_pcm_frames(&Source, &cursor);
				HZ_CORE_ASSERT(r == MA_SUCCESS);
				r = ma_resource_manager_data_source_get_length_in_pcm_frames(&Source, &length);
				HZ_CORE_ASSERT(r == MA_SUCCESS);

				return cursor == length;
			}
		};

		std::unordered_map<AssetHandle, Reader> Readers;

		bool InitializeReader(ma_resource_manager* resourceManager, AssetHandle waveAssetHandle)
		{
			if (!Application::IsRuntime())
			{
				if (!AssetManager::IsAssetHandleValid(waveAssetHandle))
				{
					// TODO: remove "empty" wave refs before this stage (?)
					HZ_CORE_ASSERT(false);
					return false;
				}

				const auto& assetMetadata = Project::GetEditorAssetManager()->GetMetadata(waveAssetHandle);
				HZ_CORE_ASSERT(assetMetadata.Type == AssetType::Audio);

				const std::string filepath = Project::GetEditorAssetManager()->GetFileSystemPathString(assetMetadata);

				//? This may be unnecessary, as we're only interested in auido files, not meta assets
				if (!std::filesystem::exists(filepath))
				{
					HZ_CORE_ASSERT(false);
					return false;
				}
			}
			else
			{
				// Verify that the handle is a valid AudioFile asset
				Ref<AudioFile> audioFile = AssetManager::GetAsset<AudioFile>(waveAssetHandle);
				HZ_CORE_VERIFY(audioFile);
			}

			auto [it, is_inserted] = Readers.insert_or_assign(waveAssetHandle, Reader{});
			if (!is_inserted)
			{
				// It can fail if the asset is already there
				return false;
			}

			//? For now preloading is handled only via C# API, later we might want to move some sort of automatic preloading to Resource Manager
			//MiniAudioEngine::Get().GetResourceManager()->PreloadAudioFile(filepath);

			auto& reader = it->second;

#if 0
			{
				//? JP. I believe this is here because the filepath 'key' is different in runtime,
				//?		and the following ..data_source_init call accesses preloaded data by said key

				auto path = std::filesystem::relative(filepath, Project::GetActiveAssetDirectory());
				if (MiniAudioEngine::Get().GetResourceManager()->GetSoundBank()->Contains(path.string()))
					filepath = path.string();
			}
#endif
			const std::string sourceFile = std::to_string(waveAssetHandle);

			// TODO: flags (streaming, decoding, etc.)
			ma_uint32 flags = MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_DECODE;
			ma_result result;
			result = ma_resource_manager_data_source_init(resourceManager, sourceFile.c_str(), flags, nullptr, &reader.Source);
			HZ_CORE_ASSERT(result == MA_SUCCESS);

			result = ma_resource_manager_data_source_set_looping(&reader.Source, true);
			HZ_CORE_ASSERT(result == MA_SUCCESS);

			ma_uint64 length;
			result = ma_resource_manager_data_source_get_length_in_pcm_frames(&reader.Source, &length);
			HZ_CORE_ASSERT(result == MA_SUCCESS);

			reader.TotalFrames = length;
			ma_format sourceFormat;
			ma_uint32 sampleRate;	 //? not used for now
			ma_channel channelMap;	 //? not used for now
			ma_resource_manager_data_source_get_data_format(&reader.Source, &sourceFormat, &reader.NumChannels, &sampleRate, &channelMap, 0);

			// For now we only support f32 sample format
			HZ_CORE_ASSERT(sourceFormat == ma_format_f32);
			// For now we only support mono and stereo files
			HZ_CORE_ASSERT(reader.NumChannels > 0 && reader.NumChannels <= 2);

			// We up-convert mono file sources to stereo for now.
			// This may change in the future.
			if (reader.NumChannels == 1)
			{
				ma_channel_converter_config config = ma_channel_converter_config_init(ma_format_f32,				  // Sample format
																					  1,							  // Input channels
																					  NULL,							  // Input channel map
																					  2,							  // Output channels
																					  NULL,							  // Output channel map
																					  ma_channel_mix_mode_default);	  // The mixing algorithm to use when combining channels.

				result = ma_channel_converter_init(&config, /*nullptr, */&reader.Converter);
				HZ_CORE_ASSERT(result == MA_SUCCESS);

				// Allocate buffer for channel convertion
				reader.Buffer.Allocate(s_ConversionBufferSize * sizeof(float));
			}

			return true;
		}

		void Uninitialize()
		{
			if (Readers.empty())
				return;

			for (std::pair<const AssetHandle, Reader>& reader : Readers)
				ma_resource_manager_data_source_uninit(&reader.second.Source);

			Readers.clear();
		}

		/** Technically this is never going to return true, because all data sources are set to loop by default.*/
		bool AreAllDataSourcesAtEnd()
		{
			if (Readers.empty())
				return true;

			for (std::pair<const AssetHandle, Reader>& reader : Readers)
			{
				if (!reader.second.IsAtEnd())
					return false;
			}

			return true;
		}
	};

} // namespace Hazel::Audio

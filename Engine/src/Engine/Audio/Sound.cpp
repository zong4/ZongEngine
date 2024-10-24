#include "pch.h"
#include "Sound.h"
#include "AudioEngine.h"

#include "ResourceManager.h"

#include "Engine/Asset/AssetManager.h"
#include "Engine/Core/Application.h"

namespace Engine
{
    using namespace Audio;
    using EPlayState = ESoundPlayState;

#if 0 // Enable logging of playback state changes
    #define LOG_PLAYBACK(...) ZONG_CORE_INFO(__VA_ARGS__)
#else
    #define LOG_PLAYBACK(...)
#endif

    //==============================================================================

    Sound::~Sound()
    {
        // Technically, we should never have to unitit source here, because they are persistant with pool
        // and uninitialized when the engine is shut down. At least at this moment.
    }

    bool Sound::InitializeDataSource(const Ref<SoundConfig>& config, MiniAudioEngine* audioEngine)
    {
        ZONG_CORE_ASSERT(!IsPlaying());
        ZONG_CORE_ASSERT(!bIsReadyToPlay);
        
        // Reset Finished flag so that we don't accidentally release this voice again while it's starting for the new source
        bFinished = false;

        // TODO: handle passing in different flags for decoding (from data source asset)
        // TODO: and handle decoding somewhere else, in some other way
        
        if (!config->DataSourceAsset)
            return false;

		const std::string sourceFile = std::to_string(config->DataSourceAsset);

		if (Application::IsRuntime())
		{
			if (!audioEngine->GetResourceManager()->GetSoundBank()->Contains(config->DataSourceAsset))
			{
				ZONG_CORE_ASSERT(false);
				return false;
			}

			m_DebugName = std::to_string(config->DataSourceAsset);
		}
		else
		{
			if (AssetManager::GetAssetType(config->DataSourceAsset) != AssetType::Audio)
			{
				ZONG_CORE_ASSERT(false);
				return false;
			}

			m_DebugName = Project::GetEditorAssetManager()->GetFileSystemPath(config->DataSourceAsset).string();
		}


		//? For now preloading is handled only via C# API, later we might want to move some sort of automatic preloading to Resource Manager
		//audioEngine->GetResourceManager()->PreloadAudioFile(filepath);

        ma_result result;

		// TODO: move this whole init_from_file block to Source Manager
		const bool streaming = audioEngine->GetResourceManager()->IsStreaming(config->DataSourceAsset);
		ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION | (streaming ? MA_SOUND_FLAG_STREAM : 0);

        result = ma_sound_init_from_file(&audioEngine->m_Engine, sourceFile.c_str(), flags, nullptr, nullptr, &m_Sound);

        if (result != MA_SUCCESS)
            return false;

        InitializeEffects(config);

        // TODO: handle using parent's (parent group) spatialization vs override (config probably would be passed down here from parrent)
        const bool isSpatializationEnabled = config->bSpatializationEnabled;

        // Setting base Volume and Pitch
        m_Volume = config->VolumeMultiplier;
        m_Pitch = config->PitchMultiplier;
        ma_sound_set_volume(&m_Sound, config->VolumeMultiplier);
        ma_sound_set_pitch(&m_Sound, config->PitchMultiplier);
        
        SetLooping(config->bLooping);

        bIsReadyToPlay = result == MA_SUCCESS;
        return result == MA_SUCCESS;
    }

    void Sound::InitializeEffects(const Ref<SoundConfig>& config)
    {
        ma_node_base* currentHeaderNode = &m_Sound.engineNode.baseNode;
        
        // TODO: implement a proper init-uninit routine in SourceManager
        
        // High-Pass, Low-Pass filters

        m_LowPass.Initialize(m_Sound.engineNode.pEngine, currentHeaderNode);
        currentHeaderNode = m_LowPass.GetNode();

        m_HighPass.Initialize(m_Sound.engineNode.pEngine, currentHeaderNode);
        currentHeaderNode = m_HighPass.GetNode();

       // m_LowPass.SetCutoffValue(config->LPFilterValue);
       // m_HighPass.SetCutoffValue(config->HPFilterValue);
		m_LowPass.SetCutoffFrequency(Audio::NormalizedToFrequency(config->LPFilterValue));
        m_HighPass.SetCutoffFrequency(Audio::NormalizedToFrequency(config->HPFilterValue));

        // Reverb send
        
        ma_result result = MA_SUCCESS;
        uint32_t numChannels = ma_node_get_output_channels(&m_Sound, 0);
        ma_splitter_node_config splitterConfig = ma_splitter_node_config_init(numChannels);
        result = ma_splitter_node_init(m_Sound.engineNode.baseNode.pNodeGraph, 
                                        &splitterConfig, 
                                        &m_Sound.engineNode.pEngine->pResourceManager->config.allocationCallbacks, 
                                        &m_MasterSplitter);

        ZONG_CORE_ASSERT(result == MA_SUCCESS);

        // Store the node the sound was connected to
        auto* oldOutput = currentHeaderNode->pOutputBuses[0].pInputNode;

        // Attach splitter node to the old output of the sound
        result = ma_node_attach_output_bus(&m_MasterSplitter, 0, oldOutput, 0);
        ZONG_CORE_ASSERT(result == MA_SUCCESS);

        // Attach sound node to splitter node
        result = ma_node_attach_output_bus(currentHeaderNode, 0, &m_MasterSplitter, 0);
        ZONG_CORE_ASSERT(result == MA_SUCCESS);
        //result = ma_node_attach_output_bus(&m_MasterSplitter, 0, oldOutput, 0);

        // Set volume of the main pass-through output of the splitter to 1.0
        result = ma_node_set_output_bus_volume(&m_MasterSplitter, 0, 1.0f);
        ZONG_CORE_ASSERT(result == MA_SUCCESS);

        // Mute the "FX send" output of the splitter
        result = ma_node_set_output_bus_volume(&m_MasterSplitter, 1, 0.0f);
        ZONG_CORE_ASSERT(result == MA_SUCCESS);

        /* TODO: Refactore this to
            - InitializeFXSend()
            - GetFXSendNode()
            - ...
          */
    }

    void Sound::ReleaseResources()
    {
        // Duck-tape because uninitializing uninitialized na_nodes causes issues
        if (bIsReadyToPlay)
        {
            // Need to check if hasn't already been uninitialized by the engine (or has been initialized at all)
            if (/*m_Sound.engineNode.baseNode.pCachedData != NULL &&*/ m_Sound.pDataSource != NULL)
                ma_sound_uninit(&m_Sound);

            if (m_MasterSplitter.base.pCachedData != NULL && m_MasterSplitter.base._pHeap != NULL)
            {
                ma_allocation_callbacks* allocationCallbacks = &m_Sound.engineNode.pEngine->pResourceManager->config.allocationCallbacks;
                // ma_engine could have been uninitialized by this point, in which case allocation callbacs would've been invalidated
                if (!allocationCallbacks->onFree)
                    allocationCallbacks = nullptr;

                ma_splitter_node_uninit(&m_MasterSplitter, allocationCallbacks);
            }

            m_LowPass.Uninitialize();
            m_HighPass.Uninitialize();

            bIsReadyToPlay = false;
        }
    }

    bool Sound::Play()
    {
        if (!IsReadyToPlay())
            return false;

        ma_result result = MA_ERROR;
        LOG_PLAYBACK("Old state: " + StringFromState(m_PlayState));

        switch (m_PlayState)
        {
        case EPlayState::Stopped:
            bFinished = false;
            result = ma_sound_start(&m_Sound);
            m_PlayState = EPlayState::Starting;
            break;
        case EPlayState::Starting:
            ZONG_CORE_ASSERT(false);
            break;
        case EPlayState::Playing:
            StopNow(false, true);
            bFinished = false;
            result = ma_sound_start(&m_Sound);
            m_PlayState = EPlayState::Starting;
            break;
        case EPlayState::Pausing:
            StopNow(false, false);
            result = ma_sound_start(&m_Sound);
            m_PlayState = EPlayState::Starting;
            break;
        case EPlayState::Paused:
            // Prepare fade-in for un-pause
            ma_sound_set_fade_in_milliseconds(&m_Sound, 0.0f, m_StoredFaderValue, STOPPING_FADE_MS / 2);
            result = ma_sound_start(&m_Sound);
            m_PlayState = EPlayState::Starting;
            break;
        case EPlayState::Stopping:
            StopNow(true, true);
            result = ma_sound_start(&m_Sound);
            m_PlayState = EPlayState::Starting;
            break;
        case EPlayState::FadingOut:
            break;
        case EPlayState::FadingIn:
            break;
        default:
            break;
        }
        LOG_PLAYBACK("New state: " + StringFromState(m_PlayState));
        ZONG_CORE_ASSERT(result == MA_SUCCESS);
        return result == MA_SUCCESS;

        // TODO: consider marking this sound for stop-fade and switching to new one to prevent click on restart
        //       or some other, lower level solution to fade-out stopping while starting to read from the beginning

    }

    bool Sound::Stop()
    {
        bool result = true;
        switch (m_PlayState)
        {
        case EPlayState::Stopped:
            bFinished = true;
            result = false;
            break;
        case EPlayState::Starting:
            StopNow(false, true); // consider stop-fading
            m_PlayState = EPlayState::Stopping;
            break;
        case EPlayState::Playing:
            result = StopFade(STOPPING_FADE_MS);
            m_PlayState = EPlayState::Stopping;
            break;
        case EPlayState::Pausing:
            StopNow(true, true);
            m_PlayState = EPlayState::Stopping;
            break;
        case EPlayState::Paused:
            StopNow(true, true);
            m_PlayState = EPlayState::Stopping;
            break;
        case EPlayState::Stopping:
            StopNow(true, true);
            break;
        case EPlayState::FadingOut:
            break;
        case EPlayState::FadingIn:
            break;
        default:
            break;
        }
        m_LastFadeOutDuration = (float)STOPPING_FADE_MS / 1000.0f;
        LOG_PLAYBACK(StringFromState(m_PlayState));
        return result;
    }

    bool Sound::Pause()
    {
        bool result = true;

        switch (m_PlayState)
        {
        case EPlayState::Playing:
            result = StopFade(STOPPING_FADE_MS);
            m_PlayState = EPlayState::Pausing;
            break;
        case EPlayState::FadingOut:
            break;
        case EPlayState::FadingIn:
            break;
        default:
            // If was called to Pause while not playing
            m_PlayState = EPlayState::Paused;
            result = true;
            break;
        }
        LOG_PLAYBACK(StringFromState(m_PlayState));

        return result;
    }

    bool Sound::StopFade(uint64_t numSamples)
    {
        constexpr double stopFadeTime = (double)STOPPING_FADE_MS * 1.1 / 1000.0;
        m_StopFadeTime = (float)stopFadeTime;

        // Negative volumeBeg starts the fade from the current volume
        ma_sound_set_fade_in_pcm_frames(&m_Sound, -1.0f, 0.0f, numSamples);

        return true;
    }

    bool Sound::StopFade(int milliseconds)
    {
        ZONG_CORE_ASSERT(milliseconds > 0);

        const uint64_t fadeInFrames = (milliseconds * m_Sound.engineNode.fader.config.sampleRate) / 1000;

        return StopFade(fadeInFrames);
    }

    bool Sound::FadeIn(const float duration, const float targetVolume)
    {
        /*if (IsPlaying() || bStopping)
            return false;

        ma_result result;
        result = ma_sound_set_fade_in_milliseconds(&m_Sound, 0.0f, targetVolume, uint64_t(duration * 1000));
        if (result != MA_SUCCESS)
            return false;

        bPaused = false;
        m_StoredFaderValue = targetVolume;*/

        return ma_sound_start(&m_Sound) == MA_SUCCESS;
    }

    bool Sound::FadeOut(const float duration, const float targetVolume)
    {
        //if (!IsPlaying())
        //    return false;

        //// If fading out not completely, store the end value to reference later as "current" value
        //if(targetVolume != 0.0f)
        //    m_StoredFaderValue = targetVolume;

        //m_LastFadeOutDuration = duration;
        //bFadingOut = true;
        //StopFade(int(duration * 1000));

        return true;
    }

    bool Sound::IsPlaying() const
    {
        return m_PlayState != EPlayState::Stopped && m_PlayState != EPlayState::Paused;
    }

    bool Sound::IsFinished() const
    {
        return bFinished/* && !bPaused*/;
    }

    bool Sound::IsStopping() const
    {
        return m_PlayState == EPlayState::Stopping;
    }

    void Sound::SetVolume(float newVolume)
    {
        ma_sound_set_volume(&m_Sound, m_Volume * newVolume);
    }

    void Sound::SetPitch(float newPitch)
    {
        ma_sound_set_pitch(&m_Sound, m_Pitch * newPitch);
    }

    void Sound::SetLooping(bool looping)
    {
        bLooping = looping;
        ma_sound_set_looping(&m_Sound, bLooping);
    }

    float Sound::GetVolume()
    {
        return ma_node_get_output_bus_volume(&m_Sound, 0);
    }

    float Sound::GetPitch()
    {
        return m_Sound.engineNode.pitch;
    }

    void Sound::SetLocation(const glm::vec3& location, const glm::vec3& orientation)
    {
#ifdef SPATIALIZER_TEST

        ma_sound_set_position(&m_Sound, location.x, location.y, location.z);
#else     
        //if(m_Spatializer.IsInitialized())
           // m_Spatializer.SetPositionRotation(location, orientation);
#endif
        //m_AirAbsorptionFilter.UpdateDistance(glm::distance(location, glm::vec3(0.0f, 0.0f, 0.0f)));
    }

    void Sound::SetVelocity(const glm::vec3& velocity /*= { 0.0f, 0.0f, 0.0f }*/)
    {
    }

    void Sound::Update(Engine::Timestep ts)
    {
        auto notifyIfFinished = [&]
        {
            if (ma_sound_at_end(&m_Sound) == MA_TRUE && onPlaybackComplete)
                onPlaybackComplete();
        };

        auto isNodePlaying = [&]
        {
            return ma_sound_is_playing(&m_Sound) == MA_TRUE;
        };

        auto isFadeFinished = [&]
        {
            return m_StopFadeTime <= 0.0;
        };

        m_StopFadeTime = std::max(0.0f, m_StopFadeTime - ts.GetSeconds());

        switch (m_PlayState)
        {
        case EPlayState::Stopped:
            break;
        case EPlayState::Starting:
            if (isNodePlaying())
            {
                m_PlayState = EPlayState::Playing;
                LOG_PLAYBACK("Upd: " + StringFromState(m_PlayState));
            }
            break;
        case EPlayState::Playing:
            if (ma_sound_is_playing(&m_Sound) == MA_FALSE)
            {
                m_PlayState = EPlayState::Stopped;
                bFinished = true;
                notifyIfFinished();
                LOG_PLAYBACK("Upd: " + StringFromState(m_PlayState));
            }
            break;
        case EPlayState::Pausing:
            if (isFadeFinished())
            {
                StopNow(false, false);
                m_PlayState = EPlayState::Paused;
                LOG_PLAYBACK("Upd: " + StringFromState(m_PlayState));
            }
            break;
        case EPlayState::Paused:
            break;
        case EPlayState::Stopping:
            if (isFadeFinished())
            {
                StopNow(true, true);
                m_PlayState = EPlayState::Stopped;
                LOG_PLAYBACK("Upd: " + StringFromState(m_PlayState));
            }
            break;
        case EPlayState::FadingOut:
            break;
        case EPlayState::FadingIn:
            break;
        default:
            break;
        }
    }

    int Sound::StopNow(bool notifyPlaybackComplete /*= true*/, bool resetPlaybackPosition /*= true*/)
    {
        // Stop reading the data source
        ma_sound_stop(&m_Sound);

        if (resetPlaybackPosition)
        {
            // Reset data source read position to the beginning of the data
            ma_sound_seek_to_pcm_frame(&m_Sound, 0);

            // Mark this voice to be released.
            bFinished = true;

            ZONG_CORE_ASSERT(m_PlayState != EPlayState::Starting);
            m_PlayState = EPlayState::Stopped;
        }
        m_Sound.engineNode.fader.volumeEnd = 1.0f;

        // Need to notify AudioEngine of completion,
        // if this is one shot, AudioComponent needs to be destroyed.
        if (notifyPlaybackComplete && onPlaybackComplete)
            onPlaybackComplete();

        return 1; //m_SoundSourceID;
    }

    float Sound::GetCurrentFadeVolume()
    {
        // TODO: return volume accounted for distance attenuation, or better read output volume envelope.
        float currentVolume = ma_sound_get_current_fade_volume(&m_Sound);

        return currentVolume;
    }

    float Sound::GetPriority()
    {
        return GetCurrentFadeVolume() * ((float) m_Priority / 255.0f);
    }

    float Sound::GetPlaybackPercentage()
    {
        ma_uint64 currentFrame;
        ma_uint64 totalFrames;
        ma_sound_get_cursor_in_pcm_frames(&m_Sound, &currentFrame);

        // TODO: concider storing this value when initializing sound;
        ma_sound_get_length_in_pcm_frames(&m_Sound, &totalFrames);


        return (float)currentFrame / (float)totalFrames;
    }

} // namespace Engine

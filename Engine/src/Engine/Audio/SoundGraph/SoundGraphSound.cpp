#include "pch.h"
#include "SoundGraphSound.h"
#include "Engine/Audio/AudioEngine.h"
#include "Engine/Audio/SoundGraph/SoundGraphSource.h"
#include "Engine/Audio/SoundGraph/SoundGraph.h"

#include "Engine/Asset/AssetManager.h"
// TODO: bad. move asset file to engine.
#include "Engine/Editor/NodeGraphEditor/SoundGraph/SoundGraphAsset.h"

#include "Engine/Debug/Profiler.h"

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
    SoundGraphSound::SoundGraphSound()
    {
    }

    SoundGraphSound::~SoundGraphSound()
    {
        // Technically, we should never have to unitit source here, because they are persistant with pool
        // and uninitialized when the engine is shut down. At least at this moment.
    }

	bool Engine::SoundGraphSound::InitializeAudioCallback()
	{
		bool result = true;

		m_Source = CreateScope<SoundGraphSource>();
		Audio::AudioCallback::BusConfig busConfig;
		busConfig.InputBuses.push_back(2);
		busConfig.OutputBuses.push_back(2);
		
		result = m_Source->Initialize(MiniAudioEngine::GetMiniaudioEngine(), busConfig);

		if (!result)
			return false;

		// Need to make sure the Context / Player is running before posting any events or parameter changes
		result = m_Source->StartNode();

		return result;
	}

    bool SoundGraphSound::InitializeDataSource(const Ref<SoundConfig>& config, MiniAudioEngine* audioEngine, const Ref<SoundGraph::SoundGraph>& source)
    {
        ZONG_CORE_ASSERT(!IsPlaying());
        ZONG_CORE_ASSERT(!bIsReadyToPlay);
        ZONG_CORE_ASSERT(source);

        // Reset Finished flag so that we don't accidentally release this voice again while it's starting for the new source
        bFinished = false;

        // TODO: handle passing in different flags for decoding (from data source asset)
        // TODO: and handle decoding somewhere else, in some other way

        if (!config->DataSourceAsset)
            return false;

		
        AssetType type = AssetManager::GetAssetType(config->DataSourceAsset);
        if (type != AssetType::SoundGraphSound)
        {
            ZONG_CORE_ASSERT(false);
            return false;
        }

		if (Application::IsRuntime())
		{
			m_DebugName = std::to_string(config->DataSourceAsset);
		}
		else
		{
			auto filepath = Project::GetEditorAssetManager()->GetFileSystemPath(config->DataSourceAsset);
			m_DebugName = Utils::GetFilename(filepath.string());
		}

		const bool callbackInitialized = InitializeAudioCallback();

		if (!callbackInitialized)
		{
			ZONG_CORE_ASSERT(false, "Failed to initialize Audio Callback for SoundGraphSound.");
			return false;
		}

        // 4. Prepare streaming wave sources
        m_Source->UninitializeDataSources();
        const auto& graph = AssetManager::GetAsset<SoundGraphAsset>(config->DataSourceAsset);
        m_Source->InitializeDataSources(graph->WaveSources);
		// 5. Update preview player with the new compiled patch player

         // TODO: swap m_Source
        m_Source->SuspendProcessing(true);
        m_Source->ReplacePlayer(source);
        m_Source->SuspendProcessing(false);

        // 6. Initialize player's default parameter values (graph "preset")
            
        if (source)
        {
            m_SoundGraphPreset = graph->GraphInputs;
            const bool parametersSet = m_Source->ApplyParameterPreset(m_SoundGraphPreset);
            ZONG_CORE_ASSERT(parametersSet);
        }

        InitializeEffects(config);

        // TODO: handle using parent's (parent group) spatialization vs override (config probably would be passed down here from parrent)
        const bool isSpatializationEnabled = config->bSpatializationEnabled;

        // Setting base Volume and Pitch
        m_Volume = (double)config->VolumeMultiplier;
        m_Pitch = (double)config->PitchMultiplier;
#if 0
        ma_sound_set_volume(&m_Sound, config->VolumeMultiplier); // TODO: use new SoundGraph interface?
        ma_sound_set_pitch(&m_Sound, config->PitchMultiplier);
#else
        ma_engine_node* engineNode = m_Source->GetEngineNode();
		ma_node_set_output_bus_volume(engineNode, 0, config->VolumeMultiplier);
        // It should be safe to set pitch non-atomically here,
        // because the node should not be playing at this point.
        m_Source->SuspendProcessing(true);
		engineNode->pitch = config->PitchMultiplier;
        m_Source->SuspendProcessing(false);
#endif

        SetLooping(config->bLooping);
        m_Source->SuspendProcessing(false);

        bIsReadyToPlay = true;
        return bIsReadyToPlay;
    }

    void SoundGraphSound::InitializeEffects(const Ref<SoundConfig>& config)
    {
        ma_engine_node* engineNode = m_Source->GetEngineNode();
        auto* engine = engineNode->pEngine;
        auto* currentHeaderNode = &engineNode->baseNode;

        // TODO: implement a proper init-uninit routine in SourceManager

        // High-Pass, Low-Pass filters

        m_LowPass.Initialize(engine, currentHeaderNode);
        currentHeaderNode = m_LowPass.GetNode();

        m_HighPass.Initialize(engine, currentHeaderNode);
        currentHeaderNode = m_HighPass.GetNode();

		// m_LowPass.SetCutoffValue(config->LPFilterValue);
		// m_HighPass.SetCutoffValue(config->HPFilterValue);
		m_LowPass.SetCutoffFrequency(Audio::NormalizedToFrequency(config->LPFilterValue));
		m_HighPass.SetCutoffFrequency(Audio::NormalizedToFrequency(config->HPFilterValue));

        // Reverb send

        ma_result result = MA_SUCCESS;
        uint32_t numChannels = ma_node_get_output_channels(engineNode, 0);
        ma_splitter_node_config splitterConfig = ma_splitter_node_config_init(numChannels);
        result = ma_splitter_node_init(engineNode->baseNode.pNodeGraph,
                                       &splitterConfig,
                                       &engine->pResourceManager->config.allocationCallbacks,
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

    void SoundGraphSound::ReleaseResources()
    {
        // Duck-tape because uninitializing uninitialized na_nodes causes issues
        if (bIsReadyToPlay)
        {
            m_Source->Uninitialize();
            m_Source->UninitializeDataSources();
            // TODO: maybe completely delete m_Source?

            // Need to check if hasn't already been uninitialized by the engine (or has been initialized at all)

            if (m_MasterSplitter.base.pCachedData != NULL && m_MasterSplitter.base._pHeap != NULL)
            {
                ma_allocation_callbacks* allocationCallbacks = &m_Source->GetEngineNode()->pEngine->pResourceManager->config.allocationCallbacks;
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

    bool SoundGraphSound::Play()
    {
        if (!IsReadyToPlay())
            return false;

        LOG_PLAYBACK("Play: " + StringFromState(m_PlayState) + " -> ");

        switch (m_PlayState)
        {
        case EPlayState::Stopped:
            m_NextPlayState = EPlayState::Starting;
            break;
        case EPlayState::Starting:
            ZONG_CORE_ASSERT(false);
            break;
        case EPlayState::Playing:
            StopNow(ResetPlaybackPosition);
            m_NextPlayState = EPlayState::Starting;
            break;
        case EPlayState::Pausing:
            StopNow();
            m_NextPlayState = EPlayState::Starting;
            break;
        case EPlayState::Paused:
            // Prepare fade-in for un-pause
            //ma_sound_set_fade_in_milliseconds(&m_Sound, 0.0f, m_StoredFaderValue, STOPPING_FADE_MS / 2);
            m_NextPlayState = EPlayState::Starting;
            break;
        case EPlayState::Stopping:
            StopNow(NotifyPlaybackComplete | ResetPlaybackPosition);
            m_NextPlayState = EPlayState::Starting;
            break;
        case EPlayState::FadingOut:
            break;
        case EPlayState::FadingIn:
            break;
        default:
            break;
        }

        bFinished = false;
		ZONG_CORE_ASSERT(!m_Source->IsFinished());

        LOG_PLAYBACK("-> ..next state: " + StringFromState(m_NextPlayState));

        return true;

        // TODO: consider marking this sound for stop-fade and switching to new one to prevent click on restart
        //       or some other, lower level solution to fade-out stopping while starting to read from the beginning

    }

    bool SoundGraphSound::Stop()
    {
        bool result = true;
        switch (m_PlayState)
        {
        case EPlayState::Stopped:
            bFinished = true;
            result = false;
            break;
        case EPlayState::Starting:
            StopNow(ResetPlaybackPosition); // consider stop-fading
            m_NextPlayState = EPlayState::Stopping;
            break;
        case EPlayState::Playing:
            result = StopFade(STOPPING_FADE_MS);
            m_NextPlayState = EPlayState::Stopping;
            break;
        case EPlayState::Pausing:
            StopNow(NotifyPlaybackComplete | ResetPlaybackPosition);
            m_NextPlayState = EPlayState::Stopping;
            break;
        case EPlayState::Paused:
            StopNow(NotifyPlaybackComplete | ResetPlaybackPosition);
            m_NextPlayState = EPlayState::Stopping;
            break;
        case EPlayState::Stopping:
            StopNow(NotifyPlaybackComplete | ResetPlaybackPosition);
            break;
        case EPlayState::FadingOut:
        case EPlayState::FadingIn:
            StopNow();
            break;
        default:
            break;
        }
        m_LastFadeOutDuration = (float)STOPPING_FADE_MS / 1000.0f;
        LOG_PLAYBACK(StringFromState(m_PlayState));
        return result;
    }

    bool SoundGraphSound::Pause()
    {
        bool result = true;

        switch (m_PlayState)
        {
        case EPlayState::Playing:
            result = StopFade(STOPPING_FADE_MS);
            m_NextPlayState = EPlayState::Pausing;
            break;
        case EPlayState::FadingOut:
            break;
        case EPlayState::FadingIn:
            break;
        default:
            // If was called to Pause while not playing
            m_NextPlayState = EPlayState::Paused;
            result = true;
            break;
        }
        LOG_PLAYBACK(StringFromState(m_PlayState) + " -> " + StringFromState(m_NextPlayState));

        return result;
    }

    bool SoundGraphSound::StopFade(uint64_t numSamples)
    {
        constexpr double stopFadeTime = (double)STOPPING_FADE_MS * 1.1 / 1000.0;
        m_StopFadeTime = stopFadeTime;

        // Negative volumeBeg starts the fade from the current volume
        //~ma_sound_set_fade_in_pcm_frames(&m_Sound, -1.0f, 0.0f, numSamples);

        // TODO

        return true;
    }

    bool SoundGraphSound::StopFade(int milliseconds)
    {
        ZONG_CORE_ASSERT(milliseconds > 0);

        const uint64_t fadeInFrames = (milliseconds * m_Source->GetEngineNode()->fader.config.sampleRate) / 1000;

        return StopFade(fadeInFrames);
    }

    bool SoundGraphSound::FadeIn(const float duration, const float targetVolume)
    {
        /*if (IsPlaying() || bStopping)
            return false;

        ma_result result;
        result = ma_sound_set_fade_in_milliseconds(&m_Sound, 0.0f, targetVolume, uint64_t(duration * 1000));
        if (result != MA_SUCCESS)
            return false;

        bPaused = false;
        m_StoredFaderValue = targetVolume;*/
        // TODO
        return Play(); //~ma_sound_start(&m_Sound) == MA_SUCCESS;
    }

    bool SoundGraphSound::FadeOut(const float duration, const float targetVolume)
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

    bool SoundGraphSound::IsPlaying() const
    {
        return m_PlayState != EPlayState::Stopped && m_PlayState != EPlayState::Paused;
    }

    bool SoundGraphSound::IsFinished() const
    {
        return bFinished/* && !bPaused*/;
    }

    bool SoundGraphSound::IsStopping() const
    {
        return m_PlayState == EPlayState::Stopping;
    }

    void SoundGraphSound::SetVolume(float newVolume)
    {
        // TODO
        //~ma_sound_set_volume(&m_Sound, m_Volume * (double)newVolume);
    }

    void SoundGraphSound::SetPitch(float newPitch)
    {
        // TODO
        //~ma_sound_set_pitch(&m_Sound, m_Pitch * (double)newPitch);
    }

    void SoundGraphSound::SetLooping(bool looping)
    {
        bLooping = looping;
        // TODO
        //~ma_sound_set_looping(&m_Sound, bLooping);
    }

    float SoundGraphSound::GetVolume()
    {
        // TODO
        //~return ma_node_get_output_bus_volume(&m_Sound, 0);
        return 1.0f;
    }

    float SoundGraphSound::GetPitch()
    {
        // TODO
        //return m_Sound.engineNode.pitch;
        return 1.0f;
    }

    //==============================================================================
    /// Sound Parameter Interface
    void SoundGraphSound::SetParameter(uint32_t pareterID, float value)
    {
        m_Source->SetParameter(pareterID, choc::value::createFloat32(value));
    }

    void SoundGraphSound::SetParameter(uint32_t pareterID, int value)
    {
        m_Source->SetParameter(pareterID, choc::value::createInt32(value));
    }

    void SoundGraphSound::SetParameter(uint32_t pareterID, bool value)
    {
        m_Source->SetParameter(pareterID, choc::value::createBool(value));
    }

    //==============================================================================
    ///
    void SoundGraphSound::SetLocation(const glm::vec3& location, const glm::vec3& orientation)
    {
#ifdef SPATIALIZER_TEST

        ma_sound_set_position(&m_Sound, location.x, location.y, location.z);
#else     
        //if(m_Spatializer.IsInitialized())
           // m_Spatializer.SetPositionRotation(location, orientation);
#endif
        //m_AirAbsorptionFilter.UpdateDistance(glm::distance(location, glm::vec3(0.0f, 0.0f, 0.0f)));
    }

    void SoundGraphSound::SetVelocity(const glm::vec3& velocity /*= { 0.0f, 0.0f, 0.0f }*/)
    {
    }

    void SoundGraphSound::Update(Engine::Timestep ts)
    {
        ZONG_PROFILE_FUNC();

		m_Source->Update(ts);

        Ref<SoundGraph::SoundGraph> graph = m_Source->GetGraph();

        auto notifyIfFinished = [&]
        {
            if (/*ma_sound_at_end(&m_Sound) == MA_TRUE && */onPlaybackComplete)
                onPlaybackComplete();
        };

        auto isNodePlaying = [&]
        {
            if (!graph)
                return false;

            const ma_node_state state = ma_node_get_state(m_Source->GetNode());
            return state == ma_node_state_started;
        };

        auto isFadeFinished = [&]
        {
            return m_StopFadeTime <= 0.0;
        };

        m_StopFadeTime = glm::max(0.0, m_StopFadeTime - (double)ts.GetSeconds());

        m_PlayState = m_NextPlayState;

        switch (m_PlayState)
        {
        case EPlayState::Stopped:
            if (!bFinished)
            {
                bFinished = true;
                notifyIfFinished();
            }
            else
            {
                bFinished = true;
            }
            break;
        case EPlayState::Starting:
            if (graph)
            {

                //if (m_Source->IsSuspended())
                //    m_Source->SuspendProcessing(false);

                m_Source->SuspendProcessing(false);

                if (m_Source->SendPlayEvent())
                {
                    m_NextPlayState = EPlayState::Playing;
                    LOG_PLAYBACK("Upd: " + StringFromState(m_PlayState) + "->" + StringFromState(m_NextPlayState));

                    // Apply SoundGraph preset
                    bool parametersSet = m_Source->ApplyParameterPreset(m_SoundGraphPreset);
                    // Preset must match parameters (input events) of the compiled patch player 
                    ZONG_CORE_ASSERT(parametersSet);
                }
                else
                {
                    ZONG_CORE_ASSERT("Failed to post Play event to SoundGraph instance.");
                }
            }
#if 0
            if (isNodePlaying())
            {
                m_PlayState = EPlayState::Playing;
                LOG_PLAYBACK("Upd: " + StringFromState(m_PlayState));
            }
#endif
            break;
        case EPlayState::Playing:
        {
            //? DBG. Parameter change test
            /*static float time = 0.0f;
            static float volume = 1.0f;

            time += ts;
            const float step = 0.1f;
            if (time >= step)
            {
                time = 0.0f;

                volume = std::fmodf(volume += step, 1.0f);

                if (!m_Source->SetParameter("VolumePar", choc::value::Value(volume)))
                    ZONG_CORE_ERROR("Failed to set VolumePar");
            }*/

            //if (!m_Source->IsAnyDataSourceReading() || m_Source->IsFinished())
			if (m_Source->IsFinished())
            {
                m_NextPlayState = EPlayState::Stopped;
                LOG_PLAYBACK("Upd: " + StringFromState(m_PlayState) + "->" + StringFromState(m_NextPlayState));
            }
            break;
        }
        case EPlayState::Pausing:
            if (isFadeFinished())
            {
                StopNow();
                m_NextPlayState = EPlayState::Paused;
                LOG_PLAYBACK("Upd: " + StringFromState(m_PlayState) + "->" + StringFromState(m_NextPlayState));
            }
            break;
        case EPlayState::Paused:
            break;
        case EPlayState::Stopping:
            if (isFadeFinished())
            {
                StopNow(NotifyPlaybackComplete | ResetPlaybackPosition);
                m_NextPlayState = EPlayState::Stopped;
                LOG_PLAYBACK("Upd: " + StringFromState(m_PlayState) + "->" + StringFromState(m_NextPlayState));
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

    int SoundGraphSound::StopNow(bool notifyPlaybackComplete /*= true*/, bool resetPlaybackPosition /*= true*/)
    {
        // TODO: if !resetPlaybackPosition, just suspend processing?

        if (auto graph = m_Source->GetGraph())
        {
            // TODO: fade out before stopping?
            // TODO: clear buffer
			// TODO: send Stop event?
            m_Source->SuspendProcessing(true);
            if (resetPlaybackPosition)
                graph->Reset();
            m_Source->SuspendProcessing(false); // Probably unnecessary if the graph is stopped?

            // Need to reinitialize parameters after the player has been reset
            bool parametersSet = false;
            if (resetPlaybackPosition)
                parametersSet = m_Source->ApplyParameterPreset(m_SoundGraphPreset);

            // Preset must match parameters (input events) of the compiled patch player 
            ZONG_CORE_ASSERT(parametersSet);

        }

        if (resetPlaybackPosition)
        {
            // Mark this voice to be released.
            bFinished = true;

            ZONG_CORE_ASSERT(m_PlayState != EPlayState::Starting);
            m_NextPlayState = EPlayState::Stopped;
        }

        // Need to notify AudioEngine of completion,
        // if this is one shot, AudioComponent needs to be destroyed.
        if (notifyPlaybackComplete && onPlaybackComplete)
            onPlaybackComplete();

        return 1; //m_SoundSourceID;
    }

    int SoundGraphSound::StopNow(uint16_t options /*= None*/)
    {
        return StopNow(((uint16_t)EStopNowOptions::NotifyPlaybackComplete & options), ((uint16_t)EStopNowOptions::ResetPlaybackPosition & options));
    }

    float SoundGraphSound::GetCurrentFadeVolume()
    {
        // TODO: return volume accounted for distance attenuation, or better read output volume envelope.
        //~float currentVolume = ma_sound_get_current_fade_volume(&m_Sound);

        return 1.0f;
    }

    float SoundGraphSound::GetPriority()
    {
        return GetCurrentFadeVolume() * ((float)m_Priority / 255.0f);
    }

    float SoundGraphSound::GetPlaybackPercentage()
    {
        //! SoundGraph doesn't have a notion of playback progress.
        //  Return 0 or 1 if finished
        return m_Source->AreAllDataSourcesAtEnd() ? 1.0f : 0.0f;

        //~ma_uint64 currentFrame;
        //~ma_uint64 totalFrames;
        //~ma_sound_get_cursor_in_pcm_frames(&m_Sound, &currentFrame);

        // TODO: concider storing this value when initializing sound;
        //~ma_sound_get_length_in_pcm_frames(&m_Sound, &totalFrames);


        //~return (float)currentFrame / (float)totalFrames;
    }

} // namespace Engine

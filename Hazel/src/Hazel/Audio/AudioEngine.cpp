#include "hzpch.h"
#include "AudioEngine.h"

#include "AudioEvents/AudioCommandRegistry.h"
#include "AudioEventsManager.h"
#include "ResourceManager.h"
#include "SourceManager.h"
#include "VFS.h"

#include "DSP/Reverb/Reverb.h"
#include "DSP/Spatializer/Spatializer.h"

#include "Hazel/Core/Application.h"
#include "Hazel/Core/Timer.h"
#include "Hazel/Debug/Profiler.h"

#include <choc/containers/choc_Span.h>
#include <choc/containers/choc_SmallVector.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <execution>
#include <format>

//#define LOG_MEMORY

// Enable logging and debugging for Voice handling
//#define DBG_VOICES

#ifdef DBG_VOICES // Enable logging of Voice handling
#define LOG_VOICES(...) HZ_CORE_INFO_TAG("Audio", __VA_ARGS__)
#else
#define LOG_VOICES(...)
#endif

#if 0 // Enable logging for Events handling
#define LOG_EVENTS(...) HZ_CORE_INFO_TAG("Audio", __VA_ARGS__)
#else
#define LOG_EVENTS(...)
#endif

namespace Hazel {
	using namespace Audio;

	MiniAudioEngine* MiniAudioEngine::s_Instance = nullptr;

	Audio::Stats MiniAudioEngine::s_Stats;


	//==========================================================================
	void MiniAudioEngine::ExecuteOnAudioThread(AudioThreadCallbackFunction func, const char* jobID/* = "NONE"*/)
	{
		AudioThread::AddTask(hnew AudioFunctionCallback(std::move(func), jobID));
	}

	void MiniAudioEngine::ExecuteOnAudioThread(EIfThisIsAudioThread policy, Audio::AudioThreadCallbackFunction func, const char* jobID)
	{
		if (IsAudioThread())
		{
			switch (policy)
			{
				default:
				case Hazel::MiniAudioEngine::ExecuteAsync: AudioThread::AddTask(hnew AudioFunctionCallback(std::move(func), jobID)); break;
				case Hazel::MiniAudioEngine::ExecuteNow: func(); break;
			}
		}
		else
		{
			AudioThread::AddTask(hnew AudioFunctionCallback(std::move(func), jobID));
		}
	}
	
	//==========================================================================
	// TODO: JP. wrap this into a proper sufix allocator
	void MemFreeCallback(void* p, void* pUserData)
	{
		if (p == NULL)
			return;

		constexpr auto offset = std::max(sizeof(int), alignof(max_align_t));

		char* buffer = (char*)p - offset; //get the start of the buffer
		int* sizeBox = (int*)buffer;

		auto* alData = (AllocationCallbackData*)pUserData;
		{
			std::scoped_lock lock{ alData->Stats.mutex }; // TODO: JP. remove this mutex
			if (alData->isResourceManager) alData->Stats.MemResManager -= *sizeBox;
			else                           alData->Stats.MemEngine -= *sizeBox;
		}

#ifdef LOG_MEMORY
		HZ_CORE_INFO_TAG("Audio", "Freed mem KB : {0}", *sizeBox / 1000.0f);
#endif
		ma_free(buffer, nullptr);
	}
	void* MemAllocCallback(size_t sz, void* pUserData)
	{
		constexpr auto offset = std::max(sizeof(int), alignof(max_align_t));

		char* buffer = (char*)ma_malloc(sz + offset, nullptr); //allocate offset extra bytes 
		if (buffer == NULL)
			return NULL; // no memory! 

		auto* alData = (AllocationCallbackData*)pUserData;
		{
			std::scoped_lock lock{ alData->Stats.mutex }; // TODO: JP. remove this mutex
			if (alData->isResourceManager) alData->Stats.MemResManager += sz;
			else                           alData->Stats.MemEngine += sz;
		}


		int* sizeBox = (int*)buffer;
		*sizeBox = (int)sz; //store the size in first sizeof(int) bytes!

#ifdef LOG_MEMORY
		HZ_CORE_INFO_TAG("Audio", "Allocated mem KB : {0}", *sizeBox / 1000.0f);
#endif
		return buffer + offset; //return buffer after offset bytes!
	}
	void* MemReallocCallback(void* p, size_t sz, void* pUserData)
	{
		if (p == nullptr)
			return MemAllocCallback(sz, pUserData);

		constexpr auto offset = std::max(sizeof(int), alignof(max_align_t));

		auto* buffer = (char*)p - offset; //get the start of the buffer
		int* sizeBox = (int*)buffer;

		auto* alData = (AllocationCallbackData*)pUserData;
		{
			std::scoped_lock lock{ alData->Stats.mutex }; // TODO: JP. remove this mutex
			if (alData->isResourceManager) alData->Stats.MemResManager += sz - *sizeBox;
			else                           alData->Stats.MemEngine += sz - *sizeBox;
		}

		*sizeBox = (int)sz;

		auto* newBuffer = (char*)ma_realloc(buffer, sz, NULL);
		if (newBuffer == NULL)
			return NULL;

#ifdef LOG_MEMORY
		HZ_CORE_INFO_TAG("Audio", "Reallocated mem KB : {0}", sz / 1000.0f);
#endif
		return newBuffer + offset;
	}

	void MALogCallback(void* pUserData, ma_uint32 level, const char* pMessage)
	{
		std::string message = std::format("{0}: {1}", std::string(ma_log_level_to_string(level)), pMessage);
		message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());

		switch (level)
		{
			case MA_LOG_LEVEL_INFO:
				HZ_CORE_INFO_TAG("miniaudio", message);
				break;
			case MA_LOG_LEVEL_WARNING:
				HZ_CORE_WARN_TAG("miniaudio", message);
				break;
			case MA_LOG_LEVEL_ERROR:
				HZ_CORE_ERROR_TAG("miniaudio", message);
				break;
			default:
				HZ_CORE_TRACE_TAG("miniaudio", message);
		}
	}

	//==========================================================================
	MiniAudioEngine::MiniAudioEngine()
	{
		m_ResourceManager = CreateScope<Audio::ResourceManager>(*this);

		auto* vfs = new Audio::VFS();
		vfs->onCreateReader = [&](const char* filepath) { return m_ResourceManager->CreateReaderFor(filepath); };
		vfs->onGetFileSize = [&](const char* filepath) { return m_ResourceManager->GetFileSize(filepath); };
		m_ResourceManagerVFS = Scope<Audio::VFS>(vfs);

		m_SourceManager = CreateScope<SourceManager>(*this, *m_ResourceManager.get());
		m_EventsManager.reset(new Audio::AudioEventsManager(*this));
		
		m_OnSourceFinished.Bind<&Audio::AudioEventsManager::OnSourceFinished>(m_EventsManager.get());

		// If this EventID doesn't have any more Sources playing, remove SourceID from EventsRegistry
		static auto handleFinishedEvent = [this](Audio::EventID finishedEvent, UUID objectID)
		{
			auto& eventsOnObject = m_EventHandles.at(objectID);
			Utils::Remove(eventsOnObject, EventHandle{ finishedEvent });
			if (eventsOnObject.empty())
				m_EventHandles.erase(objectID);

			m_UpdateState.Set(UpdateState::Stats_ActiveEvents);
		};

		m_EventsManager->m_OnEventFinished.BindLambda(handleFinishedEvent);

		// Assign Action Handler to Events Manager
		{
			Audio::AudioEventsManager::ActionHandler actionHandler;
			
			actionHandler.StartPlayback			.Bind<&MiniAudioEngine::Action_StartPlayback>(this);

			actionHandler.PauseVoicesOnAnObject	.Bind<&MiniAudioEngine::Action_PauseVoicesOnAnObject>(this);
			actionHandler.PauseVoices			.Bind<&MiniAudioEngine::Action_PauseVoices>(this);

			actionHandler.ResumeVoicesOnAnObject.Bind<&MiniAudioEngine::Action_ResumeVoicesOnAnObject>(this);
			actionHandler.ResumeVoices			.Bind<&MiniAudioEngine::Action_ResumeVoices>(this);

			actionHandler.StopVoicesOnAnObject	.Bind<&MiniAudioEngine::Action_StopVoicesOnAnObject>(this);
			actionHandler.StopVoices			.Bind<&MiniAudioEngine::Action_StopVoices>(this);

			actionHandler.StopAllOnObject		.Bind<&MiniAudioEngine::Action_StopAllOnObject>(this);
			actionHandler.StopAll				.Bind<&MiniAudioEngine::Action_StopAll>(this);
			
			actionHandler.PauseAllOnObject		.Bind<&MiniAudioEngine::Action_PauseAllOnObject>(this);
			actionHandler.PauseAll				.Bind<&MiniAudioEngine::Action_PauseAll>(this);

			actionHandler.ResumeAllOnObject		.Bind<&MiniAudioEngine::Action_ResumeAllOnObject>(this);
			actionHandler.ResumeAll				.Bind<&MiniAudioEngine::Action_ResumeAll>(this);

			actionHandler.ExecuteOnSources		.Bind<&MiniAudioEngine::Action_ExecuteOnSources>(this);

			m_EventsManager->AssignActionHandler(std::move(actionHandler));
		}


		AudioThread::BindUpdateFunction<&MiniAudioEngine::Update>(this);
		AudioThread::Start();

		MiniAudioEngine::ExecuteOnAudioThread([this] { Initialize(); }, "InitializeAudioEngine");
	}

	MiniAudioEngine::~MiniAudioEngine()
	{
		if (bInitialized)
			Uninitialize();
	}

	void MiniAudioEngine::Init()
	{
		HZ_CORE_ASSERT(s_Instance == nullptr, "Audio Engine already initialized.");
		MiniAudioEngine::s_Instance = hnew MiniAudioEngine();
	}

	void MiniAudioEngine::Shutdown()
	{
		HZ_CORE_ASSERT(s_Instance, "Audio Engine was not initialized.");
		s_Instance->Uninitialize();
		delete s_Instance;
		s_Instance = nullptr;
	}

	bool MiniAudioEngine::Initialize()
	{
		if (bInitialized)
			return true;

		HZ_PROFILE_FUNC();

		ma_result result;
		ma_engine_config engineConfig = ma_engine_config_init();
		// TODO: for now splitter node and custom node don't work toghether if custom periodSizeInFrames is set
		//engineConfig.periodSizeInFrames = PCM_FRAME_CHUNK_SIZE;

#ifdef HZ_PLATFORM_WINDOWS
        // TODO(Emily): These callbacks cause heap corruption
		ma_allocation_callbacks allocationCallbacks{ &m_EngineCallbackData, &MemAllocCallback, &MemReallocCallback, &MemFreeCallback };
#else
        ma_allocation_callbacks allocationCallbacks{
            nullptr,
            [](size_t size, void*) -> void* { return malloc(size); },
            [](void* ptr, size_t size, void*) -> void* { return realloc(ptr, size); },
            [](void* ptr, void*) -> void { free(ptr); },
        };
#endif

		result = ma_log_init(&allocationCallbacks, &m_maLog);
		HZ_CORE_ASSERT(result == MA_SUCCESS, "Failed to initialize miniaudio logger.");
		ma_log_callback logCallback = ma_log_callback_init(&MALogCallback, nullptr);
		result = ma_log_register_callback(&m_maLog, logCallback);
		HZ_CORE_ASSERT(result == MA_SUCCESS, "Failed to register miniaudio log callback.");

		engineConfig.allocationCallbacks = allocationCallbacks;
		engineConfig.pLog = &m_maLog;

		//? For now forcing stereo endpoint configuration
		// TODO: handle surround endpoint device configuration when implement mixing
		engineConfig.channels = 2;

		// Initialize Resource Manager VFS
		result = Audio::ma_custom_vfs_init(m_ResourceManagerVFS.get(), &allocationCallbacks);
		if (result != MA_SUCCESS)
		{
			HZ_CORE_ASSERT(false, "Failed to initialize Resource Manager VFS.");
			return false;
		}

		engineConfig.pResourceManagerVFS = m_ResourceManagerVFS.get();

		result = ma_engine_init(&engineConfig, &m_Engine);
		if (result != MA_SUCCESS)
		{
			HZ_CORE_ASSERT(false, "Failed to initialize audio engine.");
			return false;
		}


		allocationCallbacks.pUserData = &m_RMCallbackData;
		m_Engine.pResourceManager->config.allocationCallbacks = allocationCallbacks;

		m_SourceManager->Initialize();
		m_MasterReverb = CreateScope<DSP::Reverb>();
		m_MasterReverb->Initialize(&m_Engine, &m_Engine.nodeGraph.endpoint);

		// miniaudio doesn't initialize values and we need this to check
		// if the sound needs to be uninitialized before reusing it
		m_PreviewSound.pDataSource = nullptr;

		// TODO: get max number of sources from platform interface, or user settings
		m_NumSources = 32;
		CreateSources();

		HZ_CORE_INFO_TAG("Audio", R"(Audio Engine: engine initialized.
                    -----------------------------
                    Endpoint Device Name:   {0}
                    Sample Rate:            {1}
                    Channels:               {2}
                    -----------------------------
                    Callback Buffer Size:   {3}
                    Number of Sources:      {4}
                    -----------------------------)",
					m_Engine.pDevice->playback.name,
					m_Engine.pDevice->sampleRate,
					m_Engine.pDevice->playback.channels,
					engineConfig.periodSizeInFrames,
					m_NumSources);

		{
			std::scoped_lock lock{ s_Stats.mutex };
			s_Stats.TotalSources = m_NumSources;
		}

		bInitialized = true;
		return true;
	}

	bool MiniAudioEngine::Uninitialize()
	{
		StopAll(true);

		// Wait for SopAll to be processed
		AudioThreadFence fence;
		fence.BeginAndWait();

		m_SceneContext = nullptr;

		AudioThread::Stop();

		m_SourceManager->UninitializeEffects();
		m_ResourceManager->ReleaseResources();
		m_MasterReverb.reset();

		ma_engine_uninit(&m_Engine);

		for (auto& s : m_SoundSources)
			delete s;

		m_SoundSources.clear();

		m_VoiceData.clear();
		m_VoiceHandles.clear();
		m_EventHandles.clear();
		m_ObjectData.clear();

		bInitialized = false;

		HZ_CORE_INFO_TAG("Audio", "Audio Engine: engine un-initialized.");

		return true;
	}

	void MiniAudioEngine::OnProjectLoaded()
	{
		StopAll(true);
		MiniAudioEngine::ExecuteOnAudioThread([this] { m_ResourceManager->Initialize(); }, "Initialize ResourceManager");

		AudioThreadFence resourceManagerInitFence;
		resourceManagerInitFence.BeginAndWait();

		if (!m_ResourceManager->GetSoundBank())
		{
			HZ_CORE_WARN_TAG("Audio", "Could not load SoundBank (this is not an error if you have no audio)");
		}

		// TODO: maybe reinitialize the whole engine?
	}


	//==============================================================================
	template<typename Function>
	void MiniAudioEngine::InvokeOnActiveSounds(uint64_t entityID, Function functionToInvoke)
	{
		for (auto& sound : m_ActiveSounds)
		{
			if (sound->m_AudioObjectID == entityID)
				functionToInvoke(sound);
		}
	}

	template<typename Function>
	void MiniAudioEngine::InvokeOnActiveSources(Audio::EventID playingEvent, Function functionToInvoke)
	{
		if (m_EventsManager->GetNumberOfActiveSources(playingEvent) <= 0)
		{
			// This might be unnecesary, but for the time being is useful for debugging.
			HZ_CONSOLE_LOG_WARN("Audio. Trying to set parameter for an event that's not in the active events registry.");
			return;
		}

		for (SourceID sourceID : m_EventsManager->GetActiveSources(playingEvent))
			functionToInvoke(m_SoundSources.at(sourceID));
	}

	//==============================================================================
	void MiniAudioEngine::CreateSources()
	{
		HZ_PROFILE_FUNC();

		m_SoundSources.reserve(m_NumSources);
		for (int i = 0; i < m_NumSources; ++i)
		{
			SoundObject* soundSource = new SoundObject();
			soundSource->m_SoundSourceID = i;
			m_SoundSources.push_back(soundSource);
			m_SourceManager->m_FreeSourcIDs.push(i);
		}

		// TODO: make this (and sources/voices in general) more dynamic (i.e. user setting)
		m_VoiceData.resize(m_NumSources);
	}

	SoundObject* MiniAudioEngine::FreeLowestPrioritySource()
	{
		HZ_PROFILE_FUNC();

		HZ_CORE_ASSERT(m_SourceManager->m_FreeSourcIDs.empty());

		SoundObject* lowestPriStoppingSource = nullptr;
		SoundObject* lowestPriNonLoopingSource = nullptr;
		SoundObject* lowestPriSource = nullptr;

		// Compare and return lower priority Source of the two
		auto getLowerPriority = [](SoundObject* sourceToCheck, SoundObject* sourceLowest, bool checkPlaybackPos = false) -> SoundObject*
		{
			if (!sourceLowest)
			{
				return sourceToCheck;
			}
			else
			{
				float a = sourceToCheck->GetPriority();
				float b = sourceLowest->GetPriority();

					 if (a < b)             return sourceToCheck;
				else if (a > b)             return sourceLowest;
				else if (checkPlaybackPos)  return sourceToCheck->GetPlaybackPercentage() > sourceLowest->GetPlaybackPercentage() ? sourceToCheck : sourceLowest;
				else                        return sourceLowest;
			}
		};

		// Run through all of the active sources and find the lowest priority source that can be stopped
		for (SoundObject* source : m_ActiveSounds)
		{
			/* TODO
				make sure only checking Sounds (Voices) (which could be the lowest level SoundWave),
				and handle, in some way, complex SoundObject graphs that may contain multiple Waves.
				Should probably keep track of Active Sources and Active Sounds separatelly (e.i. implement ActiveSound abstraction)
			*/
			if (source->IsStopping())
			{
				lowestPriStoppingSource = getLowerPriority(source, lowestPriStoppingSource);
			}
			else
			{
				if (!source->IsLooping())
				{
					// Checking playback percentage here, in case the volume weighted prioprity is the same
					lowestPriNonLoopingSource = getLowerPriority(source, lowestPriNonLoopingSource, true);
				}
				else
				{
					lowestPriSource = getLowerPriority(source, lowestPriSource);
				}
			}
		}

		SoundObject* releasedSoundSource = nullptr;

		if (lowestPriStoppingSource)   releasedSoundSource = lowestPriNonLoopingSource;
		else if (lowestPriNonLoopingSource) releasedSoundSource = lowestPriNonLoopingSource;
		else                                releasedSoundSource = lowestPriSource;

		HZ_CORE_ASSERT(releasedSoundSource);

		releasedSoundSource->StopNow();

		// Need to make sure we remove released source from "starting sounds"
		// in case user tries to play more than max number of sounds in one call
		const auto startingSound = std::find_if(m_SoundsToStart.begin(), m_SoundsToStart.end(), [releasedSoundSource](const SoundObject* sound)
		{ return sound == releasedSoundSource; });

		if (startingSound != m_SoundsToStart.end())
		{
			m_SoundsToStart.erase(startingSound);

			LOG_VOICES("Voice released, ID {0}", releasedSoundSource->m_SoundSourceID);
		}

		ReleaseFinishedSources();

#if DBG_VOICES
		HZ_CORE_ASSERT(releasedSoundSource->m_PlayState == Sound::ESoundPlayState::Stopped);
#endif
		HZ_CORE_ASSERT(!m_SourceManager->m_FreeSourcIDs.empty());

		/* Other possible way:
		   1. check if any of the sources already Stopping
		   2. check the lowest priority
		   3. check lowes volume
		   4. check the farthest distance from the listener
		   5. check playback time left
		*/

		return releasedSoundSource;
	}

	bool MiniAudioEngine::SubmitStartPlayback(uint64_t audioEntityID)
	{
		HZ_PROFILE_FUNC();
		
		//! Game Thread

		Entity entity = m_SceneContext->TryGetEntityWithUUID(audioEntityID);
		if (entity != Entity{})
		{
			const auto& audioComponent = entity.GetComponent<AudioComponent>();
			return PostTrigger(audioComponent.StartCommandID, audioEntityID);
		}
		else
		{
			HZ_CORE_ERROR_TAG("Audio", "AudioEntity with ID {0} doesn't exist!", audioEntityID);
			return false;
		}
	}

	bool MiniAudioEngine::StopActiveSoundSource(uint64_t entityID)
	{
		auto stopSound = [this, entityID]
		{
			// Stop Active Sound Sources associated to the Entity

			for (const auto& voice : m_VoiceHandles)
			{
				if (voice.OwningEntity == entityID)
					m_SoundSources[voice.ID]->Stop();
			}
		};

		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, stopSound, "StopSound");

		return true;
	}

	bool MiniAudioEngine::PauseActiveSoundSource(uint64_t entityID)
	{
		auto pauseSound = [this, entityID]
		{
			// Pause Active Sound Sources associated to the Entity
			for (const auto& voice : m_VoiceHandles)
			{
				if (voice.OwningEntity == entityID)
					m_SoundSources[voice.ID]->Pause();
			}
		};

		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, pauseSound, "PauseSound");

		return true;
	}

	bool MiniAudioEngine::ResumeActiveSoundSource(uint64_t entityID)
	{
		auto resumeSound = [this, entityID]
		{
			// Resume Active Sound Sources associated to the Entity
			for (const auto& voice : m_VoiceHandles)
			{
				if (voice.OwningEntity == entityID)
				{
					SoundObject* sound = m_SoundSources[voice.ID];

					// Only re-start sound if it wasn explicitly paused
					if (sound->GetPlayState() == ESoundPlayState::Paused)
						sound->Play();
				}
			}
		};

		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, resumeSound, "PauseSound");

		return true;
	}

	bool MiniAudioEngine::StopEventID(EventID playingEvent)
	{
		return m_EventsManager->ExecuteActionOnPlayingEvent(playingEvent, EActionTypeOnPlayingEvent::Stop);
	}

	bool MiniAudioEngine::PauseEventID(EventID playingEvent)
	{
		return m_EventsManager->ExecuteActionOnPlayingEvent(playingEvent, EActionTypeOnPlayingEvent::Pause);
	}

	bool MiniAudioEngine::ResumeEventID(EventID playingEvent)
	{
		// TODO: target and context
#if 0
		TriggerAction action;
		action.Type = EActionType::Resume;
		action.Target = nullptr; // All targets on this events
		action.Context = EActionContext::GameObject; // Ignored, using context of the playingEvent actions
#endif
		return m_EventsManager->ExecuteActionOnPlayingEvent(playingEvent, EActionTypeOnPlayingEvent::Resume);
	}

	bool MiniAudioEngine::HasActiveEvents(uint64_t entityID)
	{
		// TODO: JP. this is not thread safe

		for (const auto& voice : m_VoiceHandles)
		{
			if (voice.OwningEntity == entityID)
				return true;
		}

		return m_EventHandles.count(entityID);
	}

	//==================================================================================
	int MiniAudioEngine::Action_StartPlayback(UUID objectID, EventID playingEvent, Ref<SoundConfig> target)
	{
		// Submit voice for playback

		//HZ_CORE_ASSERT(m_ActiveSounds.size() < m_NumSources && m_SoundsToStart.size() < m_NumSources);

		int sourceID = Audio::INVALID_SOURCE_ID;

		if (auto* sound = InitiateNewVoice(objectID, playingEvent, target))
		{
			// Spawn location is set from the Game Thread,
			// likely before we process SoundsToStart
			m_SoundsToStart.push_back(sound);
			
			sourceID = sound->m_SoundSourceID;
		}

		return sourceID;
	}

	bool MiniAudioEngine::Action_PauseVoicesOnAnObject(UUID entityID, Ref<SoundConfig> target)
	{
		// Pause Voice of target Sound Config on Entity

		bool handled = false;

		for (auto* sound : m_ActiveSounds)
		{
			if (sound->m_AudioObjectID == entityID && sound->m_SoundConfig == target)
			{
				// If Play was called on this sound withing this Event (or within this Update),
				// need to remove it from Starting Sounds
				Utils::Remove(m_SoundsToStart, sound);

				sound->Pause();

				// at least one source matching the request has been paused
				handled = true;
			}
		}

		return handled;
	}

	void MiniAudioEngine::Action_PauseVoices(Ref<SoundConfig> target)
	{
		// Pause Voices of target Sound Config on all Entities

		for (auto* sound : m_ActiveSounds)
		{
			if (sound->m_SoundConfig == target)
			{
				// If Play was called on this sound withing this Event (or within this Update),
				// need to remove it from Starting Sounds
				Utils::Remove(m_SoundsToStart, sound);

				sound->Pause();
			}
		}
	}

	bool MiniAudioEngine::Action_ResumeVoicesOnAnObject(UUID entityID, Ref<SoundConfig> target)
	{
		// Resume Voices of target Sound Config on Entity

		bool handled = true;

		for (auto* sound : m_ActiveSounds)
		{
			if (sound->m_AudioObjectID == entityID && sound->m_SoundConfig == target)
			{
				// Making sure we only re-start sound if it wasn explicitly paused
				if (sound->GetPlayState() == ESoundPlayState::Paused)
				{
					sound->Play();

					handled &= true;
				}
				else if (sound->GetPlayState() == ESoundPlayState::Pausing)
				{
					// If the sound is Pausing, delay the Resume Action and let its stop-fade to finish
					handled = false;
				}
			}
		}

		return handled;
	}

	bool MiniAudioEngine::Action_ResumeVoices(Ref<SoundConfig> target)
	{
		// Resume all paused Voices initiated by the target Sound Config
		// TODO: JP. what if target not specified, should it resume all?

		bool handled = false;
			
		handled = true;

		for (auto* sound : m_ActiveSounds)
		{
			if (sound->m_SoundConfig == target)
			{
				// Making sure we only re-start sound if it wasn explicitly paused
				if (sound->GetPlayState() == ESoundPlayState::Paused)
				{
					sound->Play();
				}
				else if (sound->GetPlayState() == ESoundPlayState::Pausing)
				{
					handled = false;
				}
			}
		}

		return handled;
	}

	bool MiniAudioEngine::Action_StopVoicesOnAnObject(UUID entityID, Ref<SoundConfig> target)
	{
		// Stop Voices of target Sound Config on Entity

		bool handled = false;

		for (auto* sound : m_ActiveSounds)
		{
			if (sound->m_AudioObjectID == entityID && sound->m_SoundConfig == target)
			{
				// If Play was called on this sound withing this Event (or within this Update),
				// need to remove it from Starting Sounds
				Utils::Remove(m_SoundsToStart, sound);

				// Still calling Stop to flag it "Finished" and free resources / put it back in pool
				sound->Stop();
		
				handled = true;
			}
		}
		
		return handled;
	}

	void MiniAudioEngine::Action_StopVoices(Ref<SoundConfig> target)
	{
		// Stop all Voices of target Sound Config on all Entities
		// TODO: JP. what if target not specified, should it stop all?

		for (auto* sound : m_ActiveSounds)
		{
			if (sound->m_SoundConfig == target)
			{
				// If Play was called on this sound withing this Event (or within this Update),
				// need to remove it from Starting Sounds
				Utils::Remove(m_SoundsToStart, sound);

				// Still calling Stop to flag it "Finished" and free resources / put it back in pool
				sound->Stop();
			}
		}
	}

	void MiniAudioEngine::Action_StopAllOnObject(UUID entityID)
	{
		// Stop all voices of any target Sound Config on Entities

		for (auto* sound : m_ActiveSounds)
		{
			if (sound->m_AudioObjectID == entityID)
			{
				Utils::Remove(m_SoundsToStart, sound);

				// Still calling Stop to flag it "Finished" and free resources / put it back in pool
				sound->Stop();
			}
		}
	}

	void MiniAudioEngine::Action_StopAll()
	{
		// Stop all voices of any target Sound Config on all Entities

		m_SoundsToStart.clear();
		for (auto* sound : m_ActiveSounds)
		{
			sound->Stop();
		}
	}

	void MiniAudioEngine::Action_PauseAllOnObject(UUID entityID)
	{
		// Pause all voices of any target Sound Config on Entities

		for (auto* sound : m_ActiveSounds)
		{
			if (sound->m_AudioObjectID == entityID)
			{
				// If Play was called on this sound withing this Event (or within this Update),
				// need to remove it from Starting Sounds
				Utils::Remove(m_SoundsToStart, sound);

				sound->Pause();
			}
		}
	}

	void MiniAudioEngine::Action_PauseAll()
	{
		// Pause all voices of any target Sound Config on all Entities

		m_SoundsToStart.clear();
		for (auto* sound : m_ActiveSounds)
		{
			sound->Pause();
		}
	}

	bool MiniAudioEngine::Action_ResumeAllOnObject(UUID entityID)
	{
		// Resume all voices of any target Sound Config on Entities

		bool handled = true;

		for (auto* sound : m_ActiveSounds)
		{
			if (sound->m_AudioObjectID == entityID)
			{
				// Making sure we only re-start sound if it wasn explicitly paused
				if (sound->GetPlayState() == ESoundPlayState::Paused)
				{
					sound->Play();
					
					handled &= true;
				}
				else if (sound->GetPlayState() == ESoundPlayState::Pausing)
				{
					handled = false;
				}
			}
		}

		return handled;
	}

	bool MiniAudioEngine::Action_ResumeAll()
	{
		// Resume all voices of any target Sound Config on all Entities

		bool handled = true;

		for (auto* sound : m_ActiveSounds)
		{
			// Making sure we only re-start sound if it wasn explicitly paused
			if (sound->GetPlayState() == ESoundPlayState::Paused)
			{
				sound->Play();
			}
			else if (sound->GetPlayState() == ESoundPlayState::Pausing)
			{
				handled = false;
			}
		}

		return handled;
	}

	void MiniAudioEngine::Action_ExecuteOnSources(EActionTypeOnPlayingEvent action, const std::vector<SourceID>& sources)
	{
		auto forEachSource = [&](auto execute)
		{
			for (SourceID sourceID : sources)
				execute(m_SoundSources[sourceID]);
		};

		auto stop = [](SoundObject* sound) { sound->Stop(); };
		auto pause = [](SoundObject* sound) { sound->Pause(); };
		auto resume = [](SoundObject* sound)
		{
			// Only re-start sound if it wasn explicitly paused
			if (sound->GetPlayState() == ESoundPlayState::Paused)
				sound->Play();
		};

		// TODO
		auto break_ = [](SoundObject* sound) { HZ_CORE_ASSERT(false); };
		auto releaseEnvelope = [](SoundObject* sound) { HZ_CORE_ASSERT(false); };

		switch (action)
		{
			case EActionTypeOnPlayingEvent::Stop:				forEachSource(stop); break;
			case EActionTypeOnPlayingEvent::Pause:				forEachSource(pause); break;
			case EActionTypeOnPlayingEvent::Resume:				forEachSource(resume); break;
			case EActionTypeOnPlayingEvent::Break:				forEachSource(break_); break;
			case EActionTypeOnPlayingEvent::ReleaseEnvelope:	forEachSource(releaseEnvelope); break;
			default:
				break;
		}
	}


	//==================================================================================

	// TODO: move this to SourceManager
	void MiniAudioEngine::UpdateSources()
	{
		HZ_PROFILE_FUNC();

		/*
			For now we update/store objects positions even if spatialziation is disabled for them.
			Spatialization is not updated because such objects are not registered with spatializer.
			We may or may not want to change this in the future.
		*/

		const auto numberOfObjectsTracked = m_ObjectData.size();
		if (flag_SourcesUpdated.CheckAndResetIfDirty())
		{
			m_ObjectData.clear();

			// Bulk update all of the sources
			std::scoped_lock lock{ m_UpdateSourcesLock };

			// 1. Update positioning of the AudioObjects.
			
			for (const auto& data : m_SourceUpdateData)
			{
				m_ObjectData[data.entityID] = { data.Transform, data.Velocity };
			}

			// 2. Update Volume and Pitch (and other dynamic properties in the future)
			
			// TODO: maybe we could somehow remove this intermediate step and set properties directly on sources?
			for (const auto& data : m_SourceUpdateData)
			{
				VoiceInterface::SetPropertyOnVoices({ data.entityID }, &VoiceData::Volume, data.VolumeMultiplier);
				VoiceInterface::SetPropertyOnVoices({ data.entityID }, &VoiceData::Pitch, data.PitchMultiplier);
			}

			m_SourceUpdateData.clear();
		}
		else
		{
			return;
		}

		if (m_ObjectData.size() != numberOfObjectsTracked)
			m_UpdateState.Set(UpdateState::Stats_AudioObjects);

		if (m_ObjectData.empty())
			return;

		// Note: JP. We update data for SoundsToStart sources as well as already ActiveSounds
		//HZ_CORE_ASSERT(m_ActiveSounds.size() + m_SoundsToStart.size() == m_VoiceHandles.size());

		for (const auto& voice : m_VoiceHandles)
		{
			// A bit icky to access global pool like this, but should be okay
			auto* source = m_SoundSources[voice.ID]; 
			source->SetVolume(m_VoiceData[voice.ID].Volume);
			source->SetPitch(m_VoiceData[voice.ID].Pitch);
		}

		// 3. Update position of the sound sources from associated AudioObjects, which can originate from AudioComponent or not.

		{
			HZ_PROFILE_SCOPE_DYNAMIC("MiniAudioEngine::UpdateSources - USP Loop");

			// NOTE: LLVM libc++ does not support execution policies without a
			// manually applied patch.
			std::for_each(
#ifndef _LIBCPP_VERSION
				std::execution::par,
#endif
				m_VoiceHandles.begin(), m_VoiceHandles.end(), [&](const VoiceHandle& voice)
			{
				// It is valid to not yet have any data from Game Thread when the voice is already initialized
				const auto dIt = m_ObjectData.find(voice.OwningEntity);
				if (dIt == m_ObjectData.end())
					return;

				const AudioObjectData& data = dIt->second;

				m_SourceManager->m_Spatializer->UpdateSourcePosition(voice.ID, data.Transform, data.Velocity);
			});
		}
	}

	void MiniAudioEngine::SubmitSourceUpdateData(std::vector<SoundSourceUpdateData> updateData)
	{
		{
			std::scoped_lock lock{ m_UpdateSourcesLock };
			m_SourceUpdateData.swap(updateData);
		}
		flag_SourcesUpdated.SetDirty();
	}

	std::unordered_set<UUID> MiniAudioEngine::GetInactiveEntities()
	{
		std::scoped_lock lock{ m_InvalidEntitiesLock };
		std::unordered_set entities = m_InactiveEntities;
		m_InactiveEntities.clear();
		return entities;
	}
	
	
	//==================================================================================
	// TODO: JP. consolidate all Listener functionality into an interface

	void MiniAudioEngine::UpdateListener()
	{
		if (m_AudioListener.HasChanged(true))
		{
			Audio::Transform transform = m_AudioListener.GetPositionDirection();
			glm::vec3 vel;
			m_AudioListener.GetVelocity(vel);
			ma_engine_listener_set_position(&m_Engine, 0, transform.Position.x, transform.Position.y, transform.Position.z);
			ma_engine_listener_set_direction(&m_Engine, 0, transform.Orientation.x, transform.Orientation.y, transform.Orientation.z);
			ma_engine_listener_set_world_up(&m_Engine, 0, transform.Up.x, transform.Up.y, transform.Up.z);
			ma_engine_listener_set_velocity(&m_Engine, 0, vel.x, vel.y, vel.z);

			auto [innerAngle, outerAngle] = m_AudioListener.GetConeInnerOuterAngles();
			float outerGain = m_AudioListener.GetConeOuterGain();
			ma_engine_listener_set_cone(&m_Engine, 0, innerAngle, outerAngle, outerGain); //? this is not thread safe internally


			// Need to update all Spatializer sources when listener position changes
			m_SourceManager->m_Spatializer->UpdateListener(transform, vel);
		}
	}

	void MiniAudioEngine::UpdateListenerPosition(const Audio::Transform& newTransform)
	{
		if (m_AudioListener.PositionNeedsUpdate(newTransform))
			m_AudioListener.SetNewPositionDirection(newTransform);
	}

	void MiniAudioEngine::UpdateListenerVelocity(const glm::vec3& newVelocity)
	{
		auto isVelocityValid = [](const glm::vec3& velocity) {
			auto inRange = [](auto x, auto low, auto high) {
				return ((x - high) * (x - low) <= 0);
			};

			constexpr float speedOfSound = 343.0f;
			return !(!inRange(velocity.x, -speedOfSound, speedOfSound)
				  || !inRange(velocity.y, -speedOfSound, speedOfSound)
				  || !inRange(velocity.z, -speedOfSound, speedOfSound)); };

		//if (isVelocityValid(newVelocity))

		HZ_CORE_ASSERT(isVelocityValid(newVelocity));

		m_AudioListener.SetNewVelocity(newVelocity);
	}

	void MiniAudioEngine::UpdateListenerConeAttenuation(float innerAngle, float outerAngle, float outerGain)
	{
		m_AudioListener.SetNewConeAngles(innerAngle, outerAngle, outerGain);
	}

	
	//==================================================================================
	void MiniAudioEngine::ReleaseFinishedSources()
	{
		HZ_PROFILE_FUNC();

		choc::SmallVector<Audio::SourceID, 32> sourcesFinished;
		sourcesFinished.reserve(m_ActiveSounds.size());

		// Collect finished sources
		for (const auto& source : m_ActiveSounds)
		{
			if (source->IsFinished())
				sourcesFinished.push_back(source->m_SoundSourceID);
		}

		if (!sourcesFinished.empty())
		{
			m_UpdateState.Set(UpdateState::Stats_ActiveSounds);

			for (const Audio::SourceID sourceID : sourcesFinished)
			{
				VoiceInterface::ReleaseVoiceHandle(sourceID);

				Utils::RemoveIf(m_ActiveSounds, [sourceID](const auto& source) { return source->m_SoundSourceID == sourceID; });
				
				// Notify interested parties (e.g. AudioEventsManager)
				m_OnSourceFinished.Invoke(m_SoundSources[sourceID]->m_EventID, sourceID);

				// Return Sound Source for reuse
				m_SourceManager->ReleaseSource(sourceID);

				LOG_VOICES("FreeSourceIDs pushed ID {0}", sourceID);
			}
		}

		// TODO: JP. could use managed stack allocator
		choc::SmallVector<UUID, 32> inactiveEntitites;

		for (const auto& [entityID, data]: m_ObjectData)
		{
			// TODO: JP. currently this gives false positive for Audio Components that are not yet playing.
			//		And those that don't have AutoDestroy set, this can be wasteful.
			if (!HasActiveEvents(entityID))
				inactiveEntitites.push_back(entityID);
		}

		if (!inactiveEntitites.empty())
		{
			// We should not discard object data for non-AutoDestroy objects, we need it to calculate volume levels,
			// which we'll use to handle virtualization in the future.

			std::scoped_lock lock{ m_InvalidEntitiesLock };
			m_InactiveEntities.insert(inactiveEntitites.begin(), inactiveEntitites.end());
		}

		// There should be the same number of entries in these maps
		//HZ_CORE_ASSERT(m_ObjectData.size() == m_EventHandles.size());

		// If no sounds are playing, remove dangling objects
		// which might have been initialized by then failed Play requests.
		if (m_ActiveSounds.empty())
		{
			m_VoiceHandles.clear();
		}
	}

	void MiniAudioEngine::Update(Timestep ts)
	{
		HZ_PROFILE_FUNC("MiniAudioEngine::Update");

		if (!bInitialized)
			return;

		// AudioThread tasks handled before this Update function

		if (m_PlaybackState == EPlaybackState::Playing)
		{
			// Process Events queue
			m_EventsManager->Update(ts);

			// Update listener position from Game Thread
			UpdateListener();

			// Update Emitter data from Game Thread
			UpdateSources();

			if (!m_SoundsToStart.empty())
				m_UpdateState.Set(UpdateState::Stats_ActiveSounds);

			// Start sounds requested to start
			for (auto* sound : m_SoundsToStart)
			{
				sound->Play();
				m_ActiveSounds.push_back(sound);
			}
			m_SoundsToStart.clear();

			// At this stage we should have UpdatedSources for SoundsToStart as well,
			// even if they weren't in ActiveSounds pool yet, because we update VoiceData
			// for initialized Voices, not ActiveSounds.
			for (auto* sound : m_ActiveSounds)
				sound->Update(ts);

		}
		else if (m_PlaybackState == EPlaybackState::Paused)
		{
			const bool pausingOrStopping = std::any_of(m_ActiveSounds.begin(), m_ActiveSounds.end(), [](const SoundObject* sound)
			{
				return (sound->GetPlayState() == ESoundPlayState::Pausing)
					|| (sound->GetPlayState() == ESoundPlayState::Stopping);
			});

			// If any sound is still stopping, or pausing, continue to Update
			if (pausingOrStopping)
			{
				for (auto* sound : m_ActiveSounds)
					sound->Update(ts);
			}
		}

		ReleaseFinishedSources();

		// Must be called at the end of the Update function
		// to process all of the collected requests
		ProcessUpdateState();
	}

	void MiniAudioEngine::ProcessUpdateState()
	{
		std::optional<uint32_t> updatedObjectCount;

		if (m_UpdateState.Has(UpdateState::Stats_AudioObjects))
		{
			updatedObjectCount = (uint32_t)m_ObjectData.size();
		}

		if (m_UpdateState.HasAny())
		{
			std::scoped_lock lock{ s_Stats.mutex };
			s_Stats.NumActiveSounds = (uint32_t)m_ActiveSounds.size();
			s_Stats.ActiveEvents = m_EventsManager->GetNumberOfActiveEvents(); //? this may not be correct, this only returns number of 'registered" events
			
			if (updatedObjectCount)
				s_Stats.AudioObjects = *updatedObjectCount;
		}

		m_UpdateState.Clear();
	}

	void MiniAudioEngine::RegisterNewListener(AudioListenerComponent& listenerComponent)
	{
		// TODO
	}

	//==================================================================================

	SoundObject* MiniAudioEngine::InitiateNewVoice(UUID entityID, EventID eventID, const Ref<SoundConfig>& sourceConfig)
	{
		HZ_CORE_ASSERT(AudioThread::IsAudioThread());

		SoundObject* sound = nullptr;

		int freeID;
		if (!m_SourceManager->GetFreeSourceId(freeID))
		{
			// Stop lowest priority source
			FreeLowestPrioritySource();
			m_SourceManager->GetFreeSourceId(freeID);
			LOG_VOICES("Released voice ID {0}", freeID);
		}

		sound = m_SoundSources.at(freeID);

		// Associate Voice with the Objcet
		sound->m_AudioObjectID = entityID;
		sound->m_EventID = eventID;
		sound->m_SceneID = m_CurrentSceneID; //? it is not ideal to associate sound with scene this way, but it works like a charm

		// TODO: move m_SoundSources to SourceManager
		if (!m_SourceManager->InitializeSource(freeID, sourceConfig)) //? this is weird for now, because SourceManager calls back AudioEngine to init get the source by ID
		{
			HZ_CORE_ERROR_TAG("Audio", "Failed to initialize sound source!");

			// Release resources if failed to initialize
			m_SourceManager->ReleaseSource(freeID);
			return nullptr;
		}
		
		// This is what's used as "Target" for Audio Events.
		// It could be replaced by AssetHandle, but AssetHandle doesn' thold type information,
		// and can be even more volatile than Ref.
		sound->m_SoundConfig = sourceConfig;

		// Register newly initialized voice.
		// Note: the source is not Active yet.
		m_VoiceHandles.push_back({ sound->m_SoundSourceID, sound->m_AudioObjectID, sound->m_EventID });

		return sound;
	}

	EventID MiniAudioEngine::PostTrigger(CommandID triggerCommandID, UUID entityID)
	{
		if (triggerCommandID == 0)
		{
			HZ_CORE_ERROR_TAG("Audio", "PostTrigger with empty triggerCommandID.");
			return EventID::INVALID;
		}

		if (!AudioCommandRegistry::DoesCommandExist<TriggerCommand>(triggerCommandID))
		{
			HZ_CONSOLE_LOG_ERROR("[Audio] PostTrigger. Audio command with ID {0} does not exist!", (uint32_t)triggerCommandID);
			return EventID::INVALID;
		}

		auto& command = AudioCommandRegistry::GetCommand<TriggerCommand>(triggerCommandID);
		LOG_EVENTS("Posting audio trigger event: {0}", command.DebugName);

		// TODO: if entityID is invalid, this was intended to play as a global trigger, or a 2D sound?
		if (entityID == 0)
		{
			HZ_CORE_ERROR_TAG("Audio", "PostTrigger. Invalid entity ID {0}.", entityID);
			return EventID::INVALID;
		}

		EventInfo eventInfo(triggerCommandID, entityID);

		// Add state object to the EventInfo before passing it to the registry
		// TODO: JP. consider moving this to Events Manager as well
		// TODO: JP. maybe also try to remove this allocation
		eventInfo.CommandState = std::make_shared<EventInfo::StateObject>(AudioCommandRegistry::GetCommand<TriggerCommand>(triggerCommandID));

		//! JP. the only reason to stay in GameThread up to this point is to be able to return this EventID, but it's probably not valid if media for the event is not loaded
		EventID eventID = m_EventsManager->RegisterEvent(eventInfo);
		if (eventID)
		{
			auto postTrigger = [this, entityID, eventInfo]() mutable
			{
				m_EventsManager->PostTrigger(eventInfo);
				m_EventHandles[entityID].push_back({ eventInfo.EventID });

				m_UpdateState.Set(UpdateState::Stats_ActiveEvents);
			};

			ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, postTrigger, "Post Trigger");
		}

		return eventID;
	}

	void MiniAudioEngine::SetParameterFloat(Audio::CommandID parameterID, uint64_t objectID, float value)
	{
		const auto setParameter = [=]
		{
			InvokeOnActiveSounds(objectID, [=](SoundObject* sound) { sound->SetParameter(parameterID, value); });
		};
		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, setParameter, "Set Sound Parameter"); //? might overflow the queue
	}

	void MiniAudioEngine::SetParameterInt(Audio::CommandID parameterID, uint64_t objectID, int value)
	{
		const auto setParameter = [=]
		{
			InvokeOnActiveSounds(objectID, [=](SoundObject* sound) { sound->SetParameter(parameterID, value); });
		};
		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, setParameter, "Set Sound Parameter"); //? might overflow the queue
	}

	void MiniAudioEngine::SetParameterBool(Audio::CommandID parameterID, uint64_t objectID, bool value)
	{
		const auto setParameter = [=]
		{
			InvokeOnActiveSounds(objectID, [=](SoundObject* sound) { sound->SetParameter(parameterID, value); });
		};
		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, setParameter, "Set Sound Parameter"); //? might overflow the queue
	}

	void MiniAudioEngine::SetParameterFloat(Audio::CommandID parameterID, Audio::EventID playingEvent, float value)
	{
		auto setParameter = [=]
		{
			InvokeOnActiveSources(playingEvent, [=] (SoundObject* source)
			{
				source->SetParameter(parameterID, value);
			});
		};

		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, setParameter, "Set Sound Parameter");  //? might overflow the queue
	}

	void MiniAudioEngine::SetParameterInt(Audio::CommandID parameterID, Audio::EventID playingEvent, int value)
	{
		auto setParameter = [=]
		{
			InvokeOnActiveSources(playingEvent, [=](SoundObject* source)
			{
				source->SetParameter(parameterID, value);
			});
		};

		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, setParameter, "Set Sound Parameter");  //? might overflow the queue
	}

	void MiniAudioEngine::SetParameterBool(Audio::CommandID parameterID, Audio::EventID playingEvent, bool value)
	{
		auto setParameter = [=]
		{
			InvokeOnActiveSources(playingEvent, [=](SoundObject* source)
			{
				source->SetParameter(parameterID, value);
			});
		};

		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, setParameter, "Set Sound Parameter");  //? might overflow the queue
	}

	void MiniAudioEngine::SetLowPassFilterValueObj(uint64_t objectID, float value)
	{
		auto setLowPassFilter = [=]
		{
			InvokeOnActiveSounds(objectID, [=](SoundObject* sound) { sound->SetLowPassFilter(value); });
		};

		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, setLowPassFilter, "Set Low Pass Filter");  //? might overflow the queue
	}

	void MiniAudioEngine::SetHighPassFilterValueObj(uint64_t objectID, float value)
	{
		auto setHighPassFilter = [=]
		{
			InvokeOnActiveSounds(objectID, [=](SoundObject* sound) { sound->SetHighPassFilter(value); });
		};

		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, setHighPassFilter, "Set High Pass Filter");  //? might overflow the queue
	}

	void MiniAudioEngine::SetLowPassFilterValue(Audio::EventID playingEvent, float value)
	{
		auto setLowPassFilter = [=]
		{
			InvokeOnActiveSources(playingEvent, [=](SoundObject* source)
			{
				source->SetLowPassFilter(value);
			});
		};

		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, setLowPassFilter, "Set Low Pass Filter");  //? might overflow the queue
	}

	void MiniAudioEngine::SetHighPassFilterValue(Audio::EventID playingEvent, float value)
	{
		auto setHighPassFilter = [=]
		{
			InvokeOnActiveSources(playingEvent, [=](SoundObject* source)
			{
				source->SetHighPassFilter(value);
			});
		};

		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, setHighPassFilter, "Set High Pass Filter");  //? might overflow the queue
	}

	//==================================================================================

	Stats MiniAudioEngine::GetStats()
	{
		std::scoped_lock lock{ s_Stats.mutex };
		s_Stats.FrameTime = AudioThread::GetFrameTime();
		return MiniAudioEngine::s_Stats;
	}

	void MiniAudioEngine::StopAll(bool stopNow /*= false*/)
	{
		auto stopAll = [&, stopNow]
		{
			m_SoundsToStart.clear();

			for (auto* sound : m_ActiveSounds)
			{
				if (stopNow)
				{
					if (auto* simpleSound = dynamic_cast<Sound*>(sound))
						simpleSound->StopNow(true, true);
				}
				else
				{
					sound->Stop();
				}
			}

			if (!Application::IsRuntime())
				StopActiveAudioFilePreview();
		};

		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, stopAll, "StopAll");
	}

	void MiniAudioEngine::PauseAll()
	{
		if (m_PlaybackState != EPlaybackState::Playing)
		{
			HZ_CORE_ASSERT(false, "Audio Engine is already paused.");
			return;
		}

		auto pauseAll = [&]
		{
			m_PlaybackState = EPlaybackState::Paused;

			for (auto* sound : m_ActiveSounds)
			{
				sound->Pause();
			}
		};

		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, pauseAll, "PauseAudioEngine");
	}

	void MiniAudioEngine::ResumeAll()
	{
		if (m_PlaybackState != EPlaybackState::Paused)
		{
			HZ_CORE_ASSERT(false, "Audio Engine is already active.");
			return;
		}

		auto resumeAll = [&]
		{
			m_PlaybackState = EPlaybackState::Playing;

			for (auto* sound : m_ActiveSounds)
			{
				ESoundPlayState soundPlayState = sound->GetPlayState();
				// Making sure we only re-start sound if it wasn explicitly paused
				if (soundPlayState == ESoundPlayState::Paused)
				{
					sound->Play();
				}
				else if (soundPlayState == ESoundPlayState::Pausing)
				{
					sound->Play();
				}
			}
		};

		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, resumeAll, "ResumeAudioEngine");
	}

	void MiniAudioEngine::SetSceneContext(const Ref<Scene>& scene)
	{
		//! Game Thread
 
		// This function is called at the beginning of Scene::OnRuntimeStart.

		auto& audioEngine = Get();

		audioEngine.StopAll();

		//? A bit of a hack to handle changing scenes in Editor context.
		//? OnSceneDestruct is not called when creating new scene, or loading a different scene.
		OnSceneDestruct(audioEngine.m_CurrentSceneID);

		audioEngine.m_SceneContext = scene;
		if (scene)
		{
			auto* newScene = audioEngine.m_SceneContext.Raw();
			const auto newSceneID = newScene->GetUUID();
			audioEngine.m_CurrentSceneID = newSceneID;

			audioEngine.m_UpdateState.Set(UpdateState::Stats_AudioComponents);

			// Note: JP. We "Start on Awake" in 'OnRuntimePlaying()',
			//		which is called at the end of Scene::OnRuntimeStart.
		}
	}

	void MiniAudioEngine::OnRuntimePlaying(UUID sceneID)
	{
		//! Game Thread

		// This function is called at the end of Scene::OnRuntimeStart.

		auto& audioEngine = Get();

		const auto currentSceneID = audioEngine.m_CurrentSceneID;
		HZ_CORE_ASSERT(currentSceneID == sceneID)

		auto newScene = Scene::GetScene(currentSceneID);

		auto view = newScene->GetAllEntitiesWith<AudioComponent>();
		for (auto entity : view)
		{
			Entity audioEntity = { entity, newScene.Raw() };

			auto& ac = audioEntity.GetComponent<AudioComponent>();
			if (ac.bPlayOnAwake)
			{
				auto newScene = Scene::GetScene(currentSceneID);
				// TODO: Play on awake! Duck-taped.
				//       Proper implementation requires enablement state for components.
				//       For now just checking if this component was created in Runtime scene.
				if (!newScene->IsEditorScene() && newScene->IsPlaying())
				{
					audioEngine.SubmitStartPlayback(audioEntity.GetUUID());
				}
			}
		}

		//HZ_CORE_INFO_TAG("Audio", "ON RUNTIME PLAYING");
	}

	void MiniAudioEngine::OnSceneDestruct(UUID sceneID)
	{
		ExecuteOnAudioThread([] {
			auto& instance = Get();
			instance.m_ObjectData.clear();
			instance.m_VoiceHandles.clear();
			instance.m_UpdateState.Set(UpdateState::Stats_AudioObjects);
		});

		//HZ_CORE_INFO_TAG("Audio", "ON SCENE DESTRUCT");
	}

	Ref<Scene>& MiniAudioEngine::GetCurrentSceneContext()
	{
		return Get().m_SceneContext;
	}


	//==================================================================================
	void MiniAudioEngine::OnAudibleEntityCreated(Entity audioEntity)
	{
		//! Game Thread

		// Handle Play on Awake

		const auto& audioComponent = audioEntity.GetComponent<AudioComponent>();

		if (audioComponent.bPlayOnAwake)
		{
			auto scene = Scene::GetScene(audioEntity.GetSceneUUID());
			// TODO: Play on awake! Duck-taped.
			//       Proper implementation requires enablement state for components.
			//       For now just checking if this component was created in Runtime scene.

			// This is going to be 'true' only when new AudioComponents added during play of a scene,
			// skipped when scenes are copied.
			if (!scene->IsEditorScene() && scene->IsPlaying())
			{
				SubmitStartPlayback(audioEntity.GetUUID());
			}
		}
	}

	void MiniAudioEngine::OnAudibleEntityDestroy(Entity entity)
	{
		//! Game Thread

		const auto& audioComponent = entity.GetComponent<AudioComponent>();

		// TODO: JP. handle orphan Voices when this flag is not set for a component
		if (audioComponent.bStopWhenEntityDestroyed)
			StopActiveSoundSource(entity.GetUUID());
	}

	void MiniAudioEngine::SerializeSceneAudio(YAML::Emitter& out, const Ref<Scene>& scene)
	{
		out << YAML::Key << "SceneAudio";
		out << YAML::BeginMap; // SceneAudio
		if (m_MasterReverb)
		{
			out << YAML::Key << "MasterReverb";
			out << YAML::BeginMap; // MasterReverb
			auto storeParameter = [this, &out](DSP::EReverbParameters type)
			{
				out << YAML::Key << m_MasterReverb->GetParameterName(type) << YAML::Value << m_MasterReverb->GetParameter(type);
			};

			storeParameter(DSP::EReverbParameters::PreDelay);
			storeParameter(DSP::EReverbParameters::RoomSize);
			storeParameter(DSP::EReverbParameters::Damp);
			storeParameter(DSP::EReverbParameters::Width);

			out << YAML::EndMap; // MasterReverb
		}
		out << YAML::EndMap; // SceneAudio
	}

	void MiniAudioEngine::DeserializeSceneAudio(YAML::Node& data)
	{
		auto masterReverb = data["MasterReverb"];

		if (masterReverb && m_MasterReverb)
		{
			auto setParam = [this, masterReverb](DSP::EReverbParameters type)
			{
				m_MasterReverb->SetParameter(type, masterReverb[m_MasterReverb->GetParameterName(type)].as<float>());
			};

			setParam(DSP::EReverbParameters::PreDelay);
			setParam(DSP::EReverbParameters::RoomSize);
			setParam(DSP::EReverbParameters::Damp);
			setParam(DSP::EReverbParameters::Width);

		}
	}

	void MiniAudioEngine::PreviewAudioFile(AssetHandle sourceFile)
	{
		auto playAudioFile = [this, sourceFile]
		{
			if (!Application::IsRuntime() && bInitialized)
			{
				if (ma_sound_get_data_source(&m_PreviewSound))
					ma_sound_uninit(&m_PreviewSound);

				ma_uint32 soundFlags = 0;
				soundFlags |= MA_SOUND_FLAG_ASYNC;					/* For inlined sounds we don't want to be sitting around waiting for stuff to load so force an async load. */
				soundFlags |= MA_SOUND_FLAG_NO_DEFAULT_ATTACHMENT;	/* We want specific control over where the sound is attached in the graph. We'll attach it manually just before playing the sound. */
				soundFlags |= MA_SOUND_FLAG_NO_PITCH;				/* Pitching isn't usable with inlined sounds, so disable it to save on speed. */
				soundFlags |= MA_SOUND_FLAG_NO_SPATIALIZATION;		/* Not using miniaudio's spatialization. */

				const std::string sourceFileKey = std::to_string(sourceFile);
				ma_sound_init_from_file(&m_Engine, sourceFileKey.c_str(), soundFlags, nullptr, nullptr, &m_PreviewSound);

				// For now we attach straight to the endpoint,
				// in the future we may want to have a special "editor audio" bus
				ma_node_base* attachment = &m_Engine.nodeGraph.endpoint;
				ma_node_attach_output_bus(&m_PreviewSound, 0, attachment, 0);

				ma_sound_start(&m_PreviewSound);
			}
		};

		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, playAudioFile, "PreviewAudioFile");
	}

	void MiniAudioEngine::StopActiveAudioFilePreview()
	{
		auto stopPreviewPlayback = [this]
		{
			if (!Application::IsRuntime() && bInitialized)
			{
				if (ma_sound_get_data_source(&m_PreviewSound))
					ma_sound_uninit(&m_PreviewSound);
			}
		};

		ExecuteOnAudioThread(EIfThisIsAudioThread::ExecuteNow, stopPreviewPlayback, "PreviewAudioFile");
	}
	
	//==================================================================================
	bool MiniAudioEngine::BuildSoundBank(const std::filesystem::path& path)
	{
		if (Get().GetCurrentSceneContext()->IsPlaying())
		{
			HZ_CONSOLE_LOG_WARN("Can't build Sound Bank while playing a scene!");
			return false;
		}

		HZ_CONSOLE_LOG_INFO("Packaging Sound Bank...");

		Get().StopAll(true);
		
		// This will add this request to the end of the queue,
		// and it should hopefuly be processed after the 'StopAll'
		ExecuteOnAudioThread([path] { Get().m_ResourceManager->BuildSoundBank(path); }, "Build SoundBank");
		return true;
	}
	bool MiniAudioEngine::UnloadCurrentSoundBank()
	{
		if (Get().GetCurrentSceneContext()->IsPlaying())
		{
			HZ_CONSOLE_LOG_WARN("Can't unload Sound Bank while playing a scene!");
			return false;
		}

		HZ_CONSOLE_LOG_INFO("Unloading Sound Bank...");

		Get().StopAll(true);
		
		// This will add this request to the end of the queue,
		// and it should hopefuly be processed after the 'StopAll'
		ExecuteOnAudioThread([] {
			Get().m_ResourceManager->ReleaseResources();
		}, "Release SoundBank");
		return true;
	}

} // namespace Hazel

#pragma once

#include "miniaudio_incl.h"

#include "Engine/Core/Timer.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Entity.h"

#include "Audio.h"
#include "VFS.h"
#include "Sound.h"
#include "AudioObject.h"
#include "EntityIDMaps.h"
#include "AudioEvents/AudioCommands.h"
#include "AudioEvents/CommandID.h"
#include "AudioComponent.h"
#include "Engine/Scene/Components.h"
#include "Engine/Utilities/ContainerUtils.h"

#include "AudioPlayback.h"

#include <optional>
#include <queue>
#include <shared_mutex>
#include <thread>

#include <glm/glm.hpp>

namespace YAML
{
    class Emitter;
    class Node;
}

namespace Hazel
{
    class SourceManager;
    class Sound;
    struct AudioComponent;
    struct AudioObjectData;

	namespace Audio
	{
		class ResourceManager;
		struct VFS;
		class AudioEventsManager;
	}

    namespace Audio::DSP 
    {
        struct Reverb;
    }

    namespace Audio
    {
        struct Stats
        {
            Stats() = default;
            Stats(const Stats& other)
            {
				// Locking should be handled by the client code
                //std::shared_lock lock{ other.mutex };

                AudioObjects = other.AudioObjects;
                ActiveEvents = other.ActiveEvents;
                NumActiveSounds = other.NumActiveSounds;
                TotalSources = other.TotalSources;
                MemEngine = other.MemEngine;
                MemResManager = other.MemResManager;
                FrameTime = other.FrameTime;
            }

            uint32_t AudioObjects = 0;
            uint32_t ActiveEvents = 0;
            uint32_t NumActiveSounds = 0;
            uint32_t TotalSources = 0;
            uint64_t MemEngine = 0;
            uint64_t MemResManager = 0;
            float FrameTime = 0.0f;

            mutable std::shared_mutex mutex;
        };
    

        /* ----------------------------------------------------------------
            For now just a flag to pass into the memory allocation callback
            to sort allocation stats accordingly to the source of operation
            ---------------------------------------------------------------
        */
        struct AllocationCallbackData // TODO: hide this into Source Manager?
        {
            bool isResourceManager = false;
            Stats& Stats;
        };

    } // namespace Audio

	/*  =================================================================
	    Object representing current state of the Audio Listener

	    Updated from Game Thead, checked and updated to from Audio Thread
	    -----------------------------------------------------------------
	*/
    struct AudioListener
    {
        bool PositionNeedsUpdate(const Audio::Transform& newTransform) const
        {
            std::shared_lock lock{ m_Mutex };
            //return newPosition != m_LastPosition || newDirection != m_LastDirection;
            return m_Transform != newTransform;
        }

        void SetNewPositionDirection(const Audio::Transform& newTransform)
        {
            std::unique_lock lock{ m_Mutex };
            //m_LastPosition = newPosition;
            //m_LastDirection = newDirection;
            m_Transform = newTransform;
            m_Changed = true;
        }

        void SetNewVelocity(const glm::vec3& newVelocity)
        {
            std::unique_lock lock{ m_Mutex };
            m_Changed = newVelocity != m_LastVelocity;
            m_LastVelocity = newVelocity;
        }

        void SetNewConeAngles(float innerAngle, float outerAngle, float outerGain)
        {
            m_InnerAngle.store(innerAngle);
            m_OuterAngle.store(outerAngle);
            m_OuterGain.store(outerGain);
            m_Changed = true;
        }

        [[nodiscard]] std::pair<float, float> GetConeInnerOuterAngles() const
        {
            return { m_InnerAngle.load(), m_OuterAngle.load() };
        }

        [[nodiscard]] float GetConeOuterGain() const
        {
            return m_OuterGain.load();
        }

        void GetVelocity(glm::vec3& velocity) const
        {
            std::shared_lock lock{ m_Mutex };
            velocity = m_LastVelocity;
        }

        Audio::Transform GetPositionDirection() const
        {
            std::shared_lock lock{ m_Mutex };
            //position = m_LastPosition;
            //direction = m_LastDirection;
            return m_Transform;
        }

        bool HasChanged(bool resetFlag) const 
        { 
            bool changed = m_Changed.load();
            if (resetFlag) m_Changed.store(false);
            return changed;
        }
    private:
        mutable std::shared_mutex m_Mutex;
        //glm::vec3 m_LastPosition{ 0.0f, 0.0f, 0.0f };
        //glm::vec3 m_LastDirection{ 0.0f, 0.0f, -1.0f };
        glm::vec3 m_LastVelocity{ 0.0f, 0.0f, 0.0f };
        Audio::Transform m_Transform;
        mutable std::atomic<bool> m_Changed = false;

        std::atomic<float> m_InnerAngle{ 6.283185f }, m_OuterAngle{ 6.283185f }, m_OuterGain{ 0.0f };
    };

    /* ========================================
	   Generic Audio operations

	   Hardware Initialization, main Update hub
	   ----------------------------------------
    */
    class MiniAudioEngine
    {
    public:
        MiniAudioEngine();
        ~MiniAudioEngine();

        /* Initialize Instance */
        static void Init();

        /* Shutdown AudioEngine and tear down hardware initialization */
        static void Shutdown();

        static inline MiniAudioEngine& Get() { return *s_Instance; }
        static ma_engine* GetMiniaudioEngine() { return &s_Instance->m_Engine; }

        static void SetSceneContext(const Ref<Scene>& scene);
        static Ref<Scene>& GetCurrentSceneContext();
        static void OnRuntimePlaying(UUID sceneID);
        static void OnSceneDestruct(UUID sceneID);

        void SerializeSceneAudio(YAML::Emitter& out, const Ref<Scene>& scene);
        void DeserializeSceneAudio(YAML::Node& data);

		//==================================================================================
		/** Play audio file for the preview. Used in editor only.
		
		    @param sourceFile - source audio file asset
		*/
		void PreviewAudioFile(AssetHandle sourceFile);
		/** Stop any playing preview sounds. Used in editor only. */
		void StopActiveAudioFilePreview();

		//==================================================================================
		/// Packaging
		static bool BuildSoundBank();
		static bool UnloadCurrentSoundBank();

		//==================================================================================
        static Audio::Stats GetStats();

		/* Initializes AudioEngine and hardware. This is called from AudioEngine instance construct. 
            Executed on Audio Thread.

		   @returns true - if initialization succeeded, or if AudioEngine already initialized
		*/
        bool Initialize();

        /* Tear down hardware initialization. This is called from Shutdown(). Or AudioEngine deconstruct
            
		   @returns true - if initialization succeeded, or if AudioEngine already initialized
		*/
        bool Uninitialize();

		/** Should be called when a new project is loaded. */
		void OnProjectLoaded();

        //==================================================================================

		void StopAll(bool stopNow = false);
		void PauseAll();
		void ResumeAll();

		/* Stop all Active Sounds
		   @param stopNow - stop immediately if set 'true', otherwise apply "stop-fade" 
		*/
        static void StopAllSounds(bool stopNow = false) { s_Instance->StopAll(stopNow); }

        /* Pause Audio Engine. E.g. when game minimized. */
        static void Pause() { s_Instance->PauseAll(); } // TODO: just pause audio rendering callback, as well as Updates
        /* Resume paused Audio Engine. E.g. when game window restored from minimized state. */
        static void Resume() { s_Instance->ResumeAll(); }

        // Playback stated used to pause, or resume all of the audio globaly when game minimized.
        enum class EPlaybackState
        {
            Playing,
            Paused
        } m_PlaybackState{ EPlaybackState::Playing }; // Could add in "Initialized" state as well in the future.

        const SourceManager* GetSourceManager() { return m_SourceManager.get(); }
        const SourceManager* GetSourceManager() const { return m_SourceManager.get(); }

		Audio::ResourceManager* GetResourceManager() { return m_ResourceManager.get(); }
		const Audio::ResourceManager* GetResourceManager() const { return m_ResourceManager.get(); }

        Audio::DSP::Reverb* GetMasterReverb() { return m_MasterReverb.get(); }

		struct UserConfig
		{
			double FileStreamingDurationThreshold = 30.0;
		};
		UserConfig GetUserConfiguration() const { std::scoped_lock sl(m_UserConfigLock); return m_UserConfig; }
		void SetUserConfiguration(const UserConfig& newConfig) { std::scoped_lock sl(m_UserConfigLock); m_UserConfig = newConfig; }

        //==================================================================================

        /* Main Audio Thread tick update function */
        void Update(Timestep ts);

        /* Submit data to update Sound Sources from Game Thread.
		   @param updateData - updated data submitted on scene update
		*/
        void SubmitSourceUpdateData(std::vector<SoundSourceUpdateData> updateData);
		
		/**	Must be called by Game Thread to delete any Entities that were created
			for one-shot Audio Events.
		*/
		std::unordered_set<UUID> GetInactiveEntities();

        /* Update Audio Listener position from game Entity owning active AudioListenerComponent.
            Called from Game Thread.

		   @param newTranslation - new position to check Audio Listener against and to update from if changed
		   @param newDirection - new facing direction to check Audio Listener against and to update from if changed
		*/
        void UpdateListenerPosition(const Audio::Transform& newTransform);

        /* Update Audio Listener velocity from game Entity owning active AudioListenerComponent.
            Called from Game Thread.

		   @param newVelocity - new velocity to update Audio Listener from. This doesn't check if changed.
		*/
        void UpdateListenerVelocity(const glm::vec3& newVelocity);

        void UpdateListenerConeAttenuation(float innerAngle, float outerAngle, float outerGain);

        // TODO
        void RegisterNewListener(AudioListenerComponent& listenerComponent);

        
        //==================================================================================
        Audio::EventID PostTrigger(Audio::CommandID triggerCommandID, UUID objectID);

        // TODO
        void SetSwitch(Audio::SwitchCommand switchCommand, Audio::CommandID valueID);
        void SetState(Audio::StateCommand switchCommand, Audio::CommandID valueID);
        void SetParameter(Audio::ParameterCommand parameter, double value);
        
        SoundObject* InitiateNewVoice(UUID objectID, Audio::EventID playbackID, const Ref<SoundConfig>& sourceConfig);

    private:
        //==================================================================================
        // Internal utilities
        template<typename Function>
		void InvokeOnActiveSounds(uint64_t objectID, Function functionToInvoke);

		template<typename Function>
		void InvokeOnActiveSources(Audio::EventID playingEvent, Function functionToInvoke);

    public:
        //==================================================================================
        /// Sound Object Parameter Interface
        void SetParameterFloat(Audio::CommandID parameterID, uint64_t objectID, float value);
        void SetParameterInt(Audio::CommandID parameterID, uint64_t objectID, int value);
        void SetParameterBool(Audio::CommandID parameterID, uint64_t objectID, bool value);

		// TODO: move parameter storage/assignment to GameObject/AudioComponent?
        void SetParameterFloat(Audio::CommandID parameterID, Audio::EventID playingEvent, float value);
        void SetParameterInt(Audio::CommandID parameterID, Audio::EventID playingEvent, int value);
        void SetParameterBool(Audio::CommandID parameterID, Audio::EventID playingEvent, bool value);

		void SetLowPassFilterValueObj(uint64_t objectID, float value);
		void SetHighPassFilterValueObj(uint64_t objectID, float value);

		void SetLowPassFilterValue(Audio::EventID playbackID, float value);
		void SetHighPassFilterValue(Audio::EventID playbackID, float value);

        //==================================================================================

        // TODO: add blocking vs async versions
        /* Execute arbitrary function on Audio Thread. Used to synchronize updates between Game Thread and Audio Thread */
        static void ExecuteOnAudioThread(Audio::AudioThreadCallbackFunction func, const char* jobID = "NONE");

		enum EIfThisIsAudioThread
		{
			ExecuteNow,
			ExecuteAsync // Add to the audio thread job queue
		};
        static void ExecuteOnAudioThread(EIfThisIsAudioThread policy, Audio::AudioThreadCallbackFunction func, const char* jobID = "NONE");

		//==================================================================================
        /** 	Callback from Game Thread when an AudioComponent has been created
			(marking Entity as "audible" for our semantics)
		*/
        void OnAudibleEntityCreated(Entity entity);

        /** Callback from Game Thread when an Entity with AudioComponent
			is being destroyed.
		*/
        void OnAudibleEntityDestroy(Entity entity);

    private:
        /* Pre-initialize pool of Sound Sources */
        void CreateSources();

		//==========================================================================
		/// Used to flag update of stats at the end of the Update function, to not
		/// have to lock the Stats object inside of potentially hot loops.
		struct UpdateState
		{
			enum EFlags : int
			{
				Stats_ActiveSounds = BIT(0),
				Stats_AudioObjects = BIT(1),
				Stats_AudioComponents = BIT(2),
				Stats_ActiveEvents = BIT(3),
			};

			void Set(int flag) { m_Flags |= flag; }
			bool Has(EFlags flag) const { return (m_Flags & flag); }
			bool HasAny() const { return (bool)m_Flags; }
			int GetFlags() const { return m_Flags; }
			void Clear() { m_Flags = 0; }

		private:
			int m_Flags = 0;
		} m_UpdateState;

		void ProcessUpdateState();

		//==========================================================================
        /* Release sources that finished their playback back into pool */
        void ReleaseFinishedSources();

        /* Update back-end audio listener from AudioListener state object */
        void UpdateListener();

        /* Internal function call to update sound sources to new values recieved from Game Thread */
        void UpdateSources();

        /* This is called when there is no free source available in pool for new playback start request. */
        SoundObject* FreeLowestPrioritySource();


        //==================================================================================
        /// Playback interface. In most cases these functions are called from Game Thread.

		/*  Internal version of "Play" command. Posts Audio Event assigned to AudioComponent.

		    @param audioComponentID - AudioComponent to associate new sound with
		    @returns false - if failed to find audioComponentID in AudioComponentRegistry
		*/
        bool SubmitStartPlayback(uint64_t audioComponentID);

		/*  Internal version of "Stop" command. Attempts to stop Active Sources
		    initiated by playing events on AudioComponent.

		    @returns false - if failed to post event, or if audioComponentID is invalid
		*/
        bool StopActiveSoundSource(uint64_t audioComponentID);

        /*  Internal version of "Pause" command. Attempts to pause Active Sources
		    initiated by playing events on AudioComponent.

		    @returns false - if failed to find audioComponentID in AudioComponentRegistry
		*/
        bool PauseActiveSoundSource(uint64_t audioComponentID);

        /*  Internal version of "Resume" command. Attempts to resume Active Sources
             initiated by playing events on AudioComponent.

             @returns false - if failed to find audioComponentID in AudioComponentRegistry
         */
        bool ResumeActiveSoundSource(uint64_t audioComponentID);
        
        bool StopEventID(Audio::EventID playingEvent);
        bool PauseEventID(Audio::EventID playingEvent);
        bool ResumeEventID(Audio::EventID playingEvent);

        /*  Check if AudioComponent has any Active Events associated to it

		    @returns false - if there are not Active Events associated to the entityID
		*/
        bool HasActiveEvents(uint64_t entityID);
		
		//==================================================================================
		/// Action Handler interface (these functions must be called from Audio Thread only)
		// TODO: JP. move or wrap it in a nicer way
		int Action_StartPlayback(UUID objectID, Audio::EventID playingEvent, Ref<SoundConfig> target);

		bool Action_PauseVoicesOnAnObject(UUID objectID, Ref<SoundConfig> target);
		void Action_PauseVoices(Ref<SoundConfig> target);

		bool Action_ResumeVoicesOnAnObject(UUID objectID, Ref<SoundConfig> target);
		bool Action_ResumeVoices(Ref<SoundConfig> target);

		bool Action_StopVoicesOnAnObject(UUID objectID, Ref<SoundConfig> target);
		void Action_StopVoices(Ref<SoundConfig> target);

		void Action_StopAllOnObject(UUID objectID);
		void Action_StopAll();

		void Action_PauseAllOnObject(UUID objectID);
		void Action_PauseAll();

		bool Action_ResumeAllOnObject(UUID objectID);
		bool Action_ResumeAll();

		void Action_ExecuteOnSources(Audio::EActionTypeOnPlayingEvent action, const std::vector<Audio::SourceID>& sources);

    private:
        friend class SourceManager;
        friend class Audio::ResourceManager;
        friend class Sound;
        friend class AudioPlayback;

        ma_engine m_Engine;
        ma_log m_maLog;
        bool bInitialized = false;
        ma_sound m_PreviewSound;

		mutable std::mutex m_UserConfigLock;
		UserConfig m_UserConfig;
        Scope<Audio::DSP::Reverb> m_MasterReverb;

		Scope<Audio::ResourceManager> m_ResourceManager;
		Scope<Audio::VFS> m_ResourceManagerVFS;
		Scope<SourceManager> m_SourceManager;
		Scope<Audio::AudioEventsManager> m_EventsManager;

        AudioListener m_AudioListener;
        Ref<Scene> m_SceneContext;
        UUID m_CurrentSceneID;

		/* Maximum number of sources availble due to platform limitation, or user settings.
			(for now this is just an arbitrary number)
		*/
        int m_NumSources = 0;
        std::vector<SoundObject*> m_SoundSources;
        std::vector<SoundObject*> m_ActiveSounds;
        std::vector<SoundObject*> m_SoundsToStart;

		//==============================================
        std::mutex m_UpdateSourcesLock;
		AtomicFlag flag_SourcesUpdated;
        std::vector<SoundSourceUpdateData> m_SourceUpdateData;
		
		//==============================================

		struct VoiceData
		{
			float Pitch = 1.0f;
			float Volume = 1.0f;
		};

		struct VoiceHandle								//! never changes
		{
			Audio::SourceID ID;							// ID to access by

			UUID OwningEntity;
			Audio::EventID InvokerEvent;
		};

		struct EventHandle
		{
			Audio::EventID ID;							// ID to access by

		private:
			friend bool operator==(const EventHandle& lhs, const EventHandle& rhs)
			{
				return lhs.ID == rhs.ID;
			}
		};

		std::vector<VoiceData> m_VoiceData;	// Initialized with the pool of Sources (for now)
		std::vector<VoiceHandle> m_VoiceHandles;
		std::unordered_map<UUID, std::vector<EventHandle>> m_EventHandles;

		std::unordered_map<UUID, AudioObjectData> m_ObjectData;	//! Audio Entity positions

		std::mutex m_InvalidEntitiesLock;
		std::unordered_set<UUID> m_InactiveEntities;

		struct VoiceInterface
		{
			static void SetPropertyOnVoices(UUID entityID, float VoiceData::* prop, float value)
			{
				auto& engine = MiniAudioEngine::Get();

				for (const auto& voice : engine.m_VoiceHandles)
				{
					if (voice.OwningEntity == entityID)
					{
						engine.m_VoiceData[voice.ID].*prop = value;
					}
				}
			}

			static void ReleaseVoiceHandle(Audio::SourceID id)
			{
				auto& engine = MiniAudioEngine::Get();
				Utils::RemoveIf(engine.m_VoiceHandles, [id](const VoiceHandle& handle) { return handle.ID == id; });
			}
		};
		friend struct VoiceInterface;

        //==============================================
        static MiniAudioEngine* s_Instance;

        static Audio::Stats s_Stats;
        Audio::AllocationCallbackData m_EngineCallbackData{false, s_Stats};
        Audio::AllocationCallbackData m_RMCallbackData{true, s_Stats};
		
		Delegate<void(Audio::EventID, Audio::SourceID)> m_OnSourceFinished;
    };
} // namespace Hazel

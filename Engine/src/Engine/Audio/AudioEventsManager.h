#pragma once

#include "AudioEngine.h"
#include "EntityIDMaps.h"

namespace Engine::Audio {

	class AudioEventsManager
	{
	public:
		~AudioEventsManager() = default;

		void Update(Timestep ts);

		/** Returnsand assigns the same EventID to the supplied EventInfo */
		EventID RegisterEvent(EventInfo& info);
		
		/** Unregister source with the playing event. Called by AudioEngine. */
		void OnSourceFinished(EventID playingEvent, SourceID source);
		
		void PostTrigger(const EventInfo& eventInfo);

		uint32_t GetNumberOfActiveEvents() const { return m_EventRegistry.Count(); }
		uint32_t GetNumberOfActiveSources(EventID playingEvent) const { return m_EventRegistry.GetNumberOfSources(playingEvent); }
		std::vector<SourceID> GetActiveSources(EventID playingEvent) const { return m_EventRegistry.Get(playingEvent).ActiveSources; }

		/** Execute Action on all active sources of a playing event. */
		bool ExecuteActionOnPlayingEvent(EventID playingEvent, EActionTypeOnPlayingEvent action);

	private:
		friend MiniAudioEngine::MiniAudioEngine();
		explicit AudioEventsManager(MiniAudioEngine& audioEngine);
		AudioEventsManager() = delete;
		
		struct ActionHandler;
		
		void AssignActionHandler(ActionHandler&& actionHandler) { m_ActionHandler = std::move(actionHandler); }

		bool ProcessTriggerCommand(EventInfo& info);
		bool ProcessSwitchCommand(EventInfo& info);
		bool ProcessStateCommand(EventInfo& info);
		bool ProcessParameterCommand(EventInfo& info);

	private:
		MiniAudioEngine& m_AudioEngine;
		std::queue<std::pair<Audio::ECommandType, Audio::EventInfo>> m_CommandQueue;

		// Map of all active Events
		Audio::EventRegistry m_EventRegistry;

		Delegate<void(EventID, UUID)> m_OnEventFinished;

		// Handling trigger command actions
		struct ActionHandler
		{
			// should return source ID of the initialized voice, or -1 if failed
			Delegate<int(UUID objectID, EventID playingEvent, Ref<SoundConfig> target)>		StartPlayback;			// Submit voice for playback
			
			Delegate<bool(UUID objectID, Ref<SoundConfig> target)>							PauseVoicesOnAnObject;	// Pause voices of a target SoundConfig on an Entity
			Delegate<void(Ref<SoundConfig> target)>											PauseVoices;			// Pause voices of a target SoundConfig everywhere
			
			Delegate<bool(UUID objectID, Ref<SoundConfig> target)>							ResumeVoicesOnAnObject;	// Resume voices of a target SoundConfig on an Entity
			Delegate<bool(Ref<SoundConfig> target)>											ResumeVoices;			// Resume voices of a target SoundConfig everywhere
			
			Delegate<bool(UUID objectID, Ref<SoundConfig> target)>							StopVoicesOnAnObject;	// Stop voices of a target SoundConfig on an Entity
			Delegate<void(Ref<SoundConfig> target)>											StopVoices;				// Stop all Voices of a target SoundConfig
			
			Delegate<void(UUID objectID)>													StopAllOnObject;		// Stop all voices an Entity
			Delegate<void()>																StopAll;				// Stop all voices
			
			Delegate<void(UUID objectID)>													PauseAllOnObject;		// Pause all voices an Entity
			Delegate<void()>																PauseAll;				// Pause all voices
			
			Delegate<bool(UUID objectID)>													ResumeAllOnObject;		// Resume all voices an Entity
			Delegate<bool()>																ResumeAll;				// Resume all voices

			Delegate<void(EActionTypeOnPlayingEvent action, const std::vector<SourceID>& sources)> ExecuteOnSources; //? not ideal to pass vector by reference, need to be careful with the lifetime
		} m_ActionHandler;
	};

} // namespace Engine::Audio

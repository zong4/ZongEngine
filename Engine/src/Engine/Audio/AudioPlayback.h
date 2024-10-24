#pragma once

#include "Audio.h"
#include "Sound.h"
#include "AudioEvents/AudioCommands.h"
#include "AudioEvents/CommandID.h"

#include <optional>

namespace Engine
{
	/*  ========================================
		Audio Playback interface

         In the future this might be be replaced
         with Audio Events Abstraction Layer 
		---------------------------------------
	*/
    class AudioPlayback
    {
    public:

        // Playback of sound source associated to an existing AudioComponent
        static bool Play(uint64_t audioEntityID, float startTime = 0.0f/*, float fadeInTime = 0.0f*/); // TODO: fadeInTime
		static bool StopActiveSound(uint64_t audioEntityID);
		static bool PauseActiveSound(uint64_t audioEntityID);
		static bool Resume(uint64_t audioEntityID);
		static bool IsPlaying(uint64_t audioEntityID);

		// TODO: replace with Parameter Controls
#if OLD_API
		static void SetMasterReverbSend(uint64_t audioComponetnID, float sendLevel);
		static float GetMasterReverbSend(uint64_t audioComponetnID);
#endif
		//==================================
		//=== Audio Events Abstraction Layer 

		static uint32_t PostTrigger(Audio::CommandID triggerCommandID, uint64_t audioEntityID);
		static uint32_t PostTriggerFromAC(Audio::CommandID triggerCommandID, uint64_t audioEntityID);

		static uint32_t PostTriggerAtLocation(Audio::CommandID triggerID, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale = { 1.0f, 1.0f, 1.0f });

		// TODO: rewrite these to take in context: Global, Entity, Event
		static void SetParameterFloat(Audio::CommandID parameterID, uint64_t audioEntityID, float value);
		static void SetParameterInt(Audio::CommandID parameterID, uint64_t audioEntityID, int value);
		static void SetParameterBool(Audio::CommandID parameterID, uint64_t audioEntityID, bool value);
		static void SetParameterFloatForAC(Audio::CommandID parameterID, uint64_t audioEntityID, float value);
		static void SetParameterIntForAC(Audio::CommandID parameterID, uint64_t audioEntityID, int value);
		static void SetParameterBoolForAC(Audio::CommandID parameterID, uint64_t audioEntityID, bool value);

		static void SetParameterFloat(Audio::CommandID parameterID, uint32_t eventID, float value);
		static void SetParameterInt(Audio::CommandID parameterID, uint32_t eventID, int value);
		static void SetParameterBool(Audio::CommandID parameterID, uint32_t eventID, bool value);

		static bool StopEventID(uint32_t playingEvent);
		static bool PauseEventID(uint32_t playingEvent);
		static bool ResumeEventID(uint32_t playingEvent);

		/** Preload Wave Assets referenced by 'Play' action in the even with 'eventID'. */
		static void PreloadEventSources(Audio::CommandID eventID);

		/** Unload Wave Assets reference by 'Play' action in the even with 'eventID'.
			Unloaded Wave Assets can still be loaded next time they are accessed for playback.
		*/
		static void UnloadEventSources(Audio::CommandID eventID);

		static void SetLowPassFilterValueObj(uint64_t audioEntityID, float value);
		static void SetHighPassFilterValueObj(uint64_t audioEntityID, float value);

		static void SetLowPassFilterValue(uint32_t eventID, float value);
		static void SetHighPassFilterValue(uint32_t eventID, float value);

		static void SetLowPassFilterValueAC(uint64_t audioEntityID, float value);
		static void SetHighPassFilterValueAC(uint64_t audioEntityID, float value);
    };

}

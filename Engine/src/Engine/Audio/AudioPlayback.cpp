#include <pch.h>
#include "AudioPlayback.h"

#include "AudioEngine.h"
#include "SourceManager.h"
#include "AudioComponent.h"
#include "Engine/Scene/Components.h"
#include "Engine/Audio/AudioEvents/AudioCommandRegistry.h"

#include "Engine/Scene/Scene.h"

#include "Engine/Debug/Profiler.h"

namespace Hazel
{
    using namespace Audio;

    // TODO: in the future get active instance of AudioEngine from "World", or from a Game Instance,
    //       get "World" or a Game Instance from passed in Entity / caller

bool AudioPlayback::Play(uint64_t audioEntityID, float startTime)
{
	ZONG_PROFILE_FUNC();

    auto& engine = MiniAudioEngine::Get();
    return engine.SubmitStartPlayback(audioEntityID);
}

bool AudioPlayback::StopActiveSound(uint64_t audioEntityID)
{
	ZONG_PROFILE_FUNC();

    auto& engine = MiniAudioEngine::Get();
    return engine.StopActiveSoundSource(audioEntityID);
}

bool AudioPlayback::PauseActiveSound(uint64_t audioEntityID)
{
	ZONG_PROFILE_FUNC();

    auto& engine = MiniAudioEngine::Get();
    return engine.PauseActiveSoundSource(audioEntityID);
}

bool AudioPlayback::Resume(uint64_t audioEntityID)
{
    ZONG_PROFILE_FUNC();

    auto& engine = MiniAudioEngine::Get();
    return engine.ResumeActiveSoundSource(audioEntityID);
}

bool AudioPlayback::IsPlaying(uint64_t audioEntityID)
{
	ZONG_PROFILE_FUNC();

    auto& engine = MiniAudioEngine::Get();
    return engine.HasActiveEvents(audioEntityID);
}

#if OLD_API
void AudioPlayback::SetMasterReverbSend(uint64_t audioComponetnID, float sendLevel)
{
	ZONG_PROFILE_FUNC();

    auto& engine = MiniAudioEngine::Get();

    if (auto* sound = engine.GetSoundForAudioComponent(audioComponetnID))
    {
        SourceManager::SetMasterReverbSendForSource(sound, sendLevel);
    
        //? Don't need to change anything for the Ref<SoundConfig> at runtime
        //if (auto* audioComponent = engine.GetAudioComponentFromID(engine.m_CurrentSceneID, audioComponetnID))
            //audioComponent->SoundConfig.MasterReverbSend = sendLevel;
    }
}

float AudioPlayback::GetMasterReverbSend(uint64_t audioComponetnID)
{
	ZONG_PROFILE_FUNC();

    auto& engine = MiniAudioEngine::Get();
    if (auto* sound = engine.GetSoundForAudioComponent(audioComponetnID))
    {
        return SourceManager::GetMasterReverbSendForSource(sound);

        //if (auto* audioComponent = engine.GetAudioComponentFromID(engine.m_CurrentSceneID, audioComponetnID))
            //return audioComponent->SoundConfig.MasterReverbSend;
    }

    return -1.0f;
}
#endif

uint32_t AudioPlayback::PostTrigger(CommandID triggerCommandID, uint64_t audioEntityID)
{
	if (AudioCommandRegistry::DoesCommandExist<TriggerCommand>(triggerCommandID))
	{
		auto& engine = MiniAudioEngine::Get();
		return engine.PostTrigger(triggerCommandID, audioEntityID);
	}
	else
	{
		ZONG_CORE_ERROR("Audio command with ID {0} does not exist!", triggerCommandID);
		return Audio::EventID::INVALID;
	}
}

uint32_t AudioPlayback::PostTriggerFromAC(Audio::CommandID triggerCommandID, uint64_t audioEntityID)
{
	if (audioEntityID == 0)
	{
		ZONG_CORE_ERROR_TAG("Audio", "Invalid Entity ID {0}!", audioEntityID);
		return Audio::EventID::INVALID;
	}

	if (!AudioCommandRegistry::DoesCommandExist<TriggerCommand>(triggerCommandID))
	{
		ZONG_CORE_ERROR("Audio command with ID {0} does not exist!", triggerCommandID);
		return Audio::EventID::INVALID;
	}

	auto& engine = MiniAudioEngine::Get();
	return engine.PostTrigger(triggerCommandID, audioEntityID);
}

uint32_t AudioPlayback::PostTriggerAtLocation(Audio::CommandID triggerID, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
{
	if (AudioCommandRegistry::DoesCommandExist<TriggerCommand>(triggerID))
	{
		auto& engine = MiniAudioEngine::Get();

		// Create Audible Entity.
		Entity newAudioEntity = engine.GetCurrentSceneContext()->CreateEntity("Ambient Sound");

		const UUID audioEntityID = newAudioEntity.GetUUID();
		//engine.InitTransform(audioEntityID, location);

		// Set flag to destroy Entity when audio finishes.
		auto& audioComponent = newAudioEntity.AddComponent<AudioComponent>();
		audioComponent.bAutoDestroy = true;

		// Set initial position of the Entity
		auto& transformComponent = newAudioEntity.GetComponent<TransformComponent>();
		transformComponent.Translation = position;
		transformComponent.SetRotationEuler(rotation);
		transformComponent.Scale = scale;

		// Post trigger on the AudioComponent.
		return engine.PostTrigger(triggerID, audioEntityID);
	}
	else
	{
		return Audio::EventID::INVALID;
	}
}

void AudioPlayback::SetParameterFloat(Audio::CommandID parameterID, uint64_t audioEntityID, float value)
{
	auto& engine = MiniAudioEngine::Get();
	engine.SetParameterFloat(parameterID, audioEntityID, value);
}

void AudioPlayback::SetParameterInt(Audio::CommandID parameterID, uint64_t audioEntityID, int value)
{
	auto& engine = MiniAudioEngine::Get();
	engine.SetParameterInt(parameterID, audioEntityID, value);
}

void AudioPlayback::SetParameterBool(Audio::CommandID parameterID, uint64_t audioEntityID, bool value)
{
	auto& engine = MiniAudioEngine::Get();
	engine.SetParameterBool(parameterID, audioEntityID, value);
}

void AudioPlayback::SetParameterFloatForAC(Audio::CommandID parameterID, uint64_t audioEntityID, float value)
{
	if (audioEntityID)
	{
		auto& engine = MiniAudioEngine::Get();
		return engine.SetParameterFloat(parameterID, audioEntityID, value);
	}
	else
	{
		ZONG_CORE_ERROR_TAG("Audio", "Invalid Entity ID {0}!", audioEntityID);
		return;
	}
}

void AudioPlayback::SetParameterIntForAC(Audio::CommandID parameterID, uint64_t audioEntityID, int value)
{
	if (audioEntityID)
	{
		auto& engine = MiniAudioEngine::Get();
		return engine.SetParameterInt(parameterID, audioEntityID, value);
	}
	else
	{
		ZONG_CORE_ERROR_TAG("Audio", "Invalid Entity ID {0}!", audioEntityID);
		return;
	}
}

void AudioPlayback::SetParameterBoolForAC(Audio::CommandID parameterID, uint64_t audioEntityID, bool value)
{
	if (audioEntityID)
	{
		auto& engine = MiniAudioEngine::Get();
		return engine.SetParameterBool(parameterID, audioEntityID, value);
	}
	else
	{
		ZONG_CORE_ERROR_TAG("Audio", "Invalid Entity ID {0}!", audioEntityID);
		return;
	}
}

void AudioPlayback::SetParameterFloat(Audio::CommandID parameterID, uint32_t eventID, float value)
{
	auto& engine = MiniAudioEngine::Get();
	engine.SetParameterFloat(parameterID, (Audio::EventID)eventID, value);
}

void AudioPlayback::SetParameterInt(Audio::CommandID parameterID, uint32_t eventID, int value)
{
	auto& engine = MiniAudioEngine::Get();
	engine.SetParameterInt(parameterID, (Audio::EventID)eventID, value);
}

void AudioPlayback::SetParameterBool(Audio::CommandID parameterID, uint32_t eventID, bool value)
{
	auto& engine = MiniAudioEngine::Get();
	engine.SetParameterBool(parameterID, (Audio::EventID)eventID, value);
}

bool AudioPlayback::StopEventID(uint32_t playingEvent)
{
	auto& engine = MiniAudioEngine::Get();
	return engine.StopEventID(playingEvent);
}

bool AudioPlayback::PauseEventID(uint32_t playingEvent)
{
	auto& engine = MiniAudioEngine::Get();
	return engine.PauseEventID(playingEvent);
}

bool AudioPlayback::ResumeEventID(uint32_t playingEvent)
{
	auto& engine = MiniAudioEngine::Get();
	return engine.ResumeEventID(playingEvent);
}

void AudioPlayback::PreloadEventSources(Audio::CommandID eventID)
{
	MiniAudioEngine::ExecuteOnAudioThread([eventID] { MiniAudioEngine::Get().GetResourceManager()->PreloadEventSources(eventID); });
}
void AudioPlayback::UnloadEventSources(Audio::CommandID eventID)
{
	MiniAudioEngine::ExecuteOnAudioThread([eventID] { MiniAudioEngine::Get().GetResourceManager()->UnloadEventSources(eventID); });
}


void AudioPlayback::SetLowPassFilterValueObj(uint64_t audioEntityID, float value)
{
	MiniAudioEngine::Get().SetLowPassFilterValueObj(audioEntityID, value);
}

void AudioPlayback::SetHighPassFilterValueObj(uint64_t audioEntityID, float value)
{
	MiniAudioEngine::Get().SetLowPassFilterValueObj(audioEntityID, value);
}

void AudioPlayback::SetLowPassFilterValue(uint32_t eventID, float value)
{
	MiniAudioEngine::Get().SetLowPassFilterValue(eventID, value);
}

void AudioPlayback::SetHighPassFilterValue(uint32_t eventID, float value)
{
	MiniAudioEngine::Get().SetHighPassFilterValue(eventID, value);
}

void AudioPlayback::SetLowPassFilterValueAC(uint64_t audioEntityID, float value)
{
	if (audioEntityID)
	{
		auto& engine = MiniAudioEngine::Get();
		engine.SetLowPassFilterValueObj(audioEntityID, value);
	}
	else
	{
		ZONG_CORE_ERROR_TAG("Audio", "Invalid Entity ID {0}!", audioEntityID);
		return;
	}
}

void AudioPlayback::SetHighPassFilterValueAC(uint64_t audioEntityID, float value)
{
	if (audioEntityID)
	{
		auto& engine = MiniAudioEngine::Get();
		engine.SetLowPassFilterValueObj(audioEntityID, value);
	}
	else
	{
		ZONG_CORE_ERROR_TAG("Audio", "Invalid Entity ID {0}!", audioEntityID);
		return;
	}
}

}

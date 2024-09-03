#pragma once

#include "Sound.h"
#include "AudioEvents/CommandID.h"

namespace Hazel
{
	struct SoundSourceUpdateData
	{
		uint64_t entityID;
		Audio::Transform Transform;
		glm::vec3 Velocity;
		float VolumeMultiplier;
		float PitchMultiplier;
	};


	/*  =========================================================================
		Entity Component, contains data to initialize playback instances

		Playback of Sound Sources controlled via association with AudioComponent
		------------------------------------------------------------------------
	*/
	struct AudioComponent
	{
		UUID ParentHandle;

		std::string StartEvent;
		Audio::CommandID StartCommandID; // Internal

		/* If set to 'true', AudioEngine will initialize and play SoundObject for this AudioComponent
		   when the AudioComponent gets enabled (temp: when a scene containing instance of this AC starts).
		*/
		bool bPlayOnAwake = false;
		bool bStopWhenEntityDestroyed = true;
		
		////bool bLooping = false;

		/* Overrides muliplier of the SoundConfig. */
		float VolumeMultiplier = 1.0f;
		float PitchMultiplier = 1.0f;

		/* Used for "one-shots". If set to 'true', owning entity is destroyed when playback finished! */
		bool bAutoDestroy = false;

		AudioComponent() = default;
		explicit AudioComponent(UUID parent) : ParentHandle(parent) {}
	};

} // namespace Hazel

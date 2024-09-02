using System.Runtime.InteropServices;

namespace Hazel
{
    using ParameterID = AudioCommandID;

	/// <summary> <c>Sound2D</c> is just a basic helper class 
	/// to easily spawn and controll simple 2D sound objects.
	/// </summary>
	public class AudioEntity : Entity
	{
		internal protected AudioEntity(ulong entityID) : base(entityID)
		{
			if (!m_Entity.HasComponent<AudioComponent>())
				m_AudioComponent = m_Entity.CreateComponent<AudioComponent>();
		}

		protected Entity m_Entity;
		private AudioComponent m_AudioComponent;

		public float VolumeMultiplier
		{
			get => m_AudioComponent.VolumeMultiplier;
			set => m_AudioComponent.VolumeMultiplier = value;
		}

		public float PitchMultiplier
		{
			get => m_AudioComponent.PitchMultiplier;
			set => m_AudioComponent.PitchMultiplier = value;
		}

		public bool IsPlaying() => m_AudioComponent.IsPlaying();
		public bool Play(float startTime = 0.0f) => m_AudioComponent.Play(startTime);
		public bool Stop() => m_AudioComponent.Stop();
		public bool Pause() => m_AudioComponent.Pause();
		public bool Resume() => m_AudioComponent.Resume();
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct AudioCommandID
	{
		public readonly uint ID;

		public AudioCommandID(string commandName)
		{
			ID = InternalCalls.AudioCommandID_Constructor(commandName);
		}

		public static bool operator ==(AudioCommandID c1, AudioCommandID c2) => c1.ID == c2.ID;
		public static bool operator !=(AudioCommandID c1, AudioCommandID c2) => c1.ID != c2.ID;
		public override bool Equals(object c) => ID == ((AudioCommandID)c).ID;
		public override int GetHashCode() => ID.GetHashCode();
		public static implicit operator uint(AudioCommandID commandID) => commandID.ID;
	}

	public static class Audio
	{
		/// <summary> Post event on an object.
		/// <returns>Returns active Event ID
		/// </returns></summary>
		public static uint PostEvent(AudioCommandID id, ulong objectID) => InternalCalls.Audio_PostEvent(id, objectID);

		/// <summary> Post event by name. Prefer overload that takes CommandID for speed and initialize CommandIDs once.
		/// This creates new CommandID object from eventName, which has to hash the string.
		/// </summary>
		public static uint PostEvent(string eventName, ulong objectID) => PostEvent(new AudioCommandID(eventName), objectID);

		//public static uint PostEvent(string eventName, AudioObject audioObj) => PostEvent(new AudioCommandID(eventName), audioObj.ID);

		/// <summary> Post event on AudioComponent.
		/// <returns>Returns active Event ID
		/// </returns></summary>
		public static uint PostEvent(AudioCommandID id, ref AudioComponent audioComponent) => InternalCalls.Audio_PostEventFromAC(id, audioComponent.Entity.ID);

		/// <summary> Post event on at location creating temporary <c>Audio Object</c>.
		/// <returns>Returns active Event ID
		/// </returns></summary>
		public static uint PostEventAtLocation(AudioCommandID id, Vector3 location, Vector3 rotation)
		{
			var spawnLocation = new Transform { Position = location,
												Rotation = rotation,
												Scale = new Vector3(1.0f, 1.0f, 1.0f) };
			return InternalCalls.Audio_PostEventAtLocation(id, ref spawnLocation);
		}

		/// <summary> Stop playing audio sources associated to active event.
		/// <returns>Returns true - if event has playing audio soruces to stop.
		/// </returns></summary>
		public static bool StopEventID(uint eventID) => InternalCalls.Audio_StopEventID(eventID);

		/// <summary> Pause playing audio sources associated to active event.
		/// <returns>Returns true - if event has playing audio soruces to pause.
		/// </returns></summary>
		public static bool PauseEventID(uint eventID) => InternalCalls.Audio_PauseEventID(eventID);

        /// <summary> Resume playing audio sources associated to active event.
        /// <returns>Returns true - if event has playing audio soruces to resume.
        /// </returns></summary>
        public static bool ResumeEventID(uint eventID) => InternalCalls.Audio_ResumeEventID(eventID);
        

        //============================================================================================
        /// Audio Parameters Interface
        
        /// <summary> Set parameter on an object.
        /// <returns>Returns nothing yet.
        /// </returns></summary>
        public static void SetParameter(ParameterID id, ulong objectID, float value) => InternalCalls.Audio_SetParameterFloat((uint)id.GetHashCode(), objectID, value);
        
        public static void SetParameterForAC(ParameterID id, ref AudioComponent audioComponent, float value) => InternalCalls.Audio_SetParameterFloatForAC((uint)id.GetHashCode(), audioComponent.Entity.ID, value);

        public static void SetParameter(ParameterID id, ulong objectID, int value) => InternalCalls.Audio_SetParameterInt((uint)id.GetHashCode(), objectID, value);

        public static void SetParameterForAC(ParameterID id, ref AudioComponent audioComponent, int value) => InternalCalls.Audio_SetParameterIntForAC((uint)id.GetHashCode(), audioComponent.Entity.ID, value);

        public static void SetParameter(ParameterID id, ulong objectID, bool value) => InternalCalls.Audio_SetParameterBool((uint)id.GetHashCode(), objectID, value);

        public static void SetParameterForAC(ParameterID id, ref AudioComponent audioComponent, bool value) => InternalCalls.Audio_SetParameterBoolForAC((uint)id.GetHashCode(), audioComponent.Entity.ID, value);

        public static void SetParameter(ParameterID id, uint eventID, float value) => InternalCalls.Audio_SetParameterFloatForEvent((uint)id.GetHashCode(), eventID, value);

        public static void SetParameter(ParameterID id, uint eventID, int value) => InternalCalls.Audio_SetParameterIntForEvent((uint)id.GetHashCode(), eventID, value);

        public static void SetParameter(ParameterID id, uint eventID, bool value) => InternalCalls.Audio_SetParameterBoolForEvent((uint)id.GetHashCode(), eventID, value);

        //============================================================================================

		public static AudioEntity CreateAudioEntity(AudioCommandID triggerEventID, Transform location, float volume = 1.0f, float pitch = 1.0f)
		{
			return new AudioEntity(InternalCalls.Audio_CreateAudioEntity(triggerEventID, ref location, volume, pitch));
		}
		 
		public static AudioEntity CreateAudioEntity(string triggerEventName, Transform location, float volume = 1.0f, float pitch = 1.0f)
		{
			return CreateAudioEntity(new AudioCommandID(triggerEventName), location, volume, pitch);
		}

		//============================================================================================
		public static void PreloadEventSources(AudioCommandID eventID ) => InternalCalls.Audio_PreloadEventSources(eventID);
		public static void UnloadEventSources(AudioCommandID eventID) => InternalCalls.Audio_UnloadEventSources(eventID);


		public static void SetLowPassFilterValueObj(ulong objectID, float value) => InternalCalls.Audio_SetLowPassFilterValue(objectID, value);
		public static void SetHighPassFilterValueObj(ulong objectID, float value) => InternalCalls.Audio_SetHighPassFilterValue(objectID, value);


		public static void SetLowPassFilterValue(ref AudioComponent audioComponent, float value) => InternalCalls.Audio_SetLowPassFilterValue_AC(audioComponent.Entity.ID, value);
		public static void SetHighPassFilterValue(ref AudioComponent audioComponent, float value) => InternalCalls.Audio_SetHighPassFilterValue_AC(audioComponent.Entity.ID, value);

		public static void SetLowPassFilterValue(uint eventID, float value) => InternalCalls.Audio_SetLowPassFilterValue_Event(eventID, value);
		public static void SetHighPassFilterValue(uint eventID, float value) => InternalCalls.Audio_SetLowPassFilterValue_Event(eventID, value);
	}
}

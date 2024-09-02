using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Hazel;


namespace AudioDemo
{
	class DopplerCube : Entity
	{
		private AudioComponent m_AudioComponent;
		private RigidBodyComponent m_RigidBody;

		private ulong frameCount = 0;

		public int numberOfLoops = 3;
		public float amplitude = 300.0f;
		public float slope = 8.0f;
		public int xAxis = 0;

		protected override void OnCreate()
		{
			m_AudioComponent = GetComponent<AudioComponent>();
			m_RigidBody = GetComponent<RigidBodyComponent>();
		}

		protected override void OnUpdate(float ts)
		{
			frameCount++;

			if(numberOfLoops > 0 || numberOfLoops == -1)
			{
				double x = frameCount / 600.0;
				double steepSine = Math.Atan(slope * (Math.Sin(x * 2 * Math.PI))) / Math.Atan(slope) * (ts * 100.0f);
				float velocity = (float)steepSine * amplitude;
				velocity = Mathf.Clamp(velocity, -100.0f, 100.0f);

				if (xAxis == 0)
					m_RigidBody.LinearVelocity = new Vector3(0.0f, 0.0f, velocity);
				else
					m_RigidBody.LinearVelocity = new Vector3(-velocity, 0.0f, 0.0f);

				if (steepSine >= 1.0 && numberOfLoops != -1)
					--numberOfLoops;

				//Console.WriteLine((float)steepSine * amplitude);
			}
			else if(m_AudioComponent.IsPlaying())
			{
				m_AudioComponent.Stop();
			}
		}
	}

	class SoundCone : Entity
	{
		//private SimpleSound[] soundAssets = new SimpleSound[7];
		private AudioComponent m_AudioComponent;
		private Vector3 m_StartingPos;
		private long frameCount = 0;

		protected override void OnCreate()
		{
			/*soundAssets[0] = new SimpleSound("audio/AudioDemo/short one-shot/weird sfx-04.wav");
			soundAssets[1] = new SimpleSound("audio/AudioDemo/short one-shot/weird sfx-07-1.wav");
			soundAssets[2] = new SimpleSound("audio/AudioDemo/short one-shot/weird sfx-07-2.wav");
			soundAssets[3] = new SimpleSound("audio/AudioDemo/short one-shot/weird sfx-09-1.wav");
			soundAssets[4] = new SimpleSound("audio/AudioDemo/short one-shot/weird sfx-14-1.wav");
			soundAssets[5] = new SimpleSound("audio/AudioDemo/short one-shot/weird sfx-14-2.wav");
			soundAssets[6] = new SimpleSound("audio/AudioDemo/short one-shot/weird sfx-18.wav");*/


			m_AudioComponent = GetComponent<AudioComponent>();
			m_StartingPos = Transform.Translation;
		}

		float amplitude = 2.82f;

		float angle = 0.0f;
		float speed = (2.0f * (float)Math.PI) / 1.0f;
		float radius = 1.5f;

		protected override void OnUpdate(float ts)
		{
			frameCount++;

			{
				double t = frameCount / 100.0;
				float sine = ((float)Math.Sin(t * (0.5f * Math.PI)) + 1.0f) / 2.0f;

				angle += (float)speed * ts;

				float x = (float)Math.Cos(angle) * (radius * (1.0f - sine)) + m_StartingPos.X;
				float z = (float)Math.Sin(angle) * (radius * (1.0f - sine)) + m_StartingPos.Z;

				Transform.Translation = new Vector3(x, sine * amplitude + m_StartingPos.Y, z);

				m_AudioComponent.PitchMultiplier = 0.5f + sine * 1.5f;
				m_AudioComponent.VolumeMultiplier = 1.0f - (float)Math.Pow(sine, 3.0);

				if(sine == 1.0f)
				{
					float rv = Hazel.Random.Float() * 0.2f;
					float rp = Hazel.Random.Float() * 0.4f;
					var spawnLocation = Transform.Translation;
					//Audio.PlaySoundAtLocation(soundAssets[rand.Next(soundAssets.Length)], spawnLocation, 0.5f - rv, 1.0f + rp);
					Audio.PostEventAtLocation(new AudioCommandID("play_sound_cone_top"), spawnLocation, Transform.Rotation);
				}

			}
		}
	}

	class TriggerVolume : Entity 
	{
		//private SimpleSound[] sounds = new SimpleSound[14];

		protected override void OnCreate()
		{
			TriggerBeginEvent += OnPlayerTriggerBegin;
			TriggerEndEvent += OnPlayerTriggerEnd;

			xBounds = 11.0f;
			zBounds = 11.0f;

			for(int i = 0; i < 14; ++i)
			{
				string filepath;
				if(i <9)
				{
					filepath = String.Format("audio/AudioDemo/random one-shots/Machine one-shot-0{0} - mono.wav", i + 1);
					//sounds[i] = new SimpleSound(filepath);
				}
				else
				{
					filepath = String.Format("audio/AudioDemo/random one-shots/Machine one-shot-{0} - mono.wav", i + 1);
					//sounds[i] = new SimpleSound(filepath);
				}
			}

		}
		private float xBounds;
		private float zBounds;

		void PlaySound()
		{
			float x = Hazel.Random.Float() * xBounds - 0.5f * xBounds + Transform.Translation.X;
			float z = Hazel.Random.Float() * zBounds - 0.5f * zBounds + Transform.Translation.Z;

			float rv = Hazel.Random.Float() * 0.2f;
			float rp = Hazel.Random.Float() * 0.4f;
			var spawnLocation = new Vector3(x, 0.5f, z);

			//Audio.PlaySoundAtLocation(sounds[rand.Next(sounds.Length)], spawnLocation, 1.2f - rv, 1.0f + rp);
			Audio.PostEventAtLocation(new AudioCommandID("play_sound_area"), spawnLocation, Transform.Rotation);
		}

		void OnPlayerTriggerBegin(Entity other)
		{
			PlaySound();
		}

		void OnPlayerTriggerEnd(Entity other)
		{
		}

		protected override void OnUpdate(float ts)
		{

		}
	}

	class OneShotTrigger : Entity
	{
		private string[] assetPaths = new string[]{ "audio/AudioDemo/sequence one-shots/Machine Language-06.wav",
													"audio/AudioDemo/sequence one-shots/Machine Language-07.wav",
													"audio/AudioDemo/sequence one-shots/Machine Language-03.wav",
													"audio/AudioDemo/sequence one-shots/Machine Language-33.wav",
													"audio/AudioDemo/sequence one-shots/Machine Language-28.wav",
													"audio/AudioDemo/sequence one-shots/Machine Language-36.wav",
													"audio/AudioDemo/sequence one-shots/Machine Language-38.wav",
													"audio/AudioDemo/sequence one-shots/Machine Language-76.wav" };
		private AudioComponent m_AudioComponent;

		private float startingPitch;
		private float startingVolume;
		private int soundIndex = 0;

		protected override void OnCreate()
		{
			TriggerBeginEvent += OnPlayerTriggerBegin;
			TriggerEndEvent += OnPlayerTriggerEnd;

			m_AudioComponent = GetComponent<AudioComponent>();
			startingPitch = m_AudioComponent.PitchMultiplier;
			startingVolume = m_AudioComponent.VolumeMultiplier;

		}

		void PlaySound()
		{
			float rv = Hazel.Random.Float() * 0.2f;
			float rp = Hazel.Random.Float() * 0.06f;

			m_AudioComponent.VolumeMultiplier = startingVolume + rv;
			m_AudioComponent.PitchMultiplier = startingPitch + rp;

			//m_AudioComponent.SetSound(assetPaths[soundIndex++]);
			m_AudioComponent.SetEvent("play_sound_sphere");
			/*if (soundIndex >= assetPaths.Length)
				soundIndex = assetPaths.Length - 1;*/

			m_AudioComponent.Play();
		}

		void OnPlayerTriggerBegin(Entity other)
		{
			if(!m_AudioComponent.IsPlaying())
				PlaySound();
		}

		void OnPlayerTriggerEnd(Entity other)
		{
		}

		protected override void OnUpdate(float ts)
		{

		}
	}
}

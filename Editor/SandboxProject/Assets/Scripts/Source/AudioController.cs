using System;
using System.Collections.Generic;
using Hazel;


namespace AudioTest
{
	using ParameterID = AudioCommandID;

	public class AudioController : Entity
	{
		private AudioComponent m_AudioComponent;
		//private SimpleSound sound = new SimpleSound("assets/audio/LR test.wav");
		//private SimpleSound sound = new SimpleSound("audio/test tone 110Hz -6dB.wav");
		private RigidBodyComponent m_RigidBody;
		private TransformComponent m_TransformComponent;

		private AudioEntity m_SpawnedSound3D;

		private float cooldown = 0.0f;
		public float interval;
		public int playCount;

		private long frameCount = 0;

		private float fallLerp = 0.0f;

		Vector3 startingPosition;

		protected override void OnCreate()
		{
			Console.WriteLine("On Create AudioController");
			m_AudioComponent = GetComponent<AudioComponent>();
			m_RigidBody = GetComponent<RigidBodyComponent>();
			m_TransformComponent = GetComponent<TransformComponent>();
			startingPosition = m_TransformComponent.Translation;
			//interval = 4.0f;
			cooldown = interval;
			//Audio.PlaySound2D("assets/audio/LR test.wav", 1.0f, 0.6f);
			//m_AudioComponent.Play();
			//m_SpawnedSound3D = Audio.CreateSoundAtLocation("assets/audio/LR test.wav", new Vector3(0.0f), 1.0f, 0.9f);
			//m_SpawnedSound3D = Audio.CreateSoundAtLocation(sound, new Vector3(0.0f), 1.0f, 2.0f);
		}

		private float Lerp(float firstFloat, float secondFloat, float by)
		{
			return firstFloat * (1 - by) + secondFloat * by;
		}

		protected override void OnUpdate(float ts)
		{
			frameCount++;

			var transform = GetComponent<TransformComponent>();

			float amplitude = 300.0f;
			/*{
				double x = frameCount / 100.0;
				double sine = Math.Sin(x * (0.5 * Math.PI));

				m_RigidBody.SetLinearVelocity(new Vector3(-(float)sine * amplitude, 0.0f, 0.0f));
			}*/
			
			{
				double x = frameCount / 600.0;
				double steepness = 8.0;
				double steepSine = Math.Atan(steepness * (Math.Sin(x * 2* Math.PI))) / Math.Atan(steepness);

				m_RigidBody.LinearVelocity = new Vector3(-(float)steepSine * amplitude, 0.0f, 0.0f);
			}


			//Console.WriteLine("x: {0}", m_TransformComponent.Translation.X);
			cooldown -= ts;

			// Controlling playback of Sound Source associated with AudioComponent
			/*
			if(m_AudioComponent.IsPlaying())
			{
				if (cooldown <= 0.0f)
				{
					//m_AudioComponent.Stop();
					cooldown = interval;
					Console.WriteLine("Component Paused");

				}
			}
			else
			{
				if (cooldown <= 0.0f)
				{
					//m_AudioComponent.Play();
					cooldown = interval;
					Console.WriteLine("Component Resumed");

				}

			}*/

			/*if(cooldown > 0.0f)
			{
				// Falling sound
				//Console.WriteLine("cooldown: {0}", cooldown);
			}
			if (!m_SpawnedSound3D.IsPlaying() && cooldown <= 0.0f)
			{
				cooldown = 0.0f;
				m_SpawnedSound3D.Play();
			}

			//m_SpawnedSound3D.Location = new Vector3((float)factor * amplitude, 0.0f, 0.0f);
			//m_SpawnedSound3D.PitchMultiplier = 4.5f + (float)factor * 0.5f;

			fallLerp += ts;
			float newY = Lerp(startingPosition.Y, -100.0f, fallLerp / 3.0f);
			//m_SpawnedSound3D.Location = new Vector3(newY, 0.0f, 0.0f);
			//Console.WriteLine("Y: {0}", m_SpawnedSound3D.Location.X);
			*/

			// Spawning one-shots
			/*if ((playCount > 0 || playCount == -1) && cooldown <= 0.0f)
			{
				var rand = new Random();

				double pitchMagnitude = 0.5;
				float pitchOffset = (float)(rand.NextDouble() * (pitchMagnitude * 2.0) - pitchMagnitude);


				Audio.PlaySoundAtLocation(sound, Translation, 1.0f, 4.0f + (float)pitchMagnitude + pitchOffset);
				cooldown = interval;
				if(playCount != -1)
					playCount--;
			}*/
		}
	}

	public class ReverbTest : Entity
	{
		private AudioComponent m_AudioComponent;

		private long frameCount = 0;

		protected override void OnCreate()
		{
			Console.WriteLine("OnCreate. ReverbTest");
			m_AudioComponent = GetComponent<AudioComponent>();
		}

		protected override void OnUpdate(float ts)
		{
			frameCount++;
			double x = frameCount / 100.0;
			double sine = Math.Sin((ts * 100.0f / 10.0f) * x * (0.5 * Math.PI));
			//m_AudioComponent.MasterReverbSend = (float)(sine + 1.0) / 2.0f;
			//Console.WriteLine("Rev send: {0:0.##}", m_AudioComponent.MasterReverbSend);
		}

	}

	public class EventsTest : Entity
	{
		public float interval = 1.0f;
		private float coolDown = 1.0f;
		private long frameCount = 0;

		private readonly AudioCommandID playSoundID = new AudioCommandID("play_sound");
		private readonly AudioCommandID stopSoundID = new AudioCommandID("stop_sound");
		private readonly AudioCommandID pauseSoundID = new AudioCommandID("pause_sound");
		private readonly AudioCommandID resumeSoundID = new AudioCommandID("resume_sound");

		private readonly AudioCommandID stopAllOnObject = new AudioCommandID("stop_object");

		private ulong audioObjectID = 0;

		private List<AudioCommandID> eventSequence = new List<AudioCommandID>();

		private Vector3 m_StartingPosition;
		private RigidBodyComponent m_RigidBody;
		private AudioComponent m_AudioComponent;

		private bool flip = true;
		private int count = 1;

		~EventsTest()
		{
		}


		private void InitializeEventsTest()
		{
			Console.WriteLine("[Events Test] Initializing.");

			eventSequence.Add(new AudioCommandID("play_test_01"));
			eventSequence.Add(new AudioCommandID("play_test_02"));
			eventSequence.Add(new AudioCommandID("play_test_03"));
			eventSequence.Add(new AudioCommandID("play_test_04"));
			eventSequence.Add(new AudioCommandID("play_test_05"));
		}

		private void EventsTestRun(int playCount)
		{
			Console.WriteLine("[Events Test] Starting.");

			for (int i = 0; i < playCount; ++i)
			{
				//Console.WriteLine("[Events Test] Posting event number: {0}", i);
				Audio.PostEvent(eventSequence[i % eventSequence.Count], ref m_AudioComponent);
			}

			Console.WriteLine("[Events Test] All events posted.");
		}

		protected override void OnCreate()
		{
			//Console.WriteLine("OnCreate. EventsTest");
			//Console.WriteLine("ID: {0}", audioObjectID);
			//var transform = GetComponent<TransformComponent>().Transform;

			m_AudioComponent = GetComponent<AudioComponent>();

			//m_AudioComponent.Play();

			/*if(audioObjectID == 0)
			{
				Console.WriteLine("OnCreate. Initializing Object");
				audioObject = Audio.InitializeAudioObject("GenericName", transform);
				audioObjectID = audioObject.ID;
			}
			else
			{
				Console.WriteLine("Object already initialized");
				audioObject = Audio.GetObject(audioObjectID);
			}

			Console.WriteLine("Object position: {0}", transform.Position);
			if(audioObjectID != 0)
			{

				Console.WriteLine("PostEvent: play_sound, objectID: {0}", audioObjectID);
				Audio.PostEvent(playSoundID, audioObjectID);
			}*/

			coolDown = interval;
			m_StartingPosition = Transform.Translation;
			//m_RigidBody = GetComponent<RigidBodyComponent>();

			InitializeEventsTest();
		}

		protected override void OnUpdate(float ts)
		{
			coolDown -= ts;
			if (coolDown <= 0.0f)
			{
				if(count >= 0)
				{ 
					if(flip)
					{
						//m_AudioComponent.Play();
						EventsTestRun(65);

						flip = !flip;
					}
					else
					{
						//Audio.PostEvent(stopAllOnObject, ref m_AudioComponent);
						
					}
					count--;
				}
				

				coolDown = interval;
			}


			frameCount++;
			{
				double t = frameCount * ts / 1.0f;
				float sine = (float)Math.Sin(t * (0.5f * Math.PI));
				float amplitude = 16.0f;

				var velocity = new Vector3(sine * amplitude, 0.0f, 0.0f);
				//m_RigidBody.SetLinearVelocity(velocity);

				//m_AudioComponent.VolumeMultiplier = 1.0f + sine * 0.5f;
				//m_AudioComponent.PitchMultiplier = 1.0f + sine * 0.5f;
				//audioObject.SetTransform(GetComponent<TransformComponent>().Transform);
			}
		}
	}

	public class Debug : Entity
	{
		public float interval = 1.0f;
		private float coolDown = 1.0f;
		private long frameCount = 0;
		private float timeCount = 0.0f;

		private readonly AudioCommandID playSoundID = new AudioCommandID("play_sound");
		private readonly AudioCommandID stopSoundID = new AudioCommandID("stop_sound");
		private readonly AudioCommandID pauseSoundID = new AudioCommandID("pause_sound");
		private readonly AudioCommandID resumeSoundID = new AudioCommandID("resume_sound");
		private ulong audioObjectID = 0;

		private List<AudioCommandID> eventSequence = new List<AudioCommandID>();

		private Vector3 m_StartingPosition;
		private RigidBodyComponent m_RigidBody;
		private AudioComponent m_AudioComponent;

		private bool flip = true;
		private int count = 1;

		private uint eventID;

		public float TimeMultiplier = 10.0f;
		public float SpeedMultiplier = 1.0f;

		~Debug()
		{
		}


		private void InitializeEventsTest()
		{
			Console.WriteLine("[Events Test] Initializing.");

			eventSequence.Add(new AudioCommandID("play_test_01"));
			eventSequence.Add(new AudioCommandID("play_test_02"));
			eventSequence.Add(new AudioCommandID("play_test_03"));
			eventSequence.Add(new AudioCommandID("play_test_04"));
			eventSequence.Add(new AudioCommandID("play_test_05"));
		}

		private void EventsTestRun(int playCount)
		{
			Console.WriteLine("[Events Test] Starting.");

			for (int i = 0; i < playCount; ++i)
			{
				//Console.WriteLine("[Events Test] Posting event number: {0}", i);
				Audio.PostEvent(eventSequence[i % eventSequence.Count], ref m_AudioComponent);
			}

			Console.WriteLine("[Events Test] All events posted.");
		}

		protected override void OnCreate()
		{
			//Console.WriteLine("OnCreate. EventsTest");
			//Console.WriteLine("ID: {0}", audioObjectID);

			m_AudioComponent = GetComponent<AudioComponent>();
			//m_AudioComponent.Play();
			//eventID = Audio.PostEvent(new Audio.CommandID("play_sound2"), ref m_AudioComponent);

			coolDown = interval;
			m_StartingPosition = Transform.Translation;
			//m_RigidBody = GetComponent<RigidBodyComponent>();

			//InitializeEventsTest();
		}

		protected override void OnUpdate(float ts)
		{
			coolDown -= ts;
			if (coolDown <= 0.0f)
			{
				if (count >= 0)
				{
					if (flip)
					{
						//m_AudioComponent.Stop();
						//EventsTestRun(12);
						//Audio.StopEventID(eventID);

						flip = !flip;
					}
					else
					{
						//Audio.PostEvent(stopSoundID, ref m_AudioComponent);

					}
					count--;
				}


				coolDown = interval;
			}


			frameCount++;
			timeCount += ts;
			{
				//double t = frameCount * ts / TimeMultiplier;
				double t = timeCount / TimeMultiplier;
				float sine = (float)Math.Sin(t * (0.5f * Math.PI));
				float amplitude = SpeedMultiplier * ts;

				var velocity = new Vector3(sine * amplitude, 0.0f, 0.0f);
				//m_RigidBody.SetLinearVelocity(velocity);

				//m_AudioComponent.VolumeMultiplier = 1.0f + sine * 0.5f;
				//m_AudioComponent.PitchMultiplier = 1.0f + sine * 0.5f;
				//audioObject.SetTransform(GetComponent<TransformComponent>().Transform);

				Transform.Translation += velocity;
			}
		}
	}

	public class SpreadTest : Entity
	{
		private Vector3 m_StartingPosition;

		private Entity m_Player;
		private Entity LeftChannel;
		private Entity RightChannel;

		public float SourceSize = 3.0f;
		private float m_Spread = 3.0f;

		private TransformComponent m_TransformComponent;

		private RigidBodyComponent m_LRB;
		private RigidBodyComponent m_RRB;

		protected override void OnCreate()
		{
			LeftChannel = Scene.FindEntityByTag("LeftChannel");
			RightChannel = Scene.FindEntityByTag("RightChannel");

			m_LRB = LeftChannel.GetComponent<RigidBodyComponent>();
			m_RRB = RightChannel.GetComponent<RigidBodyComponent>();
			
			m_TransformComponent = GetComponent<TransformComponent>();

			m_Player = Scene.FindEntityByTag("Player");

		}

		float VectorAngle(float x, float y)
		{
			if (x == 0) // special cases
				return (y > 0) ? 90
					: (y == 0) ? 0
					: 270;
			else if (y == 0) // special cases
				return (x >= 0) ? 0
					: 180;
			float ret = Mathf.Rad2Deg * (float)(Math.Atan(y / x));
			if (x < 0 && y < 0) // quadrant Ⅲ
				ret = 180 + ret;
			else if (x < 0) // quadrant Ⅱ
				ret = 180 + ret; // it actually substracts
			else if (y < 0) // quadrant Ⅳ
				ret = 270 + (90 + ret); // it actually substracts
			return ret;
		}

		Vector3 GetPositionOnCircumference(float angle, float radius)
		{
			float x = radius * Mathf.Cos(angle);
			float z = radius * Mathf.Sin(angle);

			return new Vector3(x, 0.0f, z);
		}

		float GetSpreadFromSourceSize(float sourceSize, float distance)
		{
			if (distance <= 0.0f)
				return 1.0f;

			float degreeSpread = (float)(Math.Atan((0.5f * sourceSize) / distance)) * 2.0f;
			return degreeSpread / Mathf.PI;
		}

		protected override void OnUpdate(float ts)
		{

			var origin = m_TransformComponent.WorldTransform.Position;
			var relativePosition = origin - m_Player.Transform.Translation;

			//m_Spread = GetSpreadFromSourceSize(SourceSize, Vector3.Distance(origin, m_Player.Translation));
			//Console.WriteLine(m_Spread);

			// Find angle between the source and player
			float angle = VectorAngle(relativePosition.X, relativePosition.Z) + 90.0f;

			// Position Left and Right channel spheres facing player
			float radius = SourceSize / 2.0f;
			//m_LRB.Translation = GetPositionOnCircumference(Mathf.Deg2Rad * angle, -radius, Translation);
			//m_RRB.Translation = GetPositionOnCircumference(Mathf.Deg2Rad * angle, radius, Translation);

			LeftChannel.Translation = GetPositionOnCircumference(Mathf.Deg2Rad * angle, -radius) + Translation;
			RightChannel.Translation = GetPositionOnCircumference(Mathf.Deg2Rad * angle, radius) + Translation;

			//m_RigidBody.Rotation = new Vector3(m_RigidBody.Rotation.X, Mathf.Deg2Rad * angle, m_RigidBody.Rotation.Z);

		}
	}
	
	public class SpatializerStress : Entity
	{
		public int EnableTest = 0;

		public int NumberOfObjects;
		public Vector2 SpawnOffset;
		public Vector2 SpawnRange;

		private Entity m_MovingObject;

		private Entity[] m_Objects;
		private float[] m_Rotations;

		private ulong frameCount = 0;
		public float amplitude = 100.0f;
		public float slope = 8.0f;

		protected override void OnCreate()
		{
			m_MovingObject = Scene.FindEntityByTag("_MOVING_OBJ");

			if(EnableTest != 0)
			{ 
				m_Objects = new Entity[NumberOfObjects];
				m_Rotations = new float[NumberOfObjects];

				for (int i = 0; i < NumberOfObjects; i++)
				{
					float x = SpawnOffset.X + Hazel.Random.Float() * SpawnRange.X;
					float y = SpawnOffset.Y + Hazel.Random.Float() * SpawnRange.Y;
					float rotation = Hazel.Random.Float() * 10.0f;
					m_Objects[i] = CreateObject(new Vector2(x, y), rotation);
					m_Rotations[i] = rotation;
				}
			}
		}

		private Entity CreateObject(Vector2 translation, float rotation)
		{
			Entity entity = Scene.CreateEntity();
			var tc = entity.GetComponent<TransformComponent>();
			tc.Translation = Translation + new Vector3(translation.X, translation.Y, 0.0f);
			tc.Rotation = new Vector3(0.0f, 0.0f, rotation);

			var mc = entity.CreateComponent<StaticMeshComponent>();
			mc.Mesh = m_MovingObject.GetComponent<StaticMeshComponent>().Mesh;

			tc.Scale *= 0.5f;
			//var rb = entity.CreateComponent<RigidBodyComponent>();
			//rb.LinearVelocity = tc.Rotation;
			var audio = entity.CreateComponent<AudioComponent>();
			audio.SetEvent("spatializer_stress");
			audio.PitchMultiplier = 1.0f + rotation * 0.5f;
			audio.Play();

			return entity;
		}

		protected override void OnUpdate(float ts)
		{
			if (EnableTest != 0)
			{
				frameCount++;

				double x = frameCount / 200.0;

				for(int i = 0; i < NumberOfObjects; i++)
				{
					var obj = m_Objects[i];
					var rotation = m_Rotations[i];
					var random = rotation / 10.0f;

					double steepSine = Math.Atan(slope * (Math.Sin((x + x * random) * 2 * Math.PI))) / Math.Atan(slope) * ts;
					steepSine *= 0.5f * amplitude + random * amplitude;
					float velocity = (float)steepSine;// * (1.0f + random);// * amplitude;

					obj.Transform.Translation += new Vector3(Mathf.Cos(rotation) * velocity, 0.0f, Mathf.Sin(rotation) * velocity);
				}
			}
		}
	}


	public class ParametersTest : Entity
	{
		public float interval = 1.0f;
		private float coolDown = 1.0f;

		private float volume = 1.0f;

		private readonly ParameterID volumePar = new ParameterID("VolumePar");
		private readonly AudioCommandID play_event = new AudioCommandID("play_soul");
		uint playEventID;

		private AudioComponent m_AudioComponent;
		//private ulong audioObjectID = 0;
		//private Audio.Object audioObject;

		~ParametersTest()
		{
			// TODO: releasing object not by ID, but by object ref crashes mono.
			//		probably because the object doesn't exist by the point this destructor hits
			//Audio.ReleaseAudioObject(audioObjectID);
		}

		protected override void OnCreate()
		{
			m_AudioComponent = GetComponent<AudioComponent>();
			//audioObject = Audio.InitializeAudioObject("ParametersTestObj", GetComponent<TransformComponent>().Transform);
			//audioObjectID = audioObject.ID;
			//Console.WriteLine("Volume change: {0}", volume);

			//playEventID = Audio.PostEvent(play_event, ref m_AudioComponent);

		}
		protected override void OnUpdate(float ts)
		{
			coolDown -= ts;
			if (coolDown <= 0.0f)
			{
				volume += 0.25f;
				if (volume > 1.0f)
					volume -= 1.0f;

				//Audio.SetParameterForAC(volumePar, ref m_AudioComponent, volume);
				Audio.SetParameter(volumePar, playEventID, volume);

				//Console.WriteLine("Volume change: {0}", volume);

				coolDown = interval;
			}
		}

	}
}

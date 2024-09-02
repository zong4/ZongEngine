using Hazel;

//
// Adapted from https://github.com/SebLague/Boids
//

namespace Sandbox
{
	public class BoidManager : Entity
	{
		public BoidSettings m_Settings;
		private Boid[] m_Boids;

		public int SpawnCount = 10;
		public float SpawnRadius = 10;

		public Prefab BoidPrefab;
		public Entity Target;
		
		// public ComputeShader compute;

		protected override void OnCreate()
		{
			m_Settings = new BoidSettings();
			m_Boids = new Boid[SpawnCount];
			Log.Warn("Spawning");

			for (int i = 0; i < SpawnCount; i++)
			{
				Vector3 pos = Translation + (Random.Vec3() * 2.0f - 1.0f).Normalized() * SpawnRadius;
				Boid boid = Instantiate(BoidPrefab).As<Boid>();
				boid.Translation = pos;
				boid.Rotation = (Random.Vec3() * 2.0f - 1.0f).Normalized();

				boid.Initialize(m_Settings, Target);

				m_Boids[i] = boid;
			}
		}

		protected override void OnUpdate(float ts)
		{
			if (m_Boids == null)
				return;


			int numBoids = m_Boids.Length;
			var boidData = new BoidData[numBoids];

			for (int i = 0; i < m_Boids.Length; i++)
			{
				boidData[i].position = m_Boids[i].position;
				boidData[i].direction = m_Boids[i].forward;
			}

#if false // TODO: Compute shaders
		const int threadGroupSize = 1024;


		var boidBuffer = new ComputeBuffer (numBoids, BoidData.Size);
        boidBuffer.SetData (boidData);

        compute.SetBuffer (0, "boids", boidBuffer);
        compute.SetInt ("numBoids", boids.Length);
        compute.SetFloat ("viewRadius", m_Settings.perceptionRadius);
        compute.SetFloat ("avoidRadius", m_Settings.avoidanceRadius);

        int threadGroups = Mathf.CeilToInt (numBoids / (float) threadGroupSize);
        compute.Dispatch (0, threadGroups, 1, 1);

        boidBuffer.GetData (boidData);

        for (int i = 0; i < boids.Length; i++) {
            boids[i].avgFlockHeading = boidData[i].flockHeading;
            boids[i].centreOfFlockmates = boidData[i].flockCentre;
            boids[i].avgAvoidanceHeading = boidData[i].avoidanceHeading;
            boids[i].numPerceivedFlockmates = boidData[i].numFlockmates;

            boids[i].UpdateBoid (ts);
        }

        boidBuffer.Release ();
#endif

			#region Non-compute shader version
			float viewRadius = m_Settings.perceptionRadius;
			float avoidRadius = m_Settings.avoidanceRadius;

			for (int i = 0; i < numBoids; i++)
			{
				for (int indexB = 0; indexB < numBoids; indexB++)
				{
					if (indexB == i)
						continue;

					BoidData boidB = boidData[indexB];
					Vector3 offset = boidB.position - boidData[i].position;
					float sqrDst = offset.X * offset.X + offset.Y * offset.Y + offset.Z * offset.Z;

					if (sqrDst < viewRadius * viewRadius)
					{
						boidData[i].numFlockmates += 1;
						boidData[i].flockHeading += boidB.direction;
						boidData[i].flockCentre += boidB.position;

						if (sqrDst < avoidRadius * avoidRadius)
						{
							boidData[i].avoidanceHeading -= offset / sqrDst;
						}
					}
				}
			}
			#endregion

			for (int i = 0; i < m_Boids.Length; i++)
			{
				m_Boids[i].avgFlockHeading = boidData[i].flockHeading;
				m_Boids[i].centreOfFlockmates = boidData[i].flockCentre;
				m_Boids[i].avgAvoidanceHeading = boidData[i].avoidanceHeading;
				m_Boids[i].numPerceivedFlockmates = boidData[i].numFlockmates;

				m_Boids[i].UpdateBoid(ts);
			}
		}

		public struct BoidData
		{
			public Vector3 position;
			public Vector3 direction;

			public Vector3 flockHeading;
			public Vector3 flockCentre;
			public Vector3 avoidanceHeading;
			public int numFlockmates;

			public static int Size
			{
				get
				{
					return sizeof(float) * 3 * 5 + sizeof(int);
				}
			}
		}
	}
}

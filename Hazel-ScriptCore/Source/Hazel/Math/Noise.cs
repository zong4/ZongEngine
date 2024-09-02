using System;

namespace Hazel
{
	public class Noise
	{
		public Noise(int seed = 8)
		{
			unsafe { m_UnmanagedInstance = InternalCalls.Noise_Constructor(seed); }
		}

		~Noise()
		{
			unsafe { InternalCalls.Noise_Destructor(m_UnmanagedInstance); }
		}

		public float Frequency
		{
			get
			{
				unsafe { return InternalCalls.Noise_GetFrequency(m_UnmanagedInstance); }
			}

			set
			{
				unsafe { InternalCalls.Noise_SetFrequency(m_UnmanagedInstance, value); }
			}
		}

		public int Octaves
		{
			get
			{
				unsafe { return InternalCalls.Noise_GetFractalOctaves(m_UnmanagedInstance); }
			}

			set
			{
				unsafe { InternalCalls.Noise_SetFractalOctaves(m_UnmanagedInstance, value); }
			}
		}

		public float Lacunarity
		{
			get
			{
				unsafe { return InternalCalls.Noise_GetFractalLacunarity(m_UnmanagedInstance); }
			}

			set
			{
				unsafe { InternalCalls.Noise_SetFractalLacunarity(m_UnmanagedInstance, value); }
			}
		}
		
		public float Get(float x, float y)
		{
			unsafe { return InternalCalls.Noise_Get(m_UnmanagedInstance, x, y); }
		}

		public float Gain
		{
			get
			{
				unsafe { return InternalCalls.Noise_GetFractalGain(m_UnmanagedInstance); }
			}

			set
			{
				unsafe { InternalCalls.Noise_SetFractalGain(m_UnmanagedInstance, value); }
			}
		}

		public static int StaticSeed
		{
			set
			{
				unsafe { InternalCalls.Noise_SetSeed(value); }
			}
		}

		internal IntPtr m_UnmanagedInstance;

		public static float Perlin(float x, float y)
		{
			unsafe { return InternalCalls.Noise_Perlin(x, y); }
		}

	}
}

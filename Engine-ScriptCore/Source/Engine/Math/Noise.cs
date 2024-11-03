using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Engine
{
	public class Noise
	{
		public Noise(int seed = 8)
		{
			m_UnmanagedInstance = InternalCalls.Noise_Constructor(seed);
		}

		~Noise()
		{
			InternalCalls.Noise_Destructor(m_UnmanagedInstance);
		}

		public float Frequency
		{
			get => InternalCalls.Noise_GetFrequency(m_UnmanagedInstance);
			set => InternalCalls.Noise_SetFrequency(m_UnmanagedInstance, value);
		}

		public int Octaves
		{
			get => InternalCalls.Noise_GetFractalOctaves(m_UnmanagedInstance);
			set => InternalCalls.Noise_SetFractalOctaves(m_UnmanagedInstance, value);
		}

		public float Lacunarity
		{
			get => InternalCalls.Noise_GetFractalLacunarity(m_UnmanagedInstance);
			set => InternalCalls.Noise_SetFractalLacunarity(m_UnmanagedInstance, value);
		}

		public float Gain
		{
			get => InternalCalls.Noise_GetFractalGain(m_UnmanagedInstance);
			set => InternalCalls.Noise_SetFractalGain(m_UnmanagedInstance, value);
		}

		public float Get(float x, float y)
		{
			return InternalCalls.Noise_Get(m_UnmanagedInstance, x, y);
		}

		internal IntPtr m_UnmanagedInstance;

		public static int StaticSeed
		{
			set => InternalCalls.Noise_SetSeed(value);
		}

		public static float Perlin(float x, float y)
		{
			return InternalCalls.Noise_Perlin(x, y);
		}
	}
}

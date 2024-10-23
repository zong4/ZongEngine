#include "pch.h"
#include "Noise.h"

#include "FastNoise.h"

namespace Hazel {

	static FastNoise s_FastNoise;
	static std::uniform_real_distribution<float> s_Jitters(0.f, 1.f);
	static std::default_random_engine s_JitterGenerator(1337u);

	Noise::Noise(int seed)
	{
		m_FastNoise = hnew FastNoise(seed);
		m_FastNoise->SetNoiseType(FastNoise::Simplex);
	}

	Noise::~Noise()
	{
		hdelete m_FastNoise;
	}

	float Noise::GetFrequency() const
	{
		return m_FastNoise->GetFrequency();
	}

	void Noise::SetFrequency(float frequency)
	{
		m_FastNoise->SetFrequency(frequency);
	}

	int Noise::GetFractalOctaves() const
	{
		return m_FastNoise->GetFractalOctaves();
	}

	void Noise::SetFractalOctaves(int octaves)
	{
		m_FastNoise->SetFractalOctaves(octaves);
	}

	float Noise::GetFractalLacunarity() const
	{
		return m_FastNoise->GetFractalLacunarity();
	}

	void Noise::SetFractalLacunarity(float lacunarity)
	{
		return m_FastNoise->SetFractalLacunarity(lacunarity);
	}

	float Noise::GetFractalGain() const
	{
		return m_FastNoise->GetFractalGain();
	}

	void Noise::SetFractalGain(float gain)
	{
		m_FastNoise->SetFractalGain(gain);
	}

	float Noise::Get(float x, float y)
	{
		return m_FastNoise->GetNoise(x, y);
	}

	void Noise::SetSeed(int seed)
	{
		s_FastNoise.SetSeed(seed);
	}

	float Noise::PerlinNoise(float x, float y)
	{
		s_FastNoise.SetNoiseType(FastNoise::Perlin);
		float result = s_FastNoise.GetNoise(x, y); // This returns a value between -1 and 1
		return result;
	}

	std::array<glm::vec4, 16> Noise::HBAOJitter()
	{
		constexpr float PI = 3.14159265358979323846264338f;
		const float numDir = 8.f;  // keep in sync to glsl

		std::array<glm::vec4, 16> result {};

		for (int i = 0; i < 16; i++)
		{
			float Rand1 = s_Jitters(s_JitterGenerator);
			float Rand2 = s_Jitters(s_JitterGenerator);
			// Use random rotation angles in [0,2PI/NUM_DIRECTIONS)
			const float Angle = 2.f * PI * Rand1 / numDir;
			result[i].x = cosf(Angle);
			result[i].y = sinf(Angle);
			result[i].z = Rand2;
			result[i].w = 0;
		}
		return result;
	}
}

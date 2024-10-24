#pragma once

class FastNoise;

namespace Engine {

	class Noise
	{
	public:
		Noise(int seed = 8);
		~Noise();
		
		float GetFrequency() const;
		void SetFrequency(float frequency);

		int GetFractalOctaves() const;
		void SetFractalOctaves(int octaves);

		float GetFractalLacunarity() const;
		void SetFractalLacunarity(float lacunarity);

		float GetFractalGain() const;
		void SetFractalGain(float gain);

		float Get(float x, float y);

		// Static API
		static void SetSeed(int seed);
		static float PerlinNoise(float x, float y);
		static std::array<glm::vec4, 16> HBAOJitter();
	private:
		FastNoise* m_FastNoise = nullptr;
	};

}

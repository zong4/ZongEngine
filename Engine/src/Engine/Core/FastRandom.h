#pragma once

namespace Hazel
{
	//==============================================================================
	/** Wrapper class for LCG random number generation algorithm
	*/
	struct FastRandom
	{
		FastRandom() = default;
		FastRandom(int seed) noexcept : State(seed) {}

		inline void SetSeed(int newSeed) noexcept { State = newSeed; }
		inline int GetCurrentSeed() const noexcept { return State; }

		inline int GetInt32() noexcept
		{
			State = (LCG_A * State + LCG_C) % LCG_M;
			return State;
		}

		inline unsigned int		GetUInt32() noexcept { return (unsigned int)GetInt32(); }
		inline short			GetInt16() noexcept { return (short)(GetInt32() & 0xFFFF); }
		inline unsigned short	GetUInt16() noexcept { return (unsigned short)(GetInt16() & 0xFFFF); }
		inline double			GetFloat64() noexcept { return GetInt32() / (double)0x7FFFFFFF; }
		inline float			GetFloat32() noexcept { return (float)GetFloat64(); }

		inline float GetFloat32InRange(float low, float high) noexcept
		{
			const auto scaleToRangeF32 = [](float x, float low, float high)
			{
				return low + x * (high - low);
			};
			return scaleToRangeF32(GetFloat32(), low, high);
		}

		inline double GetFloat64InRange(double low, double high) noexcept
		{
			const auto scaleToRangeF64 = [](double x, double low, double high)
			{
				return low + x * (high - low);
			};
			return scaleToRangeF64(GetFloat64(), low, high);
		}

		inline int GetInt32InRange(int low, int high) noexcept
		{
			if (low >= high)
				return low;

			return low + GetUInt32() / (0xFFFFFFFF / (high - low + 1) + 1);
		}

		float	GetInRange(float low, float high) { return GetFloat32InRange(low, high); }
		double	GetInRange(double low, double high) { return GetFloat64InRange(low, high); }
		int		GetInRange(int low, int high) { return GetInt32InRange(low, high); }

	private:
		int State = DEFAULT_LCG_SEED;

		static constexpr auto DEFAULT_LCG_SEED = 4321;
		static constexpr auto LCG_M = 2147483647;
		static constexpr auto LCG_A = 48271;
		static constexpr auto LCG_C = 0;
	};


	namespace Utils {

		inline int GetSeedFromCurrentTime() noexcept
		{
			int64_t timeSinceEpoch = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

			// NOTE(Yan): Time is used for seed, so 32-bits is fine
			return (int)timeSinceEpoch;
		}
	}

} // namespace Hazel

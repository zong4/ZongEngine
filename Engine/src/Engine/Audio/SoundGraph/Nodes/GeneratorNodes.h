#pragma once

#include "Engine/Audio/SoundGraph/NodeProcessor.h"

#include "Engine/Core/UUID.h"
#include "Engine/Core/FastRandom.h"

#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Engine::SoundGraph
{
	//==============================================================================

	// TODO: make an explicit NodeProcessor for each Noise type to avoid branching
	struct Noise : public NodeProcessor
	{
		explicit Noise(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{ 
			RegisterEndpoints();
		}

		int* in_Seed = nullptr;
		int32_t* in_Type = nullptr;
		float out_Value{ 0.0f };

		void RegisterEndpoints();
		void InitializeInputs();

		void Init() final
		{
			InitializeInputs();

			// TODO: use current time as a seed if not specified by the used and is of default value -1
			generator.Init(*in_Seed, (ENoiseType)(*in_Type));
		}

		void Process() final
		{
			// TODO: update seed if changed?

			out_Value = generator.GetNextValue();
		}

		enum ENoiseType : int32_t
		{
			WhiteNoise = 0, PinkNoise = 1, BrownianNoise = 2
		};

	private:
		struct Generator
		{
		public:
			void Init(int32_t seed, ENoiseType noiseType)
			{
				type = noiseType;
				SetSeed(seed);

				if (type == ENoiseType::PinkNoise)
				{
					memset(state.pink.bin, 0, sizeof(state.pink.bin));
					state.pink.accumulation = 0.0;
					state.pink.counter = 1;
				}

				if (type == ENoiseType::BrownianNoise)
				{
					state.brownian.accumulation = 0;
				}
			}

			inline void SetSeed(int32_t seed) noexcept
			{
				LCG.SetSeed(seed);
			}

			inline float GetNextValue()
			{
				switch (type)
				{
					case WhiteNoise: return	GetNextValueWhite();
					case PinkNoise: return GetNextValuePink();
					case BrownianNoise: return GetNextValueBrownian();
					default: return	GetNextValueWhite();
				}
			}

		private:
			/** Count the number of trailing zero bits */
			static inline unsigned int Tzcnt32(unsigned int x) noexcept
			{
				unsigned int n;

				/* Special case for odd numbers since they should happen about half the time. */
				if (x & 0x1)
					return 0;

				if (x == 0)
					return sizeof(x) << 3;

				n = 1;
				if ((x & 0x0000FFFF) == 0) { x >>= 16; n += 16; }
				if ((x & 0x000000FF) == 0) { x >>= 8; n += 8; }
				if ((x & 0x0000000F) == 0) { x >>= 4; n += 4; }
				if ((x & 0x00000003) == 0) { x >>= 2; n += 2; }
				n -= x & 0x00000001;

				return n;
			}

			inline float GetNextValueWhite() noexcept
			{
				return (float)(LCG.GetFloat64());
			}

			inline float GetNextValuePink()
			{
				double result;
				double binPrev;
				double binNext;
				uint32_t ibin;

				ibin = Tzcnt32(state.pink.counter) & (PINK_NOISE_BIN_SIZE - 1);

				binPrev = state.pink.bin[ibin];
				binNext = LCG.GetFloat64();
				state.pink.bin[ibin] = binNext;

				state.pink.accumulation += (binNext - binPrev);
				++state.pink.counter;

				result = (LCG.GetFloat64() + state.pink.accumulation);
				result /= 10;

				return (float)(result);
			}

			inline float GetNextValueBrownian()
			{
				double result;

				result = (LCG.GetFloat64() + state.brownian.accumulation);
				result /= 1.005; /* Don't escape the -1..1 range on average. */

				state.brownian.accumulation = result;
				result /= 20;

				return (float)(result);
			}

		private:
			static constexpr auto PINK_NOISE_BIN_SIZE = 16;

			ENoiseType type;
			FastRandom LCG;

			union
			{
				struct
				{
					double bin[PINK_NOISE_BIN_SIZE];
					double accumulation;
					uint32_t counter;
				} pink;
				struct
				{
					double accumulation;
				} brownian;
			} state;

		} generator;
	};

	//==============================================================================
	template<typename T>
	struct Random : public NodeProcessor
	{
		static_assert(std::is_arithmetic_v<T> && !std::is_same_v<T, bool>, "'Random' can only be of arithmentic type");

		struct IDs // TODO: could replace this with simple enum
		{
			DECLARE_ID(Next);
			DECLARE_ID(Reset);
		private:
			IDs() = delete;
		};

		explicit Random(const char* dbgName, UUID id);

		OutputEvent out_OnNext{ *this };
		OutputEvent out_OnReset{ *this };

		T* in_Min = nullptr;
		T* in_Max = nullptr;
		int* in_Seed = nullptr;
		T out_Value{ 0 };

		void Init() final;

		inline void Next()
		{
			out_Value = random.GetInRange(*in_Min, *in_Max);
			out_OnNext(1.0f);
		}

		inline void Reset()
		{
			out_OnReset(1.0f);
		}

	private:
		FastRandom random;
		int seed = -1;

		Flag fNext;
		Flag fReset;

	public:
		void Process() final
		{
			if (fNext.CheckAndResetIfDirty())
				Next();

			if (fReset.CheckAndResetIfDirty())
				Reset();
		}
	};

	static constexpr float minFreqHz = 0.0f;
	static constexpr float maxFreqHz = 22000.0f;

	//==============================================================================
	/// A simple sinewave oscillator which receives input events to control its frequency.
	struct Sine : public NodeProcessor
	{
		struct IDs
		{
			DECLARE_ID(ResetPhase);
		private:
			IDs() = delete;
		};

		explicit Sine(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			AddInEvent(IDs::ResetPhase, [&](float) { fResetPhase.SetDirty(); });
			RegisterEndpoints();
		}

		float* in_Frequency = nullptr;
		float* in_PhaseOffset = nullptr;
		float out_Sine = 0.0f;

		inline void ResetPhase()
		{
			phase = *in_PhaseOffset;
		}

	private:
		float initialFrequency = 1000.0;

		float phaseIncrement = 0.0f;
		float phase = 0.0f;
		double processorPeriod = 1.0 / 48000.0; // TODO: get from the graph

		// TODO: phase offset

		Flag fResetPhase;

	private:
		void RegisterEndpoints();
		void InitializeInputs();

	public:
		void Init() final
		{
			InitializeInputs();

			const float frequency = glm::clamp(*in_Frequency, minFreqHz, maxFreqHz);
			phaseIncrement = float(frequency * glm::two_pi<float>() * processorPeriod);

			phase = (*in_PhaseOffset);
		}

		void Process() final
		{
			if (fResetPhase.CheckAndResetIfDirty())
				ResetPhase();

			const float frequency = glm::clamp(*in_Frequency, minFreqHz, maxFreqHz); // TODO: not ideal to run this every frame, need value events
			phaseIncrement = float(frequency * glm::two_pi<float>() * processorPeriod);

			out_Sine = glm::sin(phase);
			phase = fmodf(phase + phaseIncrement, glm::two_pi<float>());
		}
	};


} // Engine::SoundGraph

#undef DECLARE_ID

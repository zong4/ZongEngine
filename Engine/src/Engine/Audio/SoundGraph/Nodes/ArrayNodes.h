#pragma once

#include "Engine/Audio/SoundGraph/NodeProcessor.h"

#include "Engine/Core/FastRandom.h"


#include <vector>
#include <chrono>

#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Engine::SoundGraph
{
	//==============================================================================
	/** Get random item from Array.
	*/
	template<typename T>
	struct GetRandom : public NodeProcessor
	{
		struct IDs // TODO: could replace this with simple enum
		{
			DECLARE_ID(Next);
			DECLARE_ID(Reset);
			
			DECLARE_ID(Array);
			DECLARE_ID(NoRepeats);			// TODO
			DECLARE_ID(EnabledSharedState);	// TODO

		private:
			IDs() = delete;
		};

		explicit GetRandom(const char* dbgName, UUID id);

		T* in_Array;
		int* in_Seed = nullptr;

		OutputEvent out_OnNext{ *this };
		OutputEvent out_OnReset{ *this };

		T out_Element{ 0 };

	private:
		Flag fNext;
		Flag fReset;

	public:

		void Init() final;

		inline void Next()
		{
			// If input Array has changed, we need to get random index for the new range
			arraySize = InValue(IDs::Array).size();

			const int index = random.GetInt32InRange(0, (int)(arraySize - 1));
			out_Element = (in_Array)[index];
			out_OnNext(1.0f);
		}

		inline void Reset()
		{
			seed = Utils::GetSeedFromCurrentTime();
			random.SetSeed(seed);
			out_OnReset(1.0f);
		}

		void Process() final
		{
			if (fNext.CheckAndResetIfDirty())
				Next();

			if (fReset.CheckAndResetIfDirty())
				Reset();
		}

	private:
		size_t arraySize = 0;
		int seed = -1;

		FastRandom random;
	};

	//==============================================================================
	/** Get item from Array.
	*/
	template<typename T>
	struct Get : public NodeProcessor
	{
		struct IDs
		{
			DECLARE_ID(Trigger);
			DECLARE_ID(Array);
		private:
			IDs() = delete;
		};

		explicit Get(const char* dbgName, UUID id);


		T* in_Array;
		int32_t* in_Index = nullptr;

		OutputEvent out_OnTrigger{ *this };

		T out_Element{ 0 };

	private:
		Flag fTrigger;

	public:

		void Init() final;

		inline void Trigger(float v)
		{
			const uint32_t arraySize = InValue(IDs::Array).size();
			const auto index = (uint32_t)(*in_Index);
			//ZONG_CORE_ASSERT(index < arraySize);

			const auto& element = in_Array[(index >= arraySize) ? (index % arraySize) : index];
			out_Element = element;
			out_OnTrigger((float)element); // TODO: value typed events? This seems to be redundand to send to Out Value and trigger Out Event
		}

		void Process() final
		{
			if (fTrigger.CheckAndResetIfDirty())
				Trigger(1.0f);
		}
	};

} // namespace Engine::SoundGraph

#undef DECLARE_ID

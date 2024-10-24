#pragma once

#include "NodeDescriptors.h"
#include "NodeTypes.h"

namespace Engine::SoundGraph {

	template<typename T>
	GetRandom<T>::GetRandom(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		AddInEvent(IDs::Next, [this](float v) { fNext.SetDirty(); });
		AddInEvent(IDs::Reset, [this](float v) { fReset.SetDirty(); });

		//AddInStream(IDs::NoRepeats); // TODO
		//AddInStream(IDs::EnabledSharedState); // TODO

		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void GetRandom<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);

		arraySize = InValue(IDs::Array).size();

		// If seed is at default value -1, take current time as a seed

		seed = *in_Seed;
		if (seed == -1)
		{
			seed = Utils::GetSeedFromCurrentTime();
		}

		random.SetSeed(seed);
		random.GetInt32(); //? need to get rid of the initial 0 value the generator produces

		const int index = random.GetInt32InRange(0, (int)(arraySize - 1));
		out_Element = (in_Array)[index];
	}

	template<typename T>
	Get<T>::Get(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		AddInEvent(IDs::Trigger, [this](float v) { fTrigger.SetDirty(); });

		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void Get<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);

		const uint32_t arraySize = InValue(IDs::Array).size();
		const auto index = (uint32_t)(*in_Index);
		//ZONG_CORE_ASSERT(index < arraySize);

		const auto& element = in_Array[(index >= arraySize) ? (index % arraySize) : index];
		out_Element = element;
	}

	template<typename T>
	Random<T>::Random(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		AddInEvent(IDs::Next, [this](float v) { fNext.SetDirty(); });
		AddInEvent(IDs::Reset, [this](float v) { fReset.SetDirty(); });

		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void Random<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);

		seed = *in_Seed;
		if (seed == -1)
		{
			// Set random seed
			random.SetSeed(Utils::GetSeedFromCurrentTime()); // TODO: use global random generator to set different seed for every next node
			random.GetInt32();
			seed = random.GetInt32();
		}

		random.SetSeed(seed);
		random.GetInt32();	 //? need to get rid of the initial 0 value the generator produces
	}

	template<typename T>
	Add<T>::Add(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void Add<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	Subtract<T>::Subtract(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void Subtract<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	Multiply<T>::Multiply(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void Multiply<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	Divide<T>::Divide(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void Divide<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	Min<T>::Min(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void Min<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	Max<T>::Max(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void Max<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	Clamp<T>::Clamp(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void Clamp<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	MapRange<T>::MapRange(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void MapRange<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	NoteToFrequency<T>::NoteToFrequency(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void NoteToFrequency<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
		out_Frequency = 440.0f * glm::exp2((*in_MIDINote - 69.0f) * (1.0f / 12.0f));
	}
}

#pragma once

#include "Engine/Audio/SoundGraph/NodeProcessor.h"
#include "Engine/Core/UUID.h"

#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Engine::SoundGraph {
	//==========================================================================
	struct BPMToSeconds : public NodeProcessor
	{
		explicit BPMToSeconds(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			RegisterEndpoints();
		}

		void Init() final
		{
			InitializeInputs();
			out_Seconds = (*in_BPM) <= 0.0f ? 0.0f : 60.0f / (*in_BPM);
		}

		//==========================================================================
		/// NodeProcessor setup
		float* in_BPM = nullptr;
		float out_Seconds = 1.0f;

	private:
		void RegisterEndpoints();
		void InitializeInputs();

	public:

		void Process() final
		{
			out_Seconds = (*in_BPM) <= 0.0f ? 0.0f : 60.0f / (*in_BPM);
		}
	};

	//==========================================================================
	template<typename T>
	struct NoteToFrequency : public NodeProcessor
	{
		explicit NoteToFrequency(const char* dbgName, UUID id);

		void Init() final;

		//==========================================================================
		/// NodeProcessor setup
		T* in_MIDINote = nullptr;
		float out_Frequency = 1.0f;

	public:
		void Process() final
		{
			out_Frequency = 440.0f * glm::exp2(((float)(*in_MIDINote) - 69.0f) * (1.0f / 12.0f));
		}
	};

	//==========================================================================
	struct FrequencyToNote : public NodeProcessor
	{
		explicit FrequencyToNote(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			RegisterEndpoints();
		}

		void Init() final
		{
			InitializeInputs();
			out_MIDINote = 69.0f + (12.0f / glm::log(2.0f)) * glm::log((*in_Frequency) * (1.0f / 440.0f));
		}

		//==========================================================================
		/// NodeProcessor setup
		float* in_Frequency = nullptr;
		float out_MIDINote = 60.0f;

	private:
		void RegisterEndpoints();
		void InitializeInputs();

	public:
		void Process() final
		{
			out_MIDINote = 69.0f + (12.0f / glm::log(2.0f)) * glm::log((*in_Frequency) * (1.0f / 440.0f));
		}
	};

} // namespace Engine::SoundGraph

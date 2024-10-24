#pragma once

#pragma once
#include "Engine/Audio/SoundGraph/NodeProcessor.h"
#include "Engine/Core/UUID.h"

#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Engine::SoundGraph
{
	//==========================================================================
	struct RepeatTrigger : public NodeProcessor
	{
		struct IDs
		{
			DECLARE_ID(Start);
			DECLARE_ID(Stop);
		private:
			IDs() = delete;
		};

		explicit RepeatTrigger(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			AddInEvent(IDs::Start, [this](float v) { fStart.SetDirty(); });
			AddInEvent(IDs::Stop, [this](float v) { fStop.SetDirty(); });
			
			RegisterEndpoints();
		}

		void Init() final
		{
			InitializeInputs();

			frameTime = 1.0f / 48000.0f; // TODO: get samplerate from the graph
			counter = 0.0f;
			playing = false;
		}

		//==========================================================================
		/// NodeProcessor setup
		float* in_Period = nullptr;
		OutputEvent out_Trigger{ *this };

	private:
		bool playing = false;
		float counter = 0.0f;
		float frameTime = 0.0f;

		Flag fStart;
		Flag fStop;

	private:
		void RegisterEndpoints();
		void InitializeInputs();

	public:
		void Start(float)
		{
			playing = true;
			counter = 0.0f;		// TODO: might want to not reset the counter for a "retrigger"
			out_Trigger(1.0f);
		}

		void Stop(float)
		{
			playing = false;
			counter = 0.0f;
		}

		void Process() final
		{
			if (fStart.CheckAndResetIfDirty())
				Start(1.0f);
			if (fStop.CheckAndResetIfDirty())
				Stop(1.0f);

			if (playing && (counter += frameTime) >= (*in_Period))
			{
				counter = 0.0f;
				out_Trigger(1.0f);
			}
		}
	};

	//==========================================================================
	struct TriggerCounter : public NodeProcessor
	{
		struct IDs
		{
			DECLARE_ID(Trigger);
			DECLARE_ID(Reset);
		private:
			IDs() = delete;
		};

		explicit TriggerCounter(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			AddInEvent(IDs::Trigger, [this](float v) { fTrigger.SetDirty(); }); // TODO: wrap these flag + function call into a utility struct?
			AddInEvent(IDs::Reset, [this](float v) { fReset.SetDirty(); });

			RegisterEndpoints();
		}

		void Init() final
		{
			InitializeInputs();

			out_Count = 0;
			out_Value = (*in_StepSize);
		}

		//==========================================================================
		/// NodeProcessor setup
		float* in_StartValue = nullptr;
		float* in_StepSize = nullptr;
		int* in_ResetCount = nullptr;

		int out_Count = 0;
		float out_Value = 0.0f;

		OutputEvent out_OnTrigger{ *this };
		OutputEvent out_OnReset{ *this };

	private:
		Flag fTrigger;
		Flag fReset;

	private:
		void RegisterEndpoints();
		void InitializeInputs();

	public:
		void Trigger(float)
		{
			++out_Count;
			out_Value = (*in_StepSize) * out_Count + (*in_StartValue);

			out_OnTrigger(1.0f);

			if ((out_Count >= (*in_ResetCount)) * (bool)(*in_ResetCount))
			{
				// TODO: should this be deffered to Process() with flag as well?
				Reset(1.0f);
			}
		}

		void Reset(float)
		{
			out_Value = (*in_StartValue);
			out_Count = 0;
			out_OnReset(1.0f);
		}

		void Process() final
		{
			if (fTrigger.CheckAndResetIfDirty())
				Trigger(1.0f);

			if (fReset.CheckAndResetIfDirty())
				Reset(1.0f);
		}
	};

	//==========================================================================
	struct DelayedTrigger : public NodeProcessor
	{
		struct IDs
		{
			DECLARE_ID(Trigger);
			DECLARE_ID(Reset);
		private:
			IDs() = delete;
		};

		explicit DelayedTrigger(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			AddInEvent(IDs::Trigger, [this](float v) { fTrigger.SetDirty(); });
			AddInEvent(IDs::Reset, [this](float v) { fReset.SetDirty(); });

			RegisterEndpoints();
		}

		void Init() final
		{
			InitializeInputs();

			frameTime = 1.0f / 48000.0f; // TODO: get samplerate from the graph
			counter = 0.0f;
			playing = false;
		}

		//==========================================================================
		/// NodeProcessor setup
		float* in_DelayTime = nullptr;
		OutputEvent out_DelayedTrigger{ *this };
		OutputEvent out_OnReset{ *this };

	private:
		bool playing = false;
		float counter = 0.0f;
		float frameTime = 0.0f;

		Flag fTrigger;
		Flag fReset;

	private:
		void RegisterEndpoints();
		void InitializeInputs();

	public:
		void Trigger(float)
		{
			playing = true;
			counter = 0.0f;
		}

		void Reset(float)
		{
			playing = false;
			counter = 0.0f;
			out_OnReset(1.0f);
		}

		void Process() final
		{
			if (fTrigger.CheckAndResetIfDirty())
				Trigger(1.0f);

			if (fReset.CheckAndResetIfDirty())
				Reset(1.0f);

			if (playing && (counter += frameTime) >= (*in_DelayTime))
			{
				playing = false;
				counter = 0.0f;
				out_DelayedTrigger(1.0f);
			}
		}
	};

} // namespace Engine::SoundGraph

#pragma once
#include "Hazel/Audio/SoundGraph/NodeProcessor.h"
#include "Hazel/Core/UUID.h"

#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Hazel::SoundGraph {
	//==========================================================================
	struct ADEnvelope : public NodeProcessor
	{
		struct IDs
		{
			DECLARE_ID(Trigger);
		private:
			IDs() = delete;
		};

		explicit ADEnvelope(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			AddInEvent(IDs::Trigger, [this](float v) { fTrigger.SetDirty(); });

			RegisterEndpoints();
		}

		void Init() final
		{
			InitializeInputs();

			attackCurve = glm::max(0.0f, *in_AttackCurve);
			decayCurve = glm::max(0.0f, *in_DecayCurve);

			if (*in_AttackTime <= 0.0f)
				attackRate = 1.0f; // clamping rate to 1 sample
			else
				attackRate = 1.0f / (*in_AttackTime * 48000.0f); // TODO: get sample rate from the graph

			if (*in_DecayTime <= 0.0f)
				decayRate = 1.0f;
			else
				decayRate = 1.0f / (*in_DecayTime * 48000.0f);

			value = 0.0f;
			target = 0.0f;

			state = Idle;
		}

		//==========================================================================
		/// NodeProcessor setup
		float* in_AttackTime = nullptr;
		float* in_DecayTime = nullptr;
		float* in_AttackCurve = nullptr;
		float* in_DecayCurve = nullptr;
		bool* in_Looping = nullptr;

		float out_OutEnvelope{ 0.0f };

		OutputEvent out_OnTrigger{ *this };
		OutputEvent out_OnComplete{ *this };

	private:
		//==========================================================================
		/// Envelope
		enum
		{
			Idle = 0,	// Before attack / after decay
			Attack = 1,
			Decay = 2,
		};
		int state{ Idle };

		float value = 0.0f;
		float target = 0.0f;
		float attackRate = 0.001f;
		float decayRate = 0.001f;

		float attackCurve = 1.0f;
		float decayCurve = 1.0f;

		Flag fTrigger;

	private:
		void RegisterEndpoints();
		void InitializeInputs();

	public:
		void Trigger(float)
		{
			attackCurve = glm::max(0.0f, *in_AttackCurve);
			decayCurve = glm::max(0.0f, *in_DecayCurve);

			if (*in_AttackTime <= 0.0f)
				attackRate = 1.0f; // clamping rate to 1 sample
			else
				attackRate = 1.0f / (*in_AttackTime * 48000.0f); // TODO: get sample rate from the graph
			
			if (*in_DecayTime <= 0.0f)
				decayRate = 1.0f;
			else
				decayRate = 1.0f / (*in_DecayTime * 48000.0f);

			// we deliberately don't reset current value to 0 to allow to restart attack from the current state
			// this makes retrigger smooth
			target = 1.0f;

			state = Attack;

			out_OnTrigger(1.0f);
		}

		void Process() final
		{
			if (fTrigger.CheckAndResetIfDirty())
				Trigger(1.0f);

			switch (state)
			{
			case Attack:
				value += attackRate;
				if (value >= target)
				{
					value = target;
					target = 0.0f;
					state = Decay;
				}

				out_OutEnvelope = glm::pow(value, attackCurve);
				break;

			case Decay:
				value -= decayRate;
				if (value <= 0.0f)
				{
					value = 0.0f;
					target = 1.0f;
					state = Idle + (int)(*in_Looping);
					
					out_OnComplete(1.0f);
				}

				out_OutEnvelope = 1.0f - glm::pow(1.0f - value, decayCurve);
				break;

			default:
				break;
			}
		}
	};

	//==========================================================================
	// TODO
	struct ADSREnvelope : public NodeProcessor
	{
		explicit ADSREnvelope(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
		}

		void Init() final
		{
		}
	};

} //Hazel::SoundGraph

#undef DECLARE_ID

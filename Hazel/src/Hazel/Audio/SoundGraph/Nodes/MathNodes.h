#pragma once
#include "Hazel/Audio/SoundGraph/NodeProcessor.h"
#include "Hazel/Core/UUID.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/log_base.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Hazel::SoundGraph
{
	//==========================================================================
	template<typename T>
	struct Add : public NodeProcessor
	{
		explicit Add(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value1 = nullptr; //? this indirection is still not ideal
		T* in_Value2 = nullptr;
		T out_Out{ 0 };

		virtual void Process() final
		{
			out_Out = (*in_Value1) + (*in_Value2);
		}
	};

	//==========================================================================
	template<typename T>
	struct Subtract : public NodeProcessor
	{
		Subtract(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value1 = nullptr;
		T* in_Value2 = nullptr;
		T out_Out{ 0 };

		void Process() final
		{
			out_Out = (*in_Value1) - (*in_Value2);
		}
	};

	//==========================================================================
	template<typename T>
	struct Multiply : public NodeProcessor
	{
		explicit Multiply(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value = nullptr;
		T* in_Multiplier = nullptr;
		T out_Out{ 0 };

		void Process() final
		{
			out_Out = (*in_Value)* (*in_Multiplier);
		}
	};

	//==========================================================================
	template<typename T>
	struct Divide : public NodeProcessor
	{
		explicit Divide(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value = nullptr;
		T* in_Denominator = nullptr;
		T out_Out{ 0 };

		void Process() final
		{
			if ((*in_Denominator) == T(0))
				out_Out = T(-1);
			else
				out_Out = (*in_Value) / (*in_Denominator);
		}
	};

	//==========================================================================
	struct Modulo : public NodeProcessor
	{
		explicit Modulo(const char* dbgName, UUID id);
		void Init() final;

		int* in_Value = nullptr;
		int* in_Modulo = nullptr;
		int out_Out{ 0 };

		void Process() final
		{
			out_Out = (*in_Value) % (*in_Modulo);
		}
	};

	//==========================================================================
	struct Power : public NodeProcessor
	{
		explicit Power(const char* dbgName, UUID id);
		void Init() final;

		float* in_Base = nullptr;
		float* in_Exponent = nullptr;
		float out_Out{ 0.0f };

		virtual void Process() final
		{
			out_Out = glm::pow((*in_Base), (*in_Exponent));
		}
	};

	//==========================================================================
	struct Log : public NodeProcessor
	{
		explicit Log(const char* dbgName, UUID id);
		void Init() final;

		float* in_Base = nullptr;
		float* in_Value = nullptr;
		float out_Out{ 0.0f };

		void Process() final
		{
			out_Out = glm::log((*in_Value), (*in_Base));
		}
	};

	//==========================================================================
	struct LinearToLogFrequency : public NodeProcessor
	{
		explicit LinearToLogFrequency(const char* dbgName, UUID id);
		void Init() final;

		float* in_Value = nullptr;
		float* in_Min = nullptr;
		float* in_Max = nullptr;
		float* in_MinFrequency = nullptr;
		float* in_MaxFrequency = nullptr;

		float out_Frequency{ 1000.0f };

		void Process() final
		{
			//? not ideal to do all this math here, should somehow cache the values and update them only when they change

			const float normalizedValue = (*in_Value - *in_Min) / (*in_Max - *in_Min);
			const float octaveRange = glm::log2(*in_MaxFrequency / *in_MinFrequency);
			out_Frequency = glm::exp2(normalizedValue * octaveRange) * (*in_MinFrequency);
		}
	};
	
	//==========================================================================
	struct FrequencyLogToLinear : public NodeProcessor
	{
		explicit FrequencyLogToLinear(const char* dbgName, UUID id);
		void Init() final;

		float* in_Frequency = nullptr;
		float* in_MinFrequency = nullptr;
		float* in_MaxFrequency = nullptr;
		float* in_Min = nullptr;
		float* in_Max = nullptr;

		float out_Value{ 0.0f };

		void Process() final
		{
			//? not ideal to do all this math here, should somehow cache the values and update them only when they change

			const float octavesBetweenMinAndTarget = glm::log2(*in_Frequency / *in_MinFrequency);
			const float octaveRange = 1.0f / glm::log2(*in_MaxFrequency / *in_MinFrequency);
			const float valueRange = (*in_Max - *in_Min);
			out_Value = octavesBetweenMinAndTarget / octaveRange * valueRange + *in_Min;
			
			//? would use cached inverse octave range if we were able to cache values
			//out_Value = octavesBetweenMinAndTarget * (1.0f / octaveRange) * valueRange + *in_Min;
		}
	};

	//==========================================================================
	template<typename T>
	struct Min : public NodeProcessor
	{
		explicit Min(const char* dbgName, UUID id);

		void Init() final;

		T* in_A = nullptr;
		T* in_B = nullptr;
		T out_Value{ 0 };

		void Process() final
		{
			out_Value = glm::min((*in_A), (*in_B));
		}
	};

	//==========================================================================
	template<typename T>
	struct Max : public NodeProcessor
	{
		explicit Max(const char* dbgName, UUID id);

		void Init() final;

		T* in_A = nullptr;
		T* in_B = nullptr;
		T out_Value{ 0 };

		void Process() final
		{
			out_Value = glm::max((*in_A), (*in_B));
		}
	};

	//==========================================================================
	template<typename T>
	struct Clamp : public NodeProcessor
	{
		explicit Clamp(const char* dbgName, UUID id);

		void Init() final;

		T* in_In = nullptr;
		T* in_Min = nullptr;
		T* in_Max = nullptr;
		T out_Value{ 0 };

		void Process() final
		{
			out_Value = glm::clamp((*in_In), (*in_Min), (*in_Max));
		}
	};

	//==========================================================================
	template<typename T>
	struct MapRange : public NodeProcessor
	{
		explicit MapRange(const char* dbgName, UUID id);

		void Init() final;

		T* in_In = nullptr;
		T* in_InRangeA = nullptr;
		T* in_InRangeB = nullptr;
		T* in_OutRangeA = nullptr;
		T* in_OutRangeB = nullptr;
		bool* in_Clamped = nullptr;

		T out_Value{ 0 };

		void Process() final
		{
			const bool clamped = (*in_Clamped);
			const T value = clamped ? glm::clamp((*in_In), (*in_InRangeA), (*in_InRangeB)) : (*in_In);
			const float t = (float)value / ((*in_InRangeB) - (*in_InRangeA));

			out_Value = glm::mix((*in_OutRangeA), (*in_OutRangeB), t);
		}
	};

} //Hazel::SoundGraph

#undef DECLARE_ID

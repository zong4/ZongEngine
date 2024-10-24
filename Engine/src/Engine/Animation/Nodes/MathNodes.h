#pragma once
#include "Engine/Animation/NodeProcessor.h"
#include "Engine/Core/UUID.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/log_base.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

namespace Engine::AnimationGraph {

	template<typename T>
	struct Add : public NodeProcessor
	{
		explicit Add(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value1 = nullptr; //? this indirection is still not ideal
		T* in_Value2 = nullptr;
		T out_Out{ 0 };

		float Process(float) final
		{
			out_Out = (*in_Value1) + (*in_Value2);
			return 0.0f;
		}
	};


	template<typename T>
	struct Subtract : public NodeProcessor
	{
		Subtract(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value1 = nullptr;
		T* in_Value2 = nullptr;
		T out_Out{ 0 };

		float Process(float) final
		{
			out_Out = (*in_Value1) - (*in_Value2);
			return 0.0f;
		}
	};


	template<typename T>
	struct Multiply : public NodeProcessor
	{
		explicit Multiply(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value = nullptr;
		T* in_Multiplier = nullptr;
		T out_Out{ 0 };

		float Process(float) final
		{
			out_Out = (*in_Value) * (*in_Multiplier);
			return 0.0f;
		}
	};


	template<typename T>
	struct Divide : public NodeProcessor
	{
		explicit Divide(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value = nullptr;
		T* in_Divisor = nullptr;
		T out_Out{ 0 };

		float Process(float) final
		{
			if ((*in_Divisor) == T(0))
				out_Out = T(-1);
			else
				out_Out = (*in_Value) / (*in_Divisor);
			return 0.0f;
		}
	};


	struct Modulo : public NodeProcessor
	{
		explicit Modulo(const char* dbgName, UUID id);
		void Init() final;

		int* in_Value = nullptr;
		int* in_Divisor = nullptr;
		int out_Out{ 0 };

		float Process(float) final
		{
			out_Out = (*in_Value) % (*in_Divisor);
			return 0.0f;
		}
	};


	struct Power : public NodeProcessor
	{
		explicit Power(const char* dbgName, UUID id);
		void Init() final;

		float* in_Base = nullptr;
		float* in_Exponent = nullptr;
		float out_Out{ 0.0f };

		virtual float Process(float timestep) final
		{
			out_Out = glm::pow((*in_Base), (*in_Exponent));
			return 0.0;
		}
	};


	struct Log : public NodeProcessor
	{
		explicit Log(const char* dbgName, UUID id);
		void Init() final;

		float* in_Value = nullptr;
		float* in_Base = nullptr;
		float out_Out{ 0.0f };

		float Process(float) final
		{
			out_Out = glm::log((*in_Value), (*in_Base));
			return 0.0f;
		}
	};


	template<typename T>
	struct Min : public NodeProcessor
	{
		explicit Min(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value1 = nullptr;
		T* in_Value2 = nullptr;
		T out_Value{ 0 };

		float Process(float) final
		{
			out_Value = glm::min((*in_Value1), (*in_Value2));
			return 0.0f;
		}
	};


	template<typename T>
	struct Max : public NodeProcessor
	{
		explicit Max(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value1 = nullptr;
		T* in_Value2 = nullptr;
		T out_Value{ 0 };

		float Process(float) final
		{
			out_Value = glm::max((*in_Value1), (*in_Value2));
			return 0.0f;
		}
	};


	template<typename T>
	struct Clamp : public NodeProcessor
	{
		explicit Clamp(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value = nullptr;
		T* in_Min = nullptr;
		T* in_Max = nullptr;
		T out_Value{ 0 };

		float Process(float) final
		{
			out_Value = glm::clamp((*in_Value), (*in_Min), (*in_Max));
			return 0.0f;
		}
	};


	template<typename T>
	struct MapRange : public NodeProcessor
	{
		explicit MapRange(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value = nullptr;
		T* in_InRangeMin = nullptr;
		T* in_InRangeMax = nullptr;
		T* in_OutRangeMin = nullptr;
		T* in_OutRangeMax = nullptr;
		bool* in_Clamped = nullptr;

		T out_Value{ 0 };

		float Process(float) final
		{
			const bool clamped = (*in_Clamped);
			const T value = clamped ? glm::clamp((*in_Value), (*in_InRangeMin), (*in_InRangeMax)) : (*in_Value);
			const float t = (float)value / ((*in_InRangeMax) - (*in_InRangeMin));

			out_Value = glm::mix((*in_OutRangeMin), (*in_OutRangeMax), t);
			return 0.0f;
		}
	};

} //Engine::AnimationGraph

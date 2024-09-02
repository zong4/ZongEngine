#pragma once
#include "Hazel/Animation/NodeDescriptor.h"
#include "Hazel/Animation/NodeProcessor.h"
#include "Hazel/Core/UUID.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/log_base.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

namespace Hazel::AnimationGraph {

	template<typename T>
	struct Add : public NodeProcessor
	{
		explicit Add(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		T* in_Value1 = nullptr; //? this indirection is still not ideal
		T* in_Value2 = nullptr;
		T out_Out{ 0 };

		float Process(float) override
		{
			out_Out = (*in_Value1) + (*in_Value2);
			return 0.0f;
		}
	};


	template<typename T>
	struct Subtract : public NodeProcessor
	{
		Subtract(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		T* in_Value1 = nullptr;
		T* in_Value2 = nullptr;
		T out_Out{ 0 };

		float Process(float) override
		{
			out_Out = (*in_Value1) - (*in_Value2);
			return 0.0f;
		}
	};


	template<typename T>
	struct Multiply : public NodeProcessor
	{
		explicit Multiply(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		T* in_Value = nullptr;
		T* in_Multiplier = nullptr;
		T out_Out{ 0 };

		float Process(float) override
		{
			out_Out = (*in_Value) * (*in_Multiplier);
			return 0.0f;
		}
	};


	template<typename T>
	struct Divide : public NodeProcessor
	{
		explicit Divide(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		T* in_Value = nullptr;
		T* in_Divisor = nullptr;
		T out_Out{ 0 };

		float Process(float) override
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
		explicit Modulo(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		int* in_Value = nullptr;
		int* in_Divisor = nullptr;
		int out_Out{ 0 };

		float Process(float) override
		{
			out_Out = (*in_Value) % (*in_Divisor);
			return 0.0f;
		}
	};


	struct Power : public NodeProcessor
	{
		explicit Power(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		float* in_Base = nullptr;
		float* in_Exponent = nullptr;
		float out_Out{ 0.0f };

		virtual float Process(float timestep) override
		{
			out_Out = glm::pow((*in_Base), (*in_Exponent));
			return 0.0;
		}
	};


	struct Log : public NodeProcessor
	{
		explicit Log(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		float* in_Value = nullptr;
		float* in_Base = nullptr;
		float out_Out{ 0.0f };

		float Process(float) override
		{
			out_Out = glm::log((*in_Value), (*in_Base));
			return 0.0f;
		}
	};


	template<typename T>
	struct Min : public NodeProcessor
	{
		explicit Min(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		T* in_Value1 = nullptr;
		T* in_Value2 = nullptr;
		T out_Value{ 0 };

		float Process(float) override
		{
			out_Value = glm::min((*in_Value1), (*in_Value2));
			return 0.0f;
		}
	};


	template<typename T>
	struct Max : public NodeProcessor
	{
		explicit Max(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		T* in_Value1 = nullptr;
		T* in_Value2 = nullptr;
		T out_Value{ 0 };

		float Process(float) override
		{
			out_Value = glm::max((*in_Value1), (*in_Value2));
			return 0.0f;
		}
	};


	template<typename T>
	struct Clamp : public NodeProcessor
	{
		explicit Clamp(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		T* in_Value = nullptr;
		T* in_Min = nullptr;
		T* in_Max = nullptr;
		T out_Value{ 0 };

		float Process(float) override
		{
			out_Value = glm::clamp((*in_Value), (*in_Min), (*in_Max));
			return 0.0f;
		}
	};


	template<typename T>
	struct MapRange : public NodeProcessor
	{
		explicit MapRange(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		T* in_Value = nullptr;
		T* in_InRangeMin = nullptr;
		T* in_InRangeMax = nullptr;
		T* in_OutRangeMin = nullptr;
		T* in_OutRangeMax = nullptr;
		bool* in_Clamped = nullptr;

		T out_Value{ 0 };

		float Process(float) override
		{
			const bool clamped = (*in_Clamped);
			const T value = clamped ? glm::clamp((*in_Value), (*in_InRangeMin), (*in_InRangeMax)) : (*in_Value);
			const float t = (float)value / ((*in_InRangeMax) - (*in_InRangeMin));

			out_Value = glm::mix((*in_OutRangeMin), (*in_OutRangeMax), t);
			return 0.0f;
		}
	};

} //Hazel::AnimationGraph


DESCRIBE_NODE(Hazel::AnimationGraph::Add<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Add<float>::in_Value1,
		&Hazel::AnimationGraph::Add<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Add<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Add<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Add<int>::in_Value1,
		&Hazel::AnimationGraph::Add<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Add<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Subtract<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Subtract<float>::in_Value1,
		&Hazel::AnimationGraph::Subtract<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Subtract<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Subtract<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Subtract<int>::in_Value1,
		&Hazel::AnimationGraph::Subtract<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Subtract<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Multiply<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Multiply<float>::in_Value,
		&Hazel::AnimationGraph::Multiply<float>::in_Multiplier),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Multiply<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Multiply<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Multiply<int>::in_Value,
		&Hazel::AnimationGraph::Multiply<int>::in_Multiplier),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Multiply<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Divide<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Divide<float>::in_Value,
		&Hazel::AnimationGraph::Divide<float>::in_Divisor),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Divide<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Divide<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Divide<int>::in_Value,
		&Hazel::AnimationGraph::Divide<int>::in_Divisor),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Divide<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Power,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Power::in_Base,
		&Hazel::AnimationGraph::Power::in_Exponent),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Power::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Log,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Log::in_Base,
		&Hazel::AnimationGraph::Log::in_Value),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Log::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Modulo,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Modulo::in_Value,
		&Hazel::AnimationGraph::Modulo::in_Divisor),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Modulo::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Min<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Min<float>::in_Value1,
		&Hazel::AnimationGraph::Min<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Min<float>::out_Value)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Min<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Min<int>::in_Value1,
		&Hazel::AnimationGraph::Min<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Min<int>::out_Value)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Max<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Max<float>::in_Value1,
		&Hazel::AnimationGraph::Max<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Max<float>::out_Value)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Max<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Max<int>::in_Value1,
		&Hazel::AnimationGraph::Max<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Max<int>::out_Value)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Clamp<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Clamp<float>::in_Value,
		&Hazel::AnimationGraph::Clamp<float>::in_Min,
		&Hazel::AnimationGraph::Clamp<float>::in_Max),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Clamp<float>::out_Value)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Clamp<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Clamp<int>::in_Value,
		&Hazel::AnimationGraph::Clamp<int>::in_Min,
		&Hazel::AnimationGraph::Clamp<int>::in_Max),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Clamp<int>::out_Value)
);

DESCRIBE_NODE(Hazel::AnimationGraph::MapRange<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::MapRange<float>::in_Value,
		&Hazel::AnimationGraph::MapRange<float>::in_InRangeMin,
		&Hazel::AnimationGraph::MapRange<float>::in_InRangeMax,
		&Hazel::AnimationGraph::MapRange<float>::in_OutRangeMin,
		&Hazel::AnimationGraph::MapRange<float>::in_OutRangeMax,
		&Hazel::AnimationGraph::MapRange<float>::in_Clamped),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::MapRange<float>::out_Value)
);

DESCRIBE_NODE(Hazel::AnimationGraph::MapRange<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::MapRange<int>::in_Value,
		&Hazel::AnimationGraph::MapRange<int>::in_InRangeMin,
		&Hazel::AnimationGraph::MapRange<int>::in_InRangeMax,
		&Hazel::AnimationGraph::MapRange<int>::in_OutRangeMin,
		&Hazel::AnimationGraph::MapRange<int>::in_OutRangeMax,
		&Hazel::AnimationGraph::MapRange<int>::in_Clamped),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::MapRange<int>::out_Value)
);

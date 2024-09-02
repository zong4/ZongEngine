#pragma once
#include "Hazel/Animation/NodeDescriptor.h"
#include "Hazel/Animation/NodeProcessor.h"
#include "Hazel/Core/UUID.h"

namespace Hazel::AnimationGraph {

	template<typename T>
	struct CheckEqual : public NodeProcessor
	{
		explicit CheckEqual(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		T* in_Value1 = nullptr; //? this indirection is still not ideal
		T* in_Value2 = nullptr;
		bool out_Out{ 0 };

		float Process(float timestep) override
		{
			out_Out = (*in_Value1 == *in_Value2);
			return timestep;
		}
	};


	template<typename T>
	struct CheckNotEqual : public NodeProcessor
	{
		explicit CheckNotEqual(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		T* in_Value1 = nullptr; //? this indirection is still not ideal
		T* in_Value2 = nullptr;
		bool out_Out{ 0 };

		float Process(float timestep) override
		{
			out_Out = (*in_Value1 != *in_Value2);
			return timestep;
		}
	};


	template<typename T>
	struct CheckLess : public NodeProcessor
	{
		explicit CheckLess(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		T* in_Value1 = nullptr; //? this indirection is still not ideal
		T* in_Value2 = nullptr;
		bool out_Out{ 0 };

		float Process(float timestep) override
		{
			out_Out = (*in_Value1 < *in_Value2);
			return timestep;
		}
	};


	template<typename T>
	struct CheckLessEqual : public NodeProcessor
	{
		explicit CheckLessEqual(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		T* in_Value1 = nullptr; //? this indirection is still not ideal
		T* in_Value2 = nullptr;
		bool out_Out{ 0 };

		float Process(float timestep) override
		{
			out_Out = (*in_Value1 <= *in_Value2);
			return timestep;
		}
	};


	template<typename T>
	struct CheckGreater : public NodeProcessor
	{
		explicit CheckGreater(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		T* in_Value1 = nullptr; //? this indirection is still not ideal
		T* in_Value2 = nullptr;
		bool out_Out{ 0 };

		float Process(float timestep) override
		{
			out_Out = (*in_Value1 > *in_Value2);
			return timestep;
		}
	};


	template<typename T>
	struct CheckGreaterEqual : public NodeProcessor
	{
		explicit CheckGreaterEqual(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		T* in_Value1 = nullptr; //? this indirection is still not ideal
		T* in_Value2 = nullptr;
		bool out_Out{ 0 };

		float Process(float timestep) override
		{
			out_Out = (*in_Value1 >= *in_Value2);
			return timestep;
		}
	};


	struct And : public NodeProcessor
	{
		explicit And(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		bool* in_Value1 = nullptr; //? this indirection is still not ideal
		bool* in_Value2 = nullptr;
		bool out_Out{ 0 };

		float Process(float timestep) override
		{
			out_Out = *in_Value1 && *in_Value2;
			return timestep;
		}
	};


	struct Or : public NodeProcessor
	{
		explicit Or(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		bool* in_Value1 = nullptr; //? this indirection is still not ideal
		bool* in_Value2 = nullptr;
		bool out_Out{ 0 };

		float Process(float timestep) override
		{
			out_Out = *in_Value1 || *in_Value2;
			return timestep;
		}
	};


	struct Not : public NodeProcessor
	{
		explicit Not(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);
		}

		bool* in_Value = nullptr; //? this indirection is still not ideal
		bool out_Out{ 0 };

		float Process(float timestep) override
		{
			out_Out = !*in_Value;
			return timestep;
		}
	};

} //Hazel::AnimationGraph


DESCRIBE_NODE(Hazel::AnimationGraph::CheckEqual<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckEqual<int>::in_Value1,
		&Hazel::AnimationGraph::CheckEqual<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckEqual<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckNotEqual<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckNotEqual<int>::in_Value1,
		&Hazel::AnimationGraph::CheckNotEqual<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckNotEqual<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckLess<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckLess<int>::in_Value1,
		&Hazel::AnimationGraph::CheckLess<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckLess<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckLessEqual<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckLessEqual<int>::in_Value1,
		&Hazel::AnimationGraph::CheckLessEqual<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckLessEqual<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckGreater<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckGreater<int>::in_Value1,
		&Hazel::AnimationGraph::CheckGreater<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckGreater<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckGreaterEqual<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckGreaterEqual<int>::in_Value1,
		&Hazel::AnimationGraph::CheckGreaterEqual<int>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckGreaterEqual<int>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckEqual<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckEqual<float>::in_Value1,
		&Hazel::AnimationGraph::CheckEqual<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckEqual<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckNotEqual<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckNotEqual<float>::in_Value1,
		&Hazel::AnimationGraph::CheckNotEqual<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckNotEqual<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckLess<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckLess<float>::in_Value1,
		&Hazel::AnimationGraph::CheckLess<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckLess<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckLessEqual<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::CheckLessEqual<float>::in_Value1,
		&Hazel::AnimationGraph::CheckLessEqual<float>::in_Value2),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::CheckLessEqual<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckGreater<float>,
		NODE_INPUTS(
			&Hazel::AnimationGraph::CheckGreater<float>::in_Value1,
			&Hazel::AnimationGraph::CheckGreater<float>::in_Value2),
		NODE_OUTPUTS(
			&Hazel::AnimationGraph::CheckGreater<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::CheckGreaterEqual<float>,
		NODE_INPUTS(
			&Hazel::AnimationGraph::CheckGreaterEqual<float>::in_Value1,
			&Hazel::AnimationGraph::CheckGreaterEqual<float>::in_Value2),
		NODE_OUTPUTS(
			&Hazel::AnimationGraph::CheckGreaterEqual<float>::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::And,
	NODE_INPUTS(
		&Hazel::AnimationGraph::And::in_Value1,
		&Hazel::AnimationGraph::And::in_Value2),
		NODE_OUTPUTS(
			&Hazel::AnimationGraph::And::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Or,
		NODE_INPUTS(
			&Hazel::AnimationGraph::Or::in_Value1,
			&Hazel::AnimationGraph::Or::in_Value2),
			NODE_OUTPUTS(
				&Hazel::AnimationGraph::Or::out_Out)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Not,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Not::in_Value),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Not::out_Out)
);

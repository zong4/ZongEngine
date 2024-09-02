#pragma once
#include "Hazel/Animation/NodeDescriptor.h"
#include "Hazel/Animation/NodeProcessor.h"
#include "Hazel/Core/FastRandom.h"

#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Hazel::AnimationGraph {

	// Sometimes we need to reuse NodeProcessor types for different nodes,
	// so that they have different names and other editor UI properties
	// but the same NodeProcessor type in the backend.
	// In such case we declare alias ID here and define it in the AnimationGraphNodes.cpp,
	// adding and extra entry to the registry of the same NodeProcessor type,
	// but with a different name.
	// Actual NodeProcessor type for the alias is assigned in AnimationGraphFactory.cpp
	//! Aliases must already be "User Friendly Type Name" in format: "Get Random (Float)" instead of "GetRandom<float>"
	namespace Alias {
		static constexpr auto GetAnimation = "Get (Animation)";
		static constexpr auto GetRandomAnimation = "Get Random (Animation)";
	}

	template<typename T>
	struct Get : public NodeProcessor
	{
		struct IDs
		{
			DECLARE_ID(Array);
		private:
			IDs() = delete;
		};

		explicit Get(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			EndpointUtilities::RegisterEndpoints(this);
		}

		T* in_Array = nullptr;
		int32_t* in_Index = nullptr;

		T out_Element{ 0 };

	public:
		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);

			const uint32_t arraySize = InValue(IDs::Array).size();
			const auto index = (uint32_t)(*in_Index);

			const auto& element = in_Array[(index >= arraySize) ? (index % arraySize) : index];
			out_Element = element;
		}

		float Process(float) override
		{
			const uint32_t arraySize = InValue(IDs::Array).size();
			const auto index = (uint32_t)(*in_Index);

			out_Element = in_Array[(index >= arraySize) ? (index % arraySize) : index];
			return 0.0f;
		}
	};


	template<typename T>
	struct GetRandom : public NodeProcessor
	{
		struct IDs // TODO: could replace this with simple enum
		{
			DECLARE_ID(Array);
			DECLARE_ID(Next);
			DECLARE_ID(Reset);

		private:
			IDs() = delete;
		};

		explicit GetRandom(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			AddInEvent(IDs::Next, [this](Identifier eventID) { fNext.SetDirty(); });
			AddInEvent(IDs::Reset, [this](Identifier eventID) { fReset.SetDirty(); });
			EndpointUtilities::RegisterEndpoints(this);
		}

		T* in_Array = nullptr;
		int* in_Seed = &DefaultSeed;

		inline static int DefaultSeed = -1;

		T out_Element{ 0 };

	private:
		Flag fNext;
		Flag fReset;

	public:
		void Init(const Skeleton*) override
		{
			EndpointUtilities::InitializeInputs(this);

			Reset();
			const auto& element = in_Array[(m_Index >= m_ArraySize) ? (m_Index % m_ArraySize) : m_Index];
			out_Element = element;
		}

		inline void Next()
		{
			m_ArraySize = InValue(IDs::Array).size();
			m_Index = m_Random.GetInt32InRange(0, (int)(m_ArraySize - 1));
		}

		inline void Reset()
		{
			m_Random.SetSeed(*in_Seed);
			Next();
		}

		float Process(float) override
		{
			if (fNext.CheckAndResetIfDirty())
				Next();

			if (fReset.CheckAndResetIfDirty())
				Reset();

			out_Element = in_Array[(m_Index >= m_ArraySize) ? (m_Index % m_ArraySize) : m_Index];
			return 0.0f;
		}

	private:
		FastRandom m_Random;
		size_t m_ArraySize = 0;
		int m_Index = 0;
	};

} // namespace Hazel::AnimationGraph


DESCRIBE_NODE(Hazel::AnimationGraph::Get<float>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Get<float>::in_Array,
		&Hazel::AnimationGraph::Get<float>::in_Index),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Get<float>::out_Element)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Get<int>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Get<int>::in_Array,
		&Hazel::AnimationGraph::Get<int>::in_Index),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Get<int>::out_Element)
);

DESCRIBE_NODE(Hazel::AnimationGraph::Get<int64_t>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::Get<int64_t>::in_Array,
		&Hazel::AnimationGraph::Get<int64_t>::in_Index),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::Get<int64_t>::out_Element)
);


DESCRIBE_NODE(Hazel::AnimationGraph::GetRandom<int64_t>,
	NODE_INPUTS(
		&Hazel::AnimationGraph::GetRandom<int64_t>::Next,
		&Hazel::AnimationGraph::GetRandom<int64_t>::Reset,
		&Hazel::AnimationGraph::GetRandom<int64_t>::in_Array,
		&Hazel::AnimationGraph::GetRandom<int64_t>::in_Seed),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::GetRandom<int64_t>::out_Element)
);

#undef DECLARE_ID

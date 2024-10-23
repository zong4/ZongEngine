#pragma once

#include "NodeDescriptors.h"

namespace Hazel::AnimationGraph {

	template<typename T>
	Get<T>::Get(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}


	template<typename T>
	void Get<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);

		const uint32_t arraySize = InValue(IDs::Array).size();
		const auto index = (uint32_t)(*in_Index);

		const auto& element = in_Array[(index >= arraySize) ? (index % arraySize) : index];
		out_Element = element;
	}


	template<typename T>
	float Get<T>::Process(float)
	{
		const uint32_t arraySize = InValue(IDs::Array).size();
		const auto index = (uint32_t)(*in_Index);

		out_Element = in_Array[(index >= arraySize) ? (index % arraySize) : index];
		return 0.0f;
	}

}

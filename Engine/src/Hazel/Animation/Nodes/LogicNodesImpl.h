#pragma once

#include "NodeDescriptors.h"

namespace Hazel::AnimationGraph {

	template<typename T>
	CheckEqual<T>::CheckEqual(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void CheckEqual<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	CheckNotEqual<T>::CheckNotEqual(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void CheckNotEqual<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	CheckLess<T>::CheckLess(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void CheckLess<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	CheckLessEqual<T>::CheckLessEqual(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void CheckLessEqual<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	CheckGreater<T>::CheckGreater(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void CheckGreater<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	CheckGreaterEqual<T>::CheckGreaterEqual(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void CheckGreaterEqual<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	And::And(const char* dbgName, UUID id) : NodeProcessor(dbgName, id) { EndpointUtilities::RegisterEndpoints(this); }
	void And::Init() { EndpointUtilities::InitializeInputs(this); }

	Or::Or(const char* dbgName, UUID id) : NodeProcessor(dbgName, id) { EndpointUtilities::RegisterEndpoints(this); }
	void Or::Init() { EndpointUtilities::InitializeInputs(this); }

	Not::Not(const char* dbgName, UUID id) : NodeProcessor(dbgName, id) { EndpointUtilities::RegisterEndpoints(this); }
	void Not::Init() { EndpointUtilities::InitializeInputs(this); }

}

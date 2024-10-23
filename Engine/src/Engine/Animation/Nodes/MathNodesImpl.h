#pragma once

#include "NodeDescriptors.h"

namespace Hazel::AnimationGraph {

	template<typename T>
	Add<T>::Add(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void Add<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	Subtract<T>::Subtract(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void Subtract<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	Multiply<T>::Multiply(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void Multiply<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	Divide<T>::Divide(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void Divide<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	Modulo::Modulo(const char* dbgName, UUID id) : NodeProcessor(dbgName, id) { EndpointUtilities::RegisterEndpoints(this); }
	void Modulo::Init() { EndpointUtilities::InitializeInputs(this); }

	Power::Power(const char* dbgName, UUID id) : NodeProcessor(dbgName, id) { EndpointUtilities::RegisterEndpoints(this); }
	void Power::Init() { EndpointUtilities::InitializeInputs(this); }

	Log::Log(const char* dbgName, UUID id) : NodeProcessor(dbgName, id) { EndpointUtilities::RegisterEndpoints(this); }
	void Log::Init() { EndpointUtilities::InitializeInputs(this); }

	template<typename T>
	Min<T>::Min(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void Min<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	Max<T>::Max(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void Max<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	Clamp<T>::Clamp(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void Clamp<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

	template<typename T>
	MapRange<T>::MapRange(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}

	template<typename T>
	void MapRange<T>::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}

}

#include "pch.h"
#include "Base.h"

#include "Log.h"
#include "Memory.h"

#define ENGINE_BUILD_ID "v0.1a"

namespace Engine {

	void InitializeCore()
	{
		Allocator::Init();
		Log::Init();

		ZONG_CORE_TRACE_TAG("Core", "Engine Engine {}", ENGINE_BUILD_ID);
		ZONG_CORE_TRACE_TAG("Core", "Initializing...");
	}

	void ShutdownCore()
	{
		ZONG_CORE_TRACE_TAG("Core", "Shutting down...");
		
		Log::Shutdown();
	}

}

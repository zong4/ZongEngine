#include "pch.h"
#include "Base.h"

#include "Log.h"
#include "Memory.h"

#define HAZEL_BUILD_ID "v0.1a"

namespace Hazel {

	void InitializeCore()
	{
		Allocator::Init();
		Log::Init();

		ZONG_CORE_TRACE_TAG("Core", "Hazel Engine {}", HAZEL_BUILD_ID);
		ZONG_CORE_TRACE_TAG("Core", "Initializing...");
	}

	void ShutdownCore()
	{
		ZONG_CORE_TRACE_TAG("Core", "Shutting down...");
		
		Log::Shutdown();
	}

}

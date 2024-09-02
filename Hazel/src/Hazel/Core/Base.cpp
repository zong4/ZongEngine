#include "hzpch.h"
#include "Base.h"

#include "Log.h"
#include "Memory.h"

namespace Hazel {

	void InitializeCore()
	{
		Allocator::Init();
		Log::Init();

		HZ_CORE_TRACE_TAG("Core", "Hazel Engine {}", HZ_VERSION);
		HZ_CORE_TRACE_TAG("Core", "Initializing...");
	}

	void ShutdownCore()
	{
		HZ_CORE_TRACE_TAG("Core", "Shutting down...");
		
		Log::Shutdown();
	}

}

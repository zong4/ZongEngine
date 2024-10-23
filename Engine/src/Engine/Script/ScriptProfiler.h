#pragma once

extern "C" {
	typedef struct _MonoProfiler MonoProfiler;
	typedef struct _MonoDomain MonoDomain;
	typedef struct _MonoObject MonoObject;
}

namespace Hazel {

	class ScriptProfiler
	{
	public:
		struct GCStats
		{
			uint32_t MaxGeneration = 0;
			uint32_t TotalAllocations = 0;
			uint32_t TotalDeallocations = 0;
			std::vector<MonoObject*> AllocatedObjects;
		};

	public:
		static void Init();
		static void Shutdown();

		static const GCStats& GetGCStats();

	private:
		static void OnDomainLoaded(MonoProfiler* profiler, MonoDomain* domain);
		static void OnDomainUnloaded(MonoProfiler* profiler, MonoDomain* domain);
		static void OnGCAllocationCallback(MonoProfiler* profiler, MonoObject* obj);
		static void OnGCMovesCallback(MonoProfiler* profiler, MonoObject* const* objects, uint64_t count);
		static void OnGCFinalizingObjectCallback(MonoProfiler* profiler, MonoObject* obj);
	};

}

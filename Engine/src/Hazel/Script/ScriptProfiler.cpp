#include "hzpch.h"
#include "ScriptProfiler.h"
#include "ScriptEngine.h"
#include "ScriptCache.h"

#include "Hazel/Core/Application.h"
#include "Hazel/Utilities/ContainerUtils.h"
#include "Hazel/Utilities/StringUtils.h"

#include <mono/metadata/profiler.h>

#include <mutex>

namespace Hazel {

#define HZ_IGNORE_EXTERNAL_ALLOCATIONS

	struct ProfilerState
	{
		MonoProfilerHandle ProfilerHandle = nullptr;
		ScriptProfiler::GCStats GCStats;
		bool ReloadingDomain = false;
	};

	static ProfilerState* s_ProfilerState = nullptr;
	void ScriptProfiler::Init()
	{
		HZ_CORE_ASSERT(!s_ProfilerState, "Trying to initialize Script Profiler multiple times!");
		s_ProfilerState = hnew ProfilerState();
		s_ProfilerState->ProfilerHandle = mono_profiler_create(NULL);

		// Register callbacks
		mono_profiler_set_domain_loaded_callback(s_ProfilerState->ProfilerHandle, OnDomainLoaded);

		if (ScriptEngine::GetConfig().EnableProfiling)
		{
			mono_profiler_enable_allocations();
			mono_profiler_set_domain_unloaded_callback(s_ProfilerState->ProfilerHandle, OnDomainUnloaded);
			mono_profiler_set_gc_allocation_callback(s_ProfilerState->ProfilerHandle, OnGCAllocationCallback);
			mono_profiler_set_gc_moves_callback(s_ProfilerState->ProfilerHandle, OnGCMovesCallback);
			mono_profiler_set_gc_finalizing_object_callback(s_ProfilerState->ProfilerHandle, OnGCFinalizingObjectCallback);
		}
	}

	void ScriptProfiler::Shutdown()
	{
		// Unregister callbacks
		mono_profiler_set_domain_loaded_callback(s_ProfilerState->ProfilerHandle, NULL);

		if (ScriptEngine::GetConfig().EnableProfiling)
		{
			mono_profiler_set_domain_unloaded_callback(s_ProfilerState->ProfilerHandle, NULL);
			mono_profiler_set_gc_allocation_callback(s_ProfilerState->ProfilerHandle, NULL);
			mono_profiler_set_gc_moves_callback(s_ProfilerState->ProfilerHandle, NULL);
			mono_profiler_set_gc_finalizing_object_callback(s_ProfilerState->ProfilerHandle, NULL);
		}

		hdelete s_ProfilerState;
		s_ProfilerState = nullptr;
	}

	void ScriptProfiler::OnDomainLoaded(MonoProfiler* profiler, MonoDomain* domain)
	{
		if (s_ProfilerState->ReloadingDomain)
			s_ProfilerState->ReloadingDomain = false;

		// NOTE(Peter): Unsure if we have to do this every time we load a domain
		s_ProfilerState->GCStats.MaxGeneration = mono_gc_max_generation();
	}

	void ScriptProfiler::OnDomainUnloaded(MonoProfiler* profiler, MonoDomain* domain)
	{
		s_ProfilerState->GCStats.TotalAllocations = 0;
		s_ProfilerState->GCStats.TotalDeallocations = 0;
		s_ProfilerState->GCStats.AllocatedObjects.clear();

		s_ProfilerState->ReloadingDomain = true;
	}

	void ScriptProfiler::OnGCAllocationCallback(MonoProfiler* profiler, MonoObject* obj)
	{
		if (s_ProfilerState->ReloadingDomain)
			return;

#ifdef HZ_IGNORE_EXTERNAL_ALLOCATIONS

		if (mono_object_get_domain(obj) != ScriptEngine::GetScriptDomain())
			return;

		if (mono_object_get_vtable(obj) == nullptr)
			return;

		ManagedClass* managedClass = ScriptCache::GetMonoObjectClass(obj);

		if (managedClass == nullptr || Utils::String::ToLowerCopy(managedClass->FullName).find("system.") != std::string::npos)
			return;
#endif

		s_ProfilerState->GCStats.TotalAllocations++;
		s_ProfilerState->GCStats.AllocatedObjects.push_back(obj);
	}

	void ScriptProfiler::OnGCMovesCallback(MonoProfiler* profiler, MonoObject* const* objects, uint64_t count)
	{
		if (s_ProfilerState->ReloadingDomain)
			return;

		for (uint64_t i = 0; i < count; i += 2)
		{
			MonoObject* src = objects[i];
			MonoObject* dst = objects[i + 1];
			bool wasBeingTracked = Utils::RemoveIf(s_ProfilerState->GCStats.AllocatedObjects, [src](const MonoObject* other) { return src == other; });

			if (wasBeingTracked)
				s_ProfilerState->GCStats.AllocatedObjects.push_back(dst);
		}
	}

	void ScriptProfiler::OnGCFinalizingObjectCallback(MonoProfiler* profiler, MonoObject* obj)
	{
		if (s_ProfilerState->ReloadingDomain)
			return;

		Utils::RemoveIf(s_ProfilerState->GCStats.AllocatedObjects, [obj](const MonoObject* other) { return obj == other; });
		s_ProfilerState->GCStats.TotalDeallocations++;
	}

	const ScriptProfiler::GCStats& ScriptProfiler::GetGCStats() { return s_ProfilerState->GCStats; }

}

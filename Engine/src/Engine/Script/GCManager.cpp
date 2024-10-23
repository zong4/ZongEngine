#include "pch.h"
#include "GCManager.h"
#include "ScriptCache.h"

#include "Engine/Debug/Profiler.h"

#include <mono/metadata/object.h>
#include <mono/metadata/mono-gc.h>
#include <mono/metadata/profiler.h>

namespace Hazel {

	using ReferenceMap = std::unordered_map<GCHandle, MonoObject*>;

	struct GCState
	{
		ReferenceMap StrongReferences;
		ReferenceMap WeakReferences;
	};

	static GCState* s_GCState = nullptr;
	void GCManager::Init()
	{
		ZONG_CORE_ASSERT(!s_GCState, "Trying to initialize GC Manager multiple times!");

		s_GCState = hnew GCState();
	}

	void GCManager::Shutdown()
	{
		if (s_GCState->StrongReferences.size() > 0)
		{
			ZONG_CORE_ERROR("ScriptEngine", "Memory leak detected!");
			ZONG_CORE_ERROR("ScriptEngine", "Not all GCHandles have been cleaned up!");

			for (auto[handle, monoObject] : s_GCState->StrongReferences)
				mono_gchandle_free_v2(handle);

			s_GCState->StrongReferences.clear();
		}

		if (s_GCState->WeakReferences.size() > 0)
		{
			ZONG_CORE_ERROR("ScriptEngine", "Memory leak detected!");
			ZONG_CORE_ERROR("ScriptEngine", "Not all GCHandles have been cleaned up!");

			for (auto [handle, monoObject] : s_GCState->WeakReferences)
				mono_gchandle_free_v2(handle);

			s_GCState->WeakReferences.clear();
		}

		// Collect any leftover garbage
		mono_gc_collect(mono_gc_max_generation());
		while (mono_gc_pending_finalizers());

		hdelete s_GCState;
		s_GCState = nullptr;
	}

	void GCManager::CollectGarbage(bool blockUntilFinalized)
	{
		ZONG_PROFILE_FUNC();
		ZONG_CORE_INFO_TAG("ScriptEngine", "Collecting garbage...");
		mono_gc_collect(mono_gc_max_generation());
		if (blockUntilFinalized)
		{
			while (mono_gc_pending_finalizers());
			ZONG_CORE_INFO_TAG("ScriptEngine", "GC Finished...");
		}
	}

	GCHandle GCManager::CreateObjectReference(MonoObject* managedObject, bool weakReference, bool pinned, bool track)
	{
		GCHandle handle = weakReference ? mono_gchandle_new_weakref_v2(managedObject, pinned) : mono_gchandle_new_v2(managedObject, pinned);
		ZONG_CORE_ASSERT(handle, "Failed to retrieve valid GC Handle!");

		if (track)
		{
			if (weakReference)
				s_GCState->WeakReferences[handle] = managedObject;
			else
				s_GCState->StrongReferences[handle] = managedObject;
		}

		return handle;
	}

	bool GCManager::IsHandleValid(GCHandle handle)
	{
		if (handle == nullptr)
			return false;

		MonoObject* obj = mono_gchandle_get_target_v2(handle);

		if (obj == nullptr)
			return false;

		if (mono_object_get_vtable(obj) == nullptr)
			return false;

		return true;
	}

	MonoObject* GCManager::GetReferencedObject(GCHandle handle)
	{
		MonoObject* obj = mono_gchandle_get_target_v2(handle);
		if (obj == nullptr || mono_object_get_vtable(obj) == nullptr)
			return nullptr;
		return obj;
	}

	void GCManager::ReleaseObjectReference(GCHandle handle)
	{
		if (mono_gchandle_get_target_v2(handle) != nullptr)
		{
			mono_gchandle_free_v2(handle);
		}
		else
		{
			ZONG_CORE_ERROR_TAG("ScriptEngine", "Tried to release an object reference using an invalid handle!");
			return;
		}

		if (s_GCState->StrongReferences.find(handle) != s_GCState->StrongReferences.end())
			s_GCState->StrongReferences.erase(handle);

		if (s_GCState->WeakReferences.find(handle) != s_GCState->WeakReferences.end())
			s_GCState->WeakReferences.erase(handle);
	}

}

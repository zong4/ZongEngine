#include "hzpch.h"
#include "ScriptEngineDebugPanel.h"
#include "Engine/Script/ScriptUtils.h"
#include "Engine/Script/ScriptProfiler.h"

#include "Engine/Core/Application.h"
#include "Engine/ImGui/ImGui.h"
#include "Engine/Core/Hash.h"

#include <mono/metadata/mono-gc.h>

namespace Hazel {

	static std::vector<AssemblyMetadata> s_LoadedAssembliesMetadata;

	ScriptEngineDebugPanel::ScriptEngineDebugPanel()
	{
	}

	ScriptEngineDebugPanel::~ScriptEngineDebugPanel()
	{
	}

	void ScriptEngineDebugPanel::OnProjectChanged(const Ref<Project>& project)
	{
		s_LoadedAssembliesMetadata.clear();

		const auto& hazelCoreAssembly = ScriptEngine::GetCoreAssemblyInfo();
		const auto& appAssembly = ScriptEngine::GetAppAssemblyInfo();
		s_LoadedAssembliesMetadata.push_back(hazelCoreAssembly->Metadata);
		s_LoadedAssembliesMetadata.push_back(appAssembly->Metadata);

		for (const auto& referencedAssemblyMetadata : hazelCoreAssembly->ReferencedAssemblies)
			s_LoadedAssembliesMetadata.push_back(referencedAssemblyMetadata);

		for (const auto& referencedAssemblyMetadata : appAssembly->ReferencedAssemblies)
		{
			auto it = std::find_if(s_LoadedAssembliesMetadata.begin(), s_LoadedAssembliesMetadata.end(), [&referencedAssemblyMetadata](const AssemblyMetadata& other)
			{
				return referencedAssemblyMetadata == other;
			});

			if (it != s_LoadedAssembliesMetadata.end())
				continue;

			s_LoadedAssembliesMetadata.push_back(referencedAssemblyMetadata);
		}

	}

	void ScriptEngineDebugPanel::OnImGuiRender(bool& open)
	{
		if (ImGui::Begin("Script Engine Debug", &open))
		{
			const auto& gcStats = ScriptProfiler::GetGCStats();

			if (UI::PropertyGridHeader("Memory"))
			{
				ImGui::Text("Available Memory (MB): %d", mono_gc_get_heap_size() / 1024 / 1024);
				ImGui::Text("Used Memory (KB): %d", mono_gc_get_used_size() / 1024);
				ImGui::Text("GC Generations: %d", gcStats.MaxGeneration);

				for (uint32_t generation = 0; generation < gcStats.MaxGeneration; generation++)
				{
					std::string generationName = fmt::format("Generation {0}", generation);
					if (UI::PropertyGridHeader(generationName, false))
					{
						ImGui::Text("Collection Count: %d", mono_gc_collection_count(generation));
						UI::EndTreeNode();
					}
				}

				UI::EndTreeNode();
			}

			if (UI::PropertyGridHeader(fmt::format("Loaded Assemblies ({0})", s_LoadedAssembliesMetadata.size()), false))
			{
				for (const auto& referencedAssembly : s_LoadedAssembliesMetadata)
					ImGui::Text("%s (%d.%d.%d.%d)", referencedAssembly.Name.c_str(), referencedAssembly.MajorVersion, referencedAssembly.MinorVersion, referencedAssembly.BuildVersion, referencedAssembly.RevisionVersion);

				UI::EndTreeNode();
			}

			if (UI::PropertyGridHeader("Cache", false))
			{
				const auto& cachedClasses = ScriptCache::GetCachedClasses();
				const auto& cachedFields = ScriptCache::GetCachedFields();
				const auto& cachedMethods = ScriptCache::GetCachedMethods();

				ImGui::Text("Total Classes: %d", cachedClasses.size());
				ImGui::Text("Total Fields: %d", cachedFields.size());
				ImGui::Text("Total Methods: %d", cachedMethods.size());

				for (const auto& [classID, managedClass] : cachedClasses)
				{
					const std::string label = fmt::format("{0} ({1})", managedClass.FullName, managedClass.ID);
					size_t unmanagedSize = sizeof(uint32_t) + managedClass.FullName.size() + (sizeof(uint32_t) * managedClass.Fields.size()) + (sizeof(uint32_t) * managedClass.Methods.size()) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(MonoClass*);
					if (UI::PropertyGridHeader(label.c_str(), false))
					{
						ImGui::Text("Unmanaged Size: %d", unmanagedSize);
						ImGui::Text("Managed Size: %d", managedClass.Size);
						UI::EndTreeNode();
					}
				}

				UI::EndTreeNode();
			}

			// NOTE(Peter): Disabled for the time since it doesn't fully work
			//if (ScriptEngine::GetConfig().EnableProfiling && UI::PropertyGridHeader("Allocated Objects", false))
			//{
			//	ImGui::Text("Total Allocations: %d", gcStats.TotalAllocations);
			//	ImGui::Text("Total Deallocations: %d", gcStats.TotalDeallocations);
			//	ImGui::Text("Object Count: %d", gcStats.AllocatedObjects.size());

			//	for (const auto obj : gcStats.AllocatedObjects)
			//	{
			//		if (!mono_monitor_try_enter(obj, 5))
			//			continue;

			//		if (mono_object_get_vtable(obj) == nullptr)
			//		{
			//			mono_monitor_exit(obj);
			//			continue;
			//		}

			//		MonoClass* klass = mono_object_get_class(obj);
			//		if (klass == nullptr)
			//		{
			//			mono_monitor_exit(obj);
			//			continue;
			//		}

			//		MonoType* klassType = mono_class_get_type(klass);

			//		std::string className = ScriptUtils::ResolveMonoClassName(klass);
			//		std::string objectName = fmt::format("{0} ({1})", (void*)obj, className);

			//		if (UI::PropertyGridHeader(objectName, false))
			//		{
			//			uint32_t size = mono_object_get_size(obj);
			//			ImGui::Text("Size: %d", size);

			//			{
			//				ImGui::Text("Class Info:");
			//				UI::Draw::Underline();
			//			
			//				UI::BeginPropertyGrid();
			//			
			//				uint32_t classID = Hash::GenerateFNVHash(className);
			//				ImGui::Text("ID: %d", classID);
			//				ImGui::Text("Name: %s", className.c_str());

			//				// TODO: Draw field info
			//				// TODO: Draw method info?
			//			
			//				UI::EndPropertyGrid();
			//			}

			//			UI::EndTreeNode();
			//		}

			//		mono_monitor_exit(obj);
			//	}

			//	UI::EndTreeNode();
			//}
		}

		ImGui::End();
	}

}

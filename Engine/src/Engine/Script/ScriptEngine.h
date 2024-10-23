#pragma once

#include "ScriptAsset.h"
#include "ScriptCache.h"
#include "ScriptTypes.h"

#include "Engine/Core/Hash.h"
#include "Engine/Debug/Profiler.h"
#include "Engine/Scene/Components.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Project/Project.h"
#include "Engine/Utilities/FileSystem.h"

#include <filewatch/FileWatch.hpp>
#include <type_traits>
#include <utility>

#define HAZEL_CORE_ASSEMBLY_INDEX 0
#define HAZEL_APP_ASSEMBLY_INDEX HAZEL_CORE_ASSEMBLY_INDEX + 1
#define HAZEL_MAX_ASSEMBLIES (size_t)2

extern "C" {
	typedef struct _MonoDomain MonoDomain;
}

namespace Hazel {

	using ScriptEntityMap = std::unordered_map<UUID, std::vector<UUID>>;
	using ScriptInstanceMap = std::unordered_map<UUID, GCHandle>;
	using AssembliesArray = std::array<Ref<AssemblyInfo>, HAZEL_MAX_ASSEMBLIES>;

	struct ScriptEngineConfig
	{
		std::filesystem::path CoreAssemblyPath;
		bool EnableDebugging;
		bool EnableProfiling;
	};

	class ScriptEngine
	{
	public:
		static void Init(const ScriptEngineConfig& config);
		static void Shutdown();

		static void InitializeRuntime(bool skipInitializedEntities = true);
		static void ShutdownRuntime();
		static void ShutdownRuntimeInstance(Entity entity);

		static void OnProjectChanged(Ref<Project> inProject);

		static bool LoadAppAssembly();
		static bool LoadAppAssemblyRuntime(Buffer appAssemblyData);
		static bool ReloadAppAssembly(const bool scheduleReload = false);
		static bool ShouldReloadAppAssembly();
		static void UnloadAppAssembly();

		static void SetSceneContext(const Ref<Scene>& scene, const Ref<SceneRenderer>& sceneRenderer);
		static Ref<Scene> GetSceneContext();
		static Ref<SceneRenderer> GetSceneRenderer();

		static void InitializeScriptEntity(Entity entity);
		static void RuntimeInitializeScriptEntity(Entity entity, bool skipInitializedEntities = true);
		static void DuplicateScriptInstance(Entity entity, Entity targetEntity);
		static void ShutdownScriptEntity(Entity entity, bool erase = true);

		static Ref<FieldStorageBase> GetFieldStorage(Entity entity, uint32_t fieldID);

		static void InitializeRuntimeDuplicatedEntities();

		// NOTE(Peter): Pass false as the second parameter if you don't care if OnCreate has been called yet
		static bool IsEntityInstantiated(Entity entity, bool checkOnCreateCalled = true);
		static GCHandle GetEntityInstance(UUID entityID);
		static const std::unordered_map<UUID, GCHandle>& GetEntityInstances();

		static uint32_t GetScriptClassIDFromComponent(const ScriptComponent& sc);
		static bool IsModuleValid(AssetHandle scriptAssetHandle);

		static MonoDomain* GetScriptDomain();

		template<typename... TConstructorArgs>
		static MonoObject* CreateManagedObject(const std::string& className, TConstructorArgs&&... args)
		{
			return CreateManagedObject_Internal(ScriptCache::GetManagedClassByID(HZ_SCRIPT_CLASS_ID(className)), std::forward<TConstructorArgs>(args)...);
		}

		template<typename... TConstructorArgs>
		static MonoObject* CreateManagedObject(uint32_t classID, TConstructorArgs&&... args)
		{
			return CreateManagedObject_Internal(ScriptCache::GetManagedClassByID(classID), std::forward<TConstructorArgs>(args)...);
		}

		static GCHandle CreateObjectReference(MonoObject* obj, bool weakReference)
		{
			return GCManager::CreateObjectReference(obj, weakReference);
		}

		static void ReleaseObjectReference(GCHandle instanceID)
		{
			GCManager::ReleaseObjectReference(instanceID);
		}

		static Ref<AssemblyInfo> GetCoreAssemblyInfo();
		static Ref<AssemblyInfo> GetAppAssemblyInfo();

		static const ScriptEngineConfig& GetConfig();

		template<typename... TArgs>
		static void CallMethod(MonoObject* managedObject, const std::string& methodName, TArgs&&... args)
		{
			HZ_PROFILE_SCOPE_DYNAMIC(methodName.c_str());

			if (managedObject == nullptr)
			{
				HZ_CORE_WARN_TAG("ScriptEngine", "Attempting to call method {0} on an invalid instance!", methodName);
				return;
			}

			constexpr size_t argsCount = sizeof...(args);

			ManagedClass* clazz = ScriptCache::GetMonoObjectClass(managedObject);
			if (clazz == nullptr)
			{
				HZ_CORE_ERROR_TAG("ScriptEngine", "Failed to find ManagedClass!");
				return;
			}

			ManagedMethod* method = ScriptCache::GetSpecificManagedMethod(clazz, methodName, argsCount);
			if (method == nullptr)
			{
				HZ_CORE_ERROR_TAG("ScriptEngine", "Failed to find a C# method called {0} with {1} parameters", methodName, argsCount);
				return;
			}

			if constexpr (argsCount > 0)
			{
				const void* data[] = { &args... };
				CallMethod(managedObject, method, data);
			}
			else
			{
				CallMethod(managedObject, method, nullptr);
			}
		}

		template<typename... TArgs>
		static void CallMethod(GCHandle instance, const std::string& methodName, TArgs&&... args)
		{
			if (instance == nullptr)
			{
				HZ_CORE_WARN_TAG("ScriptEngine", "Attempting to call method {0} on an invalid instance!", methodName);
				return;
			}

			CallMethod(GCManager::GetReferencedObject(instance), methodName, std::forward<TArgs>(args)...);
		}
		
	private:
		static void InitMono();
		static void ShutdownMono();

		static MonoAssembly* LoadMonoAssembly(const std::filesystem::path& assemblyPath);
		static MonoAssembly* LoadMonoAssemblyRuntime(Buffer assemblyData);
		static bool LoadCoreAssembly();
		static void UnloadAssembly(Ref<AssemblyInfo> assemblyInfo);

		static void LoadReferencedAssemblies(const Ref<AssemblyInfo>& assemblyInfo);
		static AssemblyMetadata GetMetadataForImage(MonoImage* image);
		static std::vector<AssemblyMetadata> GetReferencedAssembliesMetadata(MonoImage* image);

		static MonoObject* CreateManagedObject(ManagedClass* managedClass);
		static void InitRuntimeObject(MonoObject* monoObject);
		static void CallMethod(MonoObject* monoObject, ManagedMethod* managedMethod, const void** parameters);

		static void OnAppAssemblyFolderChanged(const std::filesystem::path& filepath, filewatch::Event eventType);

	private:
		template<typename... TConstructorArgs>
		static MonoObject* CreateManagedObject_Internal(ManagedClass* managedClass, TConstructorArgs&&... args)
		{
			HZ_PROFILE_SCOPE_DYNAMIC(managedClass->FullName.c_str());

			if (managedClass == nullptr)
			{
				HZ_CORE_ERROR_TAG("ScriptEngine", "Attempting to create managed object with a null class!");
				return nullptr;
			}

			if (managedClass->IsAbstract)
				return nullptr;

			MonoObject* obj = CreateManagedObject(managedClass);

			if (managedClass->IsStruct)
				return obj;

			//if (ManagedType::FromClass(managedClass).IsValueType())
			//	return obj;

			constexpr size_t argsCount = sizeof...(args);
			ManagedMethod* ctor = ScriptCache::GetSpecificManagedMethod(managedClass, ".ctor", argsCount);

			InitRuntimeObject(obj);

			if constexpr (argsCount > 0)
			{
				if (ctor == nullptr)
				{
					HZ_CORE_ERROR_TAG("ScriptEngine", "Failed to call constructor with {} parameters for class '{}'.", argsCount, managedClass->FullName);
					return obj;
				}

				const void* data[] = { &args... };
				CallMethod(obj, ctor, data);
			}

			return obj;
		}

	private:
		friend class ScriptCache;
		friend class ScriptUtils;
		friend class Application;
	};

}

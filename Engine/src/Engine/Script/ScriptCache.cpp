#include "pch.h"
#include "ScriptCache.h"

#include "ScriptUtils.h"
#include "ScriptEngine.h"
#include "CSharpInstanceInspector.h"

#include "Engine/Asset/AssetManager.h"
#include "Engine/Core/Application.h"
#include "Engine/Core/Hash.h"
#include "Engine/Project/Project.h"
#include "Engine/Utilities/StringUtils.h"

#include <mono/metadata/assembly.h>
#include <mono/metadata/class.h>
#include <mono/metadata/object.h>
#include <mono/metadata/attrdefs.h>
#include <mono/metadata/tokentype.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/appdomain.h>

#include <yaml-cpp/yaml.h>

namespace Hazel {

	struct Cache
	{
		std::unordered_map<uint32_t, ManagedClass> Classes;
		std::unordered_map<uint32_t, FieldInfo> Fields;
		std::unordered_map<uint32_t, std::vector<ManagedMethod>> Methods;
	};

	static Cache* s_Cache = nullptr;
	void ScriptCache::Init()
	{
		ZONG_CORE_ASSERT(!s_Cache, "Trying to initialize ScriptCache multiple times!");
		s_Cache = hnew Cache();

		CacheCoreClasses();
	}

	void ScriptCache::CacheClass(std::string_view className, MonoClass* monoClass)
	{
		MonoType* classType = mono_class_get_type(monoClass);
		ManagedClass managedClass;
		managedClass.FullName = className;
		managedClass.ID = Hash::GenerateFNVHash(managedClass.FullName);

		int alignment = 0;
		managedClass.Size = mono_type_size(classType, &alignment);
		managedClass.Class = monoClass;
		s_Cache->Classes[managedClass.ID] = managedClass;
		if (managedClass.FullName.find("Hazel.") != std::string::npos)
		{
			Ref<AssemblyInfo> coreAssembly = ScriptEngine::GetCoreAssemblyInfo();
			ScriptCache::CacheClassMethods(coreAssembly, managedClass);
			ScriptCache::CacheClassFields(coreAssembly, managedClass);
			ScriptCache::CacheClassProperties(coreAssembly, managedClass);
		}
	}

	void ScriptCache::CacheCoreClasses()
	{
		if (s_Cache == nullptr)
			return;
#define CACHE_CORELIB_CLASS(name) CacheClass("System." name, mono_class_from_name(mono_get_corlib(), "System", name))

		CACHE_CORELIB_CLASS("Object");
		CACHE_CORELIB_CLASS("ValueType");
		CACHE_CORELIB_CLASS("Boolean");
		CACHE_CORELIB_CLASS("SByte");
		CACHE_CORELIB_CLASS("Int16");
		CACHE_CORELIB_CLASS("Int32");
		CACHE_CORELIB_CLASS("Int64");
		CACHE_CORELIB_CLASS("Byte");
		CACHE_CORELIB_CLASS("UInt16");
		CACHE_CORELIB_CLASS("UInt32");
		CACHE_CORELIB_CLASS("UInt64");
		CACHE_CORELIB_CLASS("Single");
		CACHE_CORELIB_CLASS("Double");
		CACHE_CORELIB_CLASS("Char");
		CACHE_CORELIB_CLASS("String");
		CacheClass("System.Diagnostics.StackTrace", mono_class_from_name(mono_get_corlib(), "System.Diagnostics", "StackTrace"));

#define CACHE_HAZEL_CORE_CLASS(name) CacheClass("Hazel." name, mono_class_from_name(ScriptEngine::GetCoreAssemblyInfo()->AssemblyImage, "Hazel", name))

		CACHE_HAZEL_CORE_CLASS("ShowInEditorAttribute");
		CACHE_HAZEL_CORE_CLASS("HideFromEditorAttribute");
		CACHE_HAZEL_CORE_CLASS("ClampValueAttribute");

		CACHE_HAZEL_CORE_CLASS("AssetHandle");
		CACHE_HAZEL_CORE_CLASS("Vector2");
		CACHE_HAZEL_CORE_CLASS("Vector3");
		CACHE_HAZEL_CORE_CLASS("Vector4");
		CACHE_HAZEL_CORE_CLASS("Entity");
		CACHE_HAZEL_CORE_CLASS("IEditorRunnable");
		CACHE_HAZEL_CORE_CLASS("Prefab");
		CACHE_HAZEL_CORE_CLASS("Mesh");
		CACHE_HAZEL_CORE_CLASS("StaticMesh");
		CACHE_HAZEL_CORE_CLASS("Material");
		CACHE_HAZEL_CORE_CLASS("Shape");
		CACHE_HAZEL_CORE_CLASS("BoxShape");
		CACHE_HAZEL_CORE_CLASS("SphereShape");
		CACHE_HAZEL_CORE_CLASS("CapsuleShape");
		CACHE_HAZEL_CORE_CLASS("ConvexMeshShape");
		CACHE_HAZEL_CORE_CLASS("TriangleMeshShape");
		CACHE_HAZEL_CORE_CLASS("Collider");
		CACHE_HAZEL_CORE_CLASS("SceneQueryHit");
		CACHE_HAZEL_CORE_CLASS("PhysicsMaterial");
		CACHE_HAZEL_CORE_CLASS("RaycastHit2D");
		CACHE_HAZEL_CORE_CLASS("Texture2D");
		CACHE_HAZEL_CORE_CLASS("Scene");
	}

	void ScriptCache::Shutdown()
	{
		ClearCache();

		hdelete s_Cache;
		s_Cache = nullptr;
	}

	void ScriptCache::ClearCache()
	{
		if (s_Cache == nullptr)
			return;

		// Remove all ScriptAssets
		if (!Application::IsRuntime())
		{
			for (const auto& [classID, classInfo] : s_Cache->Classes)
			{
				AssetHandle scriptAssetHandle = AssetHandle(Hash::GenerateFNVHash(classInfo.FullName));
				Project::GetEditorAssetManager()->RemoveAsset(scriptAssetHandle);
			}
		}

		s_Cache->Fields.clear();
		s_Cache->Methods.clear();
		s_Cache->Classes.clear();
	}

	static void BuildClassMetadata(Ref<AssemblyInfo>& assemblyInfo, MonoClass* monoClass)
	{
		ZONG_CORE_VERIFY(monoClass);

		const std::string fullName = ScriptUtils::ResolveMonoClassName(monoClass);

		// C# adds a .<PrivateImplementationDetails> class for some reason?
		if (fullName.find("<PrivateImpl") != std::string::npos)
			return;

		uint32_t classID = Hash::GenerateFNVHash(fullName);
		ManagedClass& managedClass = s_Cache->Classes[classID];
		managedClass.FullName = fullName;
		managedClass.ID = classID;
		managedClass.Class = monoClass;
		uint32_t classFlags = mono_class_get_flags(monoClass);
		managedClass.IsAbstract = classFlags & MONO_TYPE_ATTR_ABSTRACT;
		managedClass.IsStruct = mono_class_is_valuetype(monoClass);

		MonoClass* parentClass = mono_class_get_parent(monoClass);
		if (parentClass != nullptr && parentClass != ZONG_CACHED_CLASS_RAW("System.Object"))
		{
			std::string parentName = ScriptUtils::ResolveMonoClassName(parentClass);
			managedClass.ParentID = Hash::GenerateFNVHash(parentName);
		}

		assemblyInfo->Classes.push_back(managedClass.ID);
	}

	void ScriptCache::GenerateCacheForAssembly(Ref<AssemblyInfo> assemblyInfo)
	{
		const MonoTableInfo* tableInfo = mono_image_get_table_info(assemblyInfo->AssemblyImage, MONO_TABLE_TYPEDEF);
		int32_t tableRowCount = mono_table_info_get_rows(tableInfo);
		for (int32_t i = 1; i < tableRowCount; i++)
		{
			MonoClass* monoClass = mono_class_get(assemblyInfo->AssemblyImage, (i + 1) | MONO_TOKEN_TYPE_DEF);
			BuildClassMetadata(assemblyInfo, monoClass);
		}

		// Process fields and properties after all classes have been parsed.
		for (auto classID : assemblyInfo->Classes)
		{
			ManagedClass& managedClass = s_Cache->Classes.at(classID);

			CacheClassMethods(assemblyInfo, managedClass);

			MonoObject* tempInstance = ScriptEngine::CreateManagedObject_Internal(&managedClass);
			if (tempInstance == nullptr)
				continue;

			CacheClassFields(assemblyInfo, managedClass);
			CacheClassProperties(assemblyInfo, managedClass);

			if (mono_class_is_subclass_of(managedClass.Class, ZONG_CACHED_CLASS_RAW("Hazel.Entity"), false))
			{
				AssetHandle handle = AssetManager::CreateMemoryOnlyAssetWithHandle<ScriptAsset>(Hash::GenerateFNVHash(managedClass.FullName), classID);
#ifndef ZONG_DIST
				// TODO(Yan): fix this for runtime
				if (!Application::IsRuntime())
					Project::GetEditorAssetManager()->GetMutableMetadata(handle).FilePath = managedClass.FullName;
#endif
			}
		}

		for (auto classID : assemblyInfo->Classes)
		{
			ManagedClass& managedClass = s_Cache->Classes.at(classID);

			if (!mono_class_is_subclass_of(managedClass.Class, ZONG_CACHED_CLASS_RAW("Hazel.Entity"), false))
				continue;

			MonoObject* tempInstance = ScriptEngine::CreateManagedObject_Internal(&managedClass);

			for (auto fieldID : managedClass.Fields)
			{
				FieldInfo& fieldInfo = s_Cache->Fields[fieldID];
				if (!fieldInfo.IsArray())
				{
					fieldInfo.DefaultValueBuffer = ScriptUtils::GetFieldValue(tempInstance, fieldInfo.Name, fieldInfo.Type, fieldInfo.IsProperty);
				}
				else
				{
					MonoArray* arr = (MonoArray*)ScriptUtils::GetFieldValueObject(tempInstance, fieldInfo.Name, fieldInfo.IsProperty);

					if (arr != nullptr)
					{
						fieldInfo.DefaultValueBuffer.Allocate(mono_array_length(arr) * fieldInfo.Size);
						fieldInfo.DefaultValueBuffer.ZeroInitialize();
					}
				}
			}
		}
	}

	ManagedClass* ScriptCache::GetManagedClassByName(const std::string& className)
	{
		if (s_Cache == nullptr)
			return nullptr;

		uint32_t classID = Hash::GenerateFNVHash(className);
		if (s_Cache->Classes.find(classID) == s_Cache->Classes.end())
			return nullptr;

		return &s_Cache->Classes[classID];
	}

	ManagedClass* ScriptCache::GetManagedClassByID(uint32_t classID)
	{
		if (s_Cache == nullptr)
			return nullptr;

		if (s_Cache->Classes.find(classID) == s_Cache->Classes.end())
			return nullptr;

		return &s_Cache->Classes[classID];
	}

	ManagedClass* ScriptCache::GetManagedClass(MonoClass* monoClass)
	{
		if (s_Cache == nullptr)
			return nullptr;

		if (monoClass == nullptr)
			return nullptr;

		return GetManagedClassByName(ScriptUtils::ResolveMonoClassName(monoClass));
	}

	ManagedClass* ScriptCache::GetMonoObjectClass(MonoObject* monoObject)
	{
		ZONG_PROFILE_FUNC();

		if (s_Cache == nullptr)
			return nullptr;

		MonoClass* objectClass = mono_object_get_class(monoObject);
		if (objectClass == nullptr)
			return nullptr;

		return GetManagedClassByName(ScriptUtils::ResolveMonoClassName(objectClass));
	}

	FieldInfo* ScriptCache::GetFieldByID(uint32_t fieldID)
	{
		if (s_Cache == nullptr)
			return nullptr;

		if (s_Cache->Fields.find(fieldID) == s_Cache->Fields.end())
			return nullptr;
		return &s_Cache->Fields.at(fieldID);
	}

	FieldInfo* ScriptCache::GetFieldByName(const ManagedClass* managedClass, const std::string& fieldName)
	{
		uint32_t fieldID = Hash::GenerateFNVHash(managedClass->FullName + ":" + fieldName);
		if (s_Cache->Fields.find(fieldID) == s_Cache->Fields.end())
			return nullptr;

		return &s_Cache->Fields[fieldID];
	}

	MonoClass* ScriptCache::GetFieldTypeClass(FieldType fieldType)
	{
		switch (fieldType)
		{
			case FieldType::Bool: return ZONG_CACHED_CLASS("System.Bool")->Class;
			case FieldType::Int8: return ZONG_CACHED_CLASS("System.SByte")->Class;
			case FieldType::Int16: return ZONG_CACHED_CLASS("System.Int16")->Class;
			case FieldType::Int32: return ZONG_CACHED_CLASS("System.Int32")->Class;
			case FieldType::Int64: return ZONG_CACHED_CLASS("System.Int64")->Class;
			case FieldType::UInt8: return ZONG_CACHED_CLASS("System.Byte")->Class;
			case FieldType::UInt16: return ZONG_CACHED_CLASS("System.UInt16")->Class;
			case FieldType::UInt32: return ZONG_CACHED_CLASS("System.UInt32")->Class;
			case FieldType::UInt64: return ZONG_CACHED_CLASS("System.UInt64")->Class;
			case FieldType::Float: return ZONG_CACHED_CLASS("System.Single")->Class;
			case FieldType::Double: return ZONG_CACHED_CLASS("System.Double")->Class;
			case FieldType::String: return ZONG_CACHED_CLASS("System.String")->Class;
			case FieldType::Vector2: return ZONG_CACHED_CLASS("Hazel.Vector2")->Class;
			case FieldType::Vector3: return ZONG_CACHED_CLASS("Hazel.Vector3")->Class;
			case FieldType::Vector4: return ZONG_CACHED_CLASS("Hazel.Vector4")->Class;
			case FieldType::AssetHandle: return ZONG_CACHED_CLASS("Hazel.AssetHandle")->Class;
			case FieldType::Prefab: return ZONG_CACHED_CLASS("Hazel.Prefab")->Class;
			case FieldType::Entity: return ZONG_CACHED_CLASS("Hazel.Entity")->Class;
			case FieldType::Mesh: return ZONG_CACHED_CLASS("Hazel.Mesh")->Class;
			case FieldType::StaticMesh: return ZONG_CACHED_CLASS("Hazel.StaticMesh")->Class;
			case FieldType::Material: return ZONG_CACHED_CLASS("Hazel.Material")->Class;
			case FieldType::PhysicsMaterial: return ZONG_CACHED_CLASS("Hazel.PhysicsMaterial")->Class;
			case FieldType::Texture2D: return ZONG_CACHED_CLASS("Hazel.Texture2D")->Class;
			case FieldType::Scene: return ZONG_CACHED_CLASS("Hazel.Scene")->Class;
		}

		return nullptr;
	}

	ManagedMethod* ScriptCache::GetManagedMethod(ManagedClass* managedClass, const std::string& name, bool ignoreParent /*= false*/)
	{
		ZONG_PROFILE_FUNC();

		if (s_Cache == nullptr)
			return nullptr;

		if (managedClass == nullptr)
		{
			ZONG_CORE_ERROR_TAG("ScriptEngine", "Attempting to get method {0} from a nullptr class!", name);
			return nullptr;
		}

		uint32_t methodID = Hash::GenerateFNVHash(fmt::format("{0}:{1}", managedClass->FullName, name));
		if (s_Cache->Methods.find(methodID) != s_Cache->Methods.end())
			return &s_Cache->Methods.at(methodID)[0];

		if (!ignoreParent && managedClass->ParentID != 0)
			return GetManagedMethod(&s_Cache->Classes.at(managedClass->ParentID), name);

		ZONG_CORE_WARN_TAG("ScriptEngine", "Failed to find method with name: {0} in class {1}", name, managedClass->FullName);
		return nullptr;
	}

	ManagedMethod* ScriptCache::GetSpecificManagedMethod(ManagedClass* managedClass, const std::string& name, uint32_t parameterCount, bool ignoreParent)
	{
		ZONG_PROFILE_FUNC();

		if (s_Cache == nullptr)
			return nullptr;

		if (managedClass == nullptr)
		{
			ZONG_CORE_ERROR_TAG("ScriptEngine", "Attempting to get method {0} from a nullptr class!", name);
			return nullptr;
		}

		ManagedMethod* method = nullptr;

		uint32_t methodID = Hash::GenerateFNVHash(managedClass->FullName + ":" + name);
		if (s_Cache->Methods.find(methodID) != s_Cache->Methods.end())
		{
			for (auto& methodCandiate : s_Cache->Methods.at(methodID))
			{
				if (methodCandiate.ParameterCount == parameterCount)
				{
					method = &methodCandiate;
					break;
				}
			}
		}

		if (method == nullptr && !ignoreParent && managedClass->ParentID != 0)
			method = GetSpecificManagedMethod(&s_Cache->Classes.at(managedClass->ParentID), name, parameterCount);

		if (method == nullptr)
			ZONG_CORE_WARN_TAG("ScriptEngine", "Failed to find method with name: {0} and parameter count: {1} in class {2}", name, parameterCount, managedClass->FullName);

		return method;
	}

	void ScriptCache::CacheClassMethods(Ref<AssemblyInfo> assemblyInfo, ManagedClass& managedClass)
	{
		MonoMethod* monoMethod = nullptr;
		void* methodPtr = 0;
		while ((monoMethod = mono_class_get_methods(managedClass.Class, &methodPtr)) != NULL)
		{
			MonoMethodSignature* sig = mono_method_signature(monoMethod);

			uint32_t flags = mono_method_get_flags(monoMethod, nullptr);
			char* fullName = mono_method_full_name(monoMethod, false);

			ManagedMethod method;
			method.ID = Hash::GenerateFNVHash(fullName);
			method.FullName = fullName;
			method.IsStatic = flags & MONO_METHOD_ATTR_STATIC;
			method.IsVirtual = flags & MONO_METHOD_ATTR_VIRTUAL;
			method.ParameterCount = mono_signature_get_param_count(sig);
			method.Method = monoMethod;

			// NOTE(Peter): We can enable this if we want information about a methods parameters later
			/*if (method.ParameterCount > 0)
			{
				const char** parameterNames = new const char*[method.ParameterCount];
				mono_method_get_param_names(monoMethod, parameterNames);

				MonoType* parameterType = nullptr;
				void* parameterIter = 0;
				uint32_t parameterIndex = 0;
				while ((parameterType = mono_signature_get_params(sig, &parameterIter)) != NULL)
				{
					const char* name = parameterNames[parameterIndex];

					FieldInfo parameter;
					parameter.ID = Hash::GenerateFNVHash(name);
					parameter.FullName = name;
					parameter.Type = ManagedType::FromType(parameterType);
					parameter.Attributes = 0;

					int alignment = 0;
					parameter.Size = mono_type_size(parameterType, &alignment);

					method.Parameters[parameter.ID] = parameter;

					parameterIndex++;
				}

				delete[] parameterNames;
			}*/

			s_Cache->Methods[method.ID].push_back(method);
			managedClass.Methods.push_back(method.ID);

			mono_free(fullName);
		}
	}

	void ScriptCache::CacheClassFields(Ref<AssemblyInfo> assemblyInfo, ManagedClass& managedClass)
	{
		MonoClass* currentClass = managedClass.Class;
		while (currentClass != nullptr)
		{
			std::string className = mono_class_get_name(currentClass);
			std::string classNameSpace = mono_class_get_namespace(currentClass);

			if (classNameSpace.find("Hazel") != std::string::npos && className.find("Entity") != std::string::npos)
			{
				currentClass = nullptr;
				continue;
			}

			MonoClassField* field = nullptr;
			void* fieldPtr = 0;
			while ((field = mono_class_get_fields(currentClass, &fieldPtr)) != NULL)
			{
				std::string name = mono_field_get_name(field);

				// Properties have a backing field called <PropertyName>k__BackingField. We don't want to include those in the class fields list.
				if (name.find("k__BackingField") != std::string::npos)
					continue;

				MonoType* monoType = mono_field_get_type(field);
				FieldType fieldType = ScriptUtils::GetFieldTypeFromMonoType(monoType);

				if (fieldType == FieldType::Void)
					continue;

				MonoCustomAttrInfo* attributes = mono_custom_attrs_from_field(currentClass, field);

				uint32_t fieldID = Hash::GenerateFNVHash(fmt::format("{0}:{1}", managedClass.FullName, name));

				int32_t typeEncoding = mono_type_get_type(monoType);

				FieldInfo& managedField = s_Cache->Fields[fieldID];
				managedField.Name = name;
				managedField.ID = fieldID;
				managedField.Type = fieldType;
				managedField.IsProperty = false;

				if (typeEncoding == MONO_TYPE_ARRAY || typeEncoding == MONO_TYPE_SZARRAY)
					managedField.Flags |= (uint64_t)FieldFlag::IsArray;

				uint32_t visibility = mono_field_get_flags(field) & MONO_FIELD_ATTR_FIELD_ACCESS_MASK;
				switch (visibility)
				{
					case MONO_FIELD_ATTR_PUBLIC:
					{
						managedField.Flags &= ~(uint64_t)FieldFlag::Protected;
						managedField.Flags &= ~(uint64_t)FieldFlag::Private;
						managedField.Flags &= ~(uint64_t)FieldFlag::Internal;
						managedField.Flags |= (uint64_t)FieldFlag::Public;
						break;
					}
					case MONO_FIELD_ATTR_FAMILY:
					{
						managedField.Flags &= ~(uint64_t)FieldFlag::Public;
						managedField.Flags &= ~(uint64_t)FieldFlag::Private;
						managedField.Flags &= ~(uint64_t)FieldFlag::Internal;
						managedField.Flags |= (uint64_t)FieldFlag::Protected;
						break;
					}
					case MONO_FIELD_ATTR_ASSEMBLY:
					{
						managedField.Flags &= ~(uint64_t)FieldFlag::Public;
						managedField.Flags &= ~(uint64_t)FieldFlag::Protected;
						managedField.Flags &= ~(uint64_t)FieldFlag::Private;
						managedField.Flags |= (uint64_t)FieldFlag::Internal;
						break;
					}
					case MONO_FIELD_ATTR_PRIVATE:
					{
						managedField.Flags &= ~(uint64_t)FieldFlag::Public;
						managedField.Flags &= ~(uint64_t)FieldFlag::Protected;
						managedField.Flags &= ~(uint64_t)FieldFlag::Internal;
						managedField.Flags |= (uint64_t)FieldFlag::Private;
						break;
					}
				}

				if (attributes && mono_custom_attrs_has_attr(attributes, GetManagedClassByName("Hazel.ShowInEditorAttribute")->Class))
				{
					managedField.Flags &= ~(uint64_t)FieldFlag::Protected;
					managedField.Flags &= ~(uint64_t)FieldFlag::Internal;
					managedField.Flags &= ~(uint64_t)FieldFlag::Private;
					managedField.Flags |= (uint64_t)FieldFlag::Public;

					MonoObject* attrib = mono_custom_attrs_get_attr(attributes, GetManagedClassByName("Hazel.ShowInEditorAttribute")->Class);

					CSharpInstanceInspector inspector(attrib);
					managedField.DisplayName = inspector.GetFieldValue<std::string>("DisplayName");
					bool isReadOnly = inspector.GetFieldValue<bool>("IsReadOnly");

					if (isReadOnly)
						managedField.Flags |= (uint64_t)FieldFlag::ReadOnly;
				}

				if (managedField.IsArray())
				{
					MonoClass* fieldArrayClass = mono_type_get_class(monoType);
					MonoClass* elementClass = mono_class_get_element_class(fieldArrayClass);
					MonoType* elementType = mono_class_get_type(elementClass);

					int align;
					managedField.Size = mono_type_size(elementType, &align);
				}
				else
				{
					int align;
					managedField.Size = mono_type_size(monoType, &align);
				}

				managedClass.Size += managedField.Size;

				if (std::find(managedClass.Fields.begin(), managedClass.Fields.end(), fieldID) == managedClass.Fields.end())
					managedClass.Fields.push_back(managedField.ID);
			}

			currentClass = mono_class_get_parent(currentClass);
		}
	}

	void ScriptCache::CacheClassProperties(Ref<AssemblyInfo> assemblyInfo, ManagedClass& managedClass)
	{
		MonoProperty* prop = nullptr;
		void* propertyIter = 0;
		while ((prop = mono_class_get_properties(managedClass.Class, &propertyIter)) != NULL)
		{
			std::string name = mono_property_get_name(prop);
			uint32_t propertyID = Hash::GenerateFNVHash(fmt::format("{0}:{1}", managedClass.FullName, name));

			uint64_t propertyFlags = 0;
			MonoType* monoType = nullptr;

			// NOTE(Peter): Parsing attributes for properties is not that straightforward.
			//				I'm not sure if this is really the best way to go about it, but it's the method that's the most accurate for C#
			MonoMethod* propertyGetter = mono_property_get_get_method(prop);
			if (propertyGetter != nullptr)
			{
				MonoMethodSignature* sig = mono_method_signature(propertyGetter);
				monoType = mono_signature_get_return_type(sig);

				uint32_t flags = mono_method_get_flags(propertyGetter, nullptr);
				uint32_t visibility = flags & MONO_METHOD_ATTR_ACCESS_MASK;

				switch (visibility)
				{
					case MONO_FIELD_ATTR_PUBLIC:
					{
						propertyFlags &= ~(uint64_t)FieldFlag::Protected;
						propertyFlags &= ~(uint64_t)FieldFlag::Private;
						propertyFlags &= ~(uint64_t)FieldFlag::Internal;
						propertyFlags |= (uint64_t)FieldFlag::Public;
						break;
					}
					case MONO_FIELD_ATTR_FAMILY:
					{
						propertyFlags &= ~(uint64_t)FieldFlag::Public;
						propertyFlags &= ~(uint64_t)FieldFlag::Private;
						propertyFlags &= ~(uint64_t)FieldFlag::Internal;
						propertyFlags |= (uint64_t)FieldFlag::Protected;
						break;
					}
					case MONO_FIELD_ATTR_ASSEMBLY:
					{
						propertyFlags &= ~(uint64_t)FieldFlag::Public;
						propertyFlags &= ~(uint64_t)FieldFlag::Protected;
						propertyFlags &= ~(uint64_t)FieldFlag::Private;
						propertyFlags |= (uint64_t)FieldFlag::Internal;
						break;
					}
					case MONO_FIELD_ATTR_PRIVATE:
					{
						propertyFlags &= ~(uint64_t)FieldFlag::Public;
						propertyFlags &= ~(uint64_t)FieldFlag::Protected;
						propertyFlags &= ~(uint64_t)FieldFlag::Internal;
						propertyFlags |= (uint64_t)FieldFlag::Private;
						break;
					}
				}

				if ((flags & MONO_METHOD_ATTR_STATIC) != 0)
					propertyFlags |= (uint64_t)FieldFlag::Static;
			}

			MonoMethod* propertySetter = mono_property_get_set_method(prop);
			if (propertySetter != nullptr)
			{
				void* paramIter = nullptr;
				MonoMethodSignature* sig = mono_method_signature(propertySetter);
				monoType = mono_signature_get_params(sig, &paramIter);

				uint32_t flags = mono_method_get_flags(propertySetter, nullptr);
				if ((flags & MONO_METHOD_ATTR_ACCESS_MASK) == MONO_METHOD_ATTR_PRIVATE)
					propertyFlags |= (uint64_t)FieldFlag::ReadOnly;

				if ((flags & MONO_METHOD_ATTR_STATIC) != 0)
					propertyFlags |= (uint64_t)FieldFlag::Static;
			}

			if (propertySetter == nullptr && propertyGetter != nullptr)
				propertyFlags |= (uint64_t)FieldFlag::ReadOnly;

			if (monoType == nullptr)
			{
				ZONG_CORE_ERROR_TAG("ScriptEngine", "Failed to retrieve managed type for property '{0}'", name);
				continue;
			}

			MonoCustomAttrInfo* attributes = mono_custom_attrs_from_property(managedClass.Class, prop);

			if (!TypeUtils::ContainsAttribute(attributes, "Hazel.ShowInEditorAttribute"))
				continue;

			FieldInfo& managedProperty = s_Cache->Fields[propertyID];
			managedProperty.Name = name;
			managedProperty.ID = propertyID;
			managedProperty.Type = ScriptUtils::GetFieldTypeFromMonoType(monoType);
			managedProperty.IsProperty = true;

			int align;
			managedProperty.Size = mono_type_size(monoType, &align);
			managedClass.Size += managedProperty.Size;
			if (std::find(managedClass.Fields.begin(), managedClass.Fields.end(), propertyID) == managedClass.Fields.end())
				managedClass.Fields.push_back(managedProperty.ID);
		}
	}

	const std::unordered_map<uint32_t, ManagedClass>& ScriptCache::GetCachedClasses() { return s_Cache->Classes; }
	const std::unordered_map<uint32_t, FieldInfo>& ScriptCache::GetCachedFields() { return s_Cache->Fields; }
	const std::unordered_map<uint32_t, std::vector<ManagedMethod>>& ScriptCache::GetCachedMethods() { return s_Cache->Methods; }

}

#include "pch.h"
#include "ScriptUtils.h"
#include "ScriptEngine.h"

#include "Engine/Core/Hash.h"
#include "Engine/ImGui/ImGui.h"
#include "Engine/Utilities/YAMLSerializationHelpers.h"

#include <mono/metadata/appdomain.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/class.h>
#include <mono/metadata/object.h>
#include <mono/metadata/reflection.h>
#include <mono/metadata/attrdefs.h>
#include <mono/metadata/tokentype.h>

namespace Engine {

	struct MethodThunkCache
	{
		ManagedMethodThunk<MonoObject*> Entity_OnCreate;
		ManagedMethodThunk<MonoObject*, float> Entity_OnUpdate;
		ManagedMethodThunk<MonoObject*, float> Entity_OnPhysicsUpdate;
		ManagedMethodThunk<MonoObject*> Entity_OnDestroyInternal;
		ManagedMethodThunk<MonoObject*> IEditorRunnable_OnInstantiate;
		ManagedMethodThunk<MonoObject*> IEditorRunnable_OnUIRender;
		ManagedMethodThunk<MonoObject*> IEditorRunnable_OnShutdown;
	};

	static MethodThunkCache* s_MethodThunks = nullptr;

	void ScriptUtils::Init()
	{
		s_MethodThunks = hnew MethodThunkCache();

#define REGISTER_METHOD_THUNK(var, method) s_MethodThunks->var.SetThunkFromMethod(method);

		REGISTER_METHOD_THUNK(Entity_OnCreate, ZONG_CACHED_METHOD("Hazel.Entity", "OnCreate", 0));
		REGISTER_METHOD_THUNK(Entity_OnUpdate, ZONG_CACHED_METHOD("Hazel.Entity", "OnUpdate", 1));
		REGISTER_METHOD_THUNK(Entity_OnPhysicsUpdate, ZONG_CACHED_METHOD("Hazel.Entity", "OnPhysicsUpdate", 1));
		REGISTER_METHOD_THUNK(Entity_OnDestroyInternal, ZONG_CACHED_METHOD("Hazel.Entity", "OnDestroyInternal", 0));
		REGISTER_METHOD_THUNK(IEditorRunnable_OnInstantiate, ZONG_CACHED_METHOD("Hazel.IEditorRunnable", "OnInstantiate", 0));
		REGISTER_METHOD_THUNK(IEditorRunnable_OnUIRender, ZONG_CACHED_METHOD("Hazel.IEditorRunnable", "OnUIRender", 0));
		REGISTER_METHOD_THUNK(IEditorRunnable_OnShutdown, ZONG_CACHED_METHOD("Hazel.IEditorRunnable", "OnShutdown", 0));

#undef REGISTER_METHOD_THUNK
	}

	void ScriptUtils::Shutdown()
	{
		hdelete s_MethodThunks;
		s_MethodThunks = nullptr;
	}

	bool ScriptUtils::CheckMonoError(MonoError& error)
	{
		bool hasError = !mono_error_ok(&error);

		if (hasError)
		{
			unsigned short errorCode = mono_error_get_error_code(&error);
			const char* errorMessage = mono_error_get_message(&error);

			ZONG_CORE_ERROR_TAG("ScriptEngine", "Mono Error!");
			ZONG_CORE_ERROR_TAG("ScriptEngine", "\tError Code: {0}", errorCode);
			ZONG_CORE_ERROR_TAG("ScriptEngine", "\tError Message: {0}", errorMessage);
			mono_error_cleanup(&error);
			ZONG_CORE_ASSERT(false);
		}

		return hasError;
	}

	struct MonoExceptionInfo
	{
		std::string TypeName;
		std::string Source;
		std::string Message;
		std::string StackTrace;
	};

	static MonoExceptionInfo GetExceptionInfo(MonoObject* exception)
	{
		MonoClass* exceptionClass = mono_object_get_class(exception);
		MonoType* exceptionType = mono_class_get_type(exceptionClass);

		auto GetExceptionString = [exception, exceptionClass](const char* stringName) -> std::string
		{
			MonoProperty* property = mono_class_get_property_from_name(exceptionClass, stringName);

			if (property == nullptr)
				return "";

			MonoMethod* getterMethod = mono_property_get_get_method(property);

			if (getterMethod == nullptr)
				return "";

			MonoString* string = (MonoString*)mono_runtime_invoke(getterMethod, exception, NULL, NULL);
			return ScriptUtils::MonoStringToUTF8(string);
		};

		return { mono_type_get_name(exceptionType), GetExceptionString("Source"), GetExceptionString("Message"), GetExceptionString("StackTrace") };
	}

	void ScriptUtils::HandleException(MonoObject* exception)
	{
		if (exception == nullptr)
			return;

		MonoExceptionInfo exceptionInfo = GetExceptionInfo(exception);
		ZONG_CONSOLE_LOG_ERROR("{0}: {1}. Source: {2}, Stack Trace: {3}", exceptionInfo.TypeName, exceptionInfo.Message, exceptionInfo.Source, exceptionInfo.StackTrace);
	}

	Buffer ScriptUtils::GetFieldValue(MonoObject* classInstance, std::string_view fieldName, FieldType fieldType, bool isProperty)
	{
		ZONG_PROFILE_FUNC();
		return MonoObjectToValue(GetFieldValueObject(classInstance, fieldName, isProperty), fieldType);
	}

	MonoObject* ScriptUtils::GetFieldValueObject(MonoObject* classInstance, std::string_view fieldName, bool isProperty)
	{
		ZONG_PROFILE_FUNC();

		MonoClass* objectClass = mono_object_get_class(classInstance);

		MonoObject* valueObject = nullptr;

		if (isProperty)
		{
			MonoProperty* classProperty = mono_class_get_property_from_name(objectClass, fieldName.data());
			valueObject = mono_property_get_value(classProperty, classInstance, nullptr, nullptr);
		}
		else
		{
			MonoClassField* classField = mono_class_get_field_from_name(objectClass, fieldName.data());
			valueObject = mono_field_get_value_object(mono_domain_get(), classField, classInstance);
		}

		return valueObject;
	}

	void ScriptUtils::SetFieldValue(MonoObject* classInstance, const FieldInfo* fieldInfo, const void* data)
	{
		ZONG_PROFILE_FUNC();

		if (classInstance == nullptr || fieldInfo == nullptr || data == nullptr)
			return;

		if (!fieldInfo->IsWritable())
			return;

		MonoClass* objectClass = mono_object_get_class(classInstance);

		if (fieldInfo->IsProperty)
		{
			MonoProperty* classProperty = mono_class_get_property_from_name(objectClass, fieldInfo->Name.c_str());
			void* propertyData = nullptr;

			if (fieldInfo->IsArray() || FieldUtils::IsPrimitiveType(fieldInfo->Type))
				propertyData = const_cast<void*>(data);
			else
				propertyData = ValueToMonoObject(data, fieldInfo->Type);

			mono_property_set_value(classProperty, classInstance, &propertyData, nullptr);
		}
		else
		{
			MonoClassField* classField = mono_class_get_field_from_name(objectClass, fieldInfo->Name.c_str());
			void* fieldData = nullptr;

			if (fieldInfo->IsArray() || FieldUtils::IsPrimitiveType(fieldInfo->Type))
				fieldData = (void*)data;
			else
				fieldData = ValueToMonoObject(data, fieldInfo->Type);

			mono_field_set_value(classInstance, classField, fieldData);
		}
	}

	// Type Utils
	Buffer ScriptUtils::MonoObjectToValue(MonoObject* obj, FieldType fieldType)
	{
		ZONG_PROFILE_FUNC();

		if (obj == nullptr)
			return Buffer();

		Buffer result;
		result.Allocate(FieldUtils::GetFieldTypeSize(fieldType));
		result.ZeroInitialize();

		switch (fieldType)
		{
			case FieldType::Bool:
			{
				bool value = (bool)Unbox<MonoBoolean>(obj);
				result.Write(&value, sizeof(bool));
				break;
			}
			case FieldType::Int8:
			{
				int8_t value = Unbox<int8_t>(obj);
				result.Write(&value, sizeof(int8_t));
				break;
			}
			case FieldType::Int16:
			{
				int16_t value = Unbox<int16_t>(obj);
				result.Write(&value, sizeof(int16_t));
				break;
			}
			case FieldType::Int32:
			{
				int32_t value = Unbox<int32_t>(obj);
				result.Write(&value, sizeof(int32_t));
				break;
			}
			case FieldType::Int64:
			{
				int64_t value = Unbox<int64_t>(obj);
				result.Write(&value, sizeof(int64_t));
				break;
			}
			case FieldType::UInt8:
			{
				uint8_t value = Unbox<uint8_t>(obj);
				result.Write(&value, sizeof(uint8_t));
				break;
			}
			case FieldType::UInt16:
			{
				uint16_t value = Unbox<uint16_t>(obj);
				result.Write(&value, sizeof(uint16_t));
				break;
			}
			case FieldType::UInt32:
			{
				uint32_t value = Unbox<uint32_t>(obj);
				result.Write(&value, sizeof(uint32_t));
				break;
			}
			case FieldType::UInt64:
			{
				uint64_t value = Unbox<uint64_t>(obj);
				result.Write(&value, sizeof(uint64_t));
				break;
			}
			case FieldType::Float:
			{
				float value = Unbox<float>(obj);
				result.Write(&value, sizeof(float));
				break;
			}
			case FieldType::Double:
			{
				double value = Unbox<double>(obj);
				result.Write(&value, sizeof(double));
				break;
			}
			case FieldType::String:
			{
				std::string str = MonoStringToUTF8((MonoString*)obj);
				result.Allocate(str.size() + 1);
				result.ZeroInitialize();
				result.Write(str.data(), str.size());
				break;
			}
			case FieldType::Vector2:
			{
				glm::vec2 value = Unbox<glm::vec2>(obj);
				result.Write(glm::value_ptr(value), sizeof(glm::vec2));
				break;
			}
			case FieldType::Vector3:
			{
				glm::vec3 value = Unbox<glm::vec3>(obj);
				result.Write(glm::value_ptr(value), sizeof(glm::vec3));
				break;
			}
			case FieldType::Vector4:
			{
				glm::vec4 value = Unbox<glm::vec4>(obj);
				result.Write(glm::value_ptr(value), sizeof(glm::vec4));
				break;
			}
			case FieldType::AssetHandle:
			{
				AssetHandle value = Unbox<AssetHandle>(obj);
				result.Write(&value, sizeof(AssetHandle));
				break;
			}
			case FieldType::Entity:
			{
				Buffer idBuffer = GetFieldValue(obj, "ID", FieldType::UInt64, false);
				result.Write(idBuffer.Data, sizeof(UUID));
				idBuffer.Release();
				break;
			}
			case FieldType::Prefab:
			case FieldType::Mesh:
			case FieldType::StaticMesh:
			case FieldType::Material:
			case FieldType::PhysicsMaterial:
			case FieldType::Scene: //TEST - TIM
			case FieldType::Texture2D:
			{
				Buffer handleBuffer = GetFieldValue(obj, "m_Handle", FieldType::AssetHandle, false);
				result.Write(handleBuffer.Data, sizeof(AssetHandle));
				handleBuffer.Release();
				break;
			}
		}

		return result;
	}

	MonoObject* ScriptUtils::ValueToMonoObject(const void* data, FieldType dataType)
	{
		ZONG_PROFILE_FUNC();

		if (FieldUtils::IsPrimitiveType(dataType))
		{
			return BoxValue(ScriptCache::GetFieldTypeClass(dataType), data);
		}
		else
		{
			switch (dataType)
			{
				case FieldType::String: return (MonoObject*)UTF8StringToMono(std::string((const char*)data));
				case FieldType::Prefab: return ScriptEngine::CreateManagedObject("Hazel.Prefab", *(AssetHandle*)data);
				case FieldType::Entity: return ScriptEngine::CreateManagedObject("Hazel.Entity", *(UUID*)data);
				case FieldType::Mesh: return ScriptEngine::CreateManagedObject("Hazel.Mesh", *(AssetHandle*)data);
				case FieldType::StaticMesh: return ScriptEngine::CreateManagedObject("Hazel.StaticMesh", *(AssetHandle*)data);
				case FieldType::Material: return ScriptEngine::CreateManagedObject("Hazel.Material", *(AssetHandle*)data);
				case FieldType::PhysicsMaterial: return ScriptEngine::CreateManagedObject("Hazel.PhysicsMaterial", *(AssetHandle*)data);
				case FieldType::Texture2D: return ScriptEngine::CreateManagedObject("Hazel.Texture2D", *(AssetHandle*)data);
				case FieldType::Scene: return ScriptEngine::CreateManagedObject("Hazel.Scene", *(AssetHandle*)data);
			}
		}

		ZONG_CORE_ASSERT(false, "Unsupported value type!");
		return nullptr;
	}

	/*Utils::ValueWrapper ScriptUtils::GetDefaultValueForType(const ManagedType& type)
	{
		ZONG_PROFILE_FUNC();

		switch (type.NativeType)
		{
		case UnmanagedType::Bool: return Utils::ValueWrapper(false);
		case UnmanagedType::Int8: return Utils::ValueWrapper(int8_t(0));
		case UnmanagedType::Int16: return Utils::ValueWrapper(int16_t(0));
		case UnmanagedType::Int32: return Utils::ValueWrapper(int32_t(0));
		case UnmanagedType::Int64: return Utils::ValueWrapper(int64_t(0));
		case UnmanagedType::UInt8: return Utils::ValueWrapper(uint8_t(0));
		case UnmanagedType::UInt16: return Utils::ValueWrapper(uint16_t(0));
		case UnmanagedType::UInt32: return Utils::ValueWrapper(uint32_t(0));
		case UnmanagedType::UInt64: return Utils::ValueWrapper(uint64_t(0));
		case UnmanagedType::Float: return Utils::ValueWrapper(0.0f);
		case UnmanagedType::Double: return Utils::ValueWrapper(0.0);
		case UnmanagedType::String: return Utils::ValueWrapper("");
		case UnmanagedType::AssetHandle: return Utils::ValueWrapper(uint64_t(0));
		case UnmanagedType::Vector2: return Utils::ValueWrapper(glm::vec2(0.0f));
		case UnmanagedType::Vector3: return Utils::ValueWrapper(glm::vec3(0.0f));
		case UnmanagedType::Vector4: return Utils::ValueWrapper(glm::vec4(0.0f));
		case UnmanagedType::Prefab: return Utils::ValueWrapper(uint64_t(0));
		case UnmanagedType::Entity: return Utils::ValueWrapper(uint64_t(0));
		case UnmanagedType::Mesh: return Utils::ValueWrapper(uint64_t(0));
		case UnmanagedType::StaticMesh: return Utils::ValueWrapper(uint64_t(0));
		case UnmanagedType::Material: return Utils::ValueWrapper(uint64_t(0));
		case UnmanagedType::PhysicsMaterial: return Utils::ValueWrapper(uint64_t(0));
		case UnmanagedType::Texture2D: return Utils::ValueWrapper(uint64_t(0));
		}

		return Utils::ValueWrapper();
	}*/

	/*MonoObject* ScriptUtils::GetDefaultValueObjectForType(const ManagedType& type)
	{
		ZONG_PROFILE_FUNC();

		switch (type.NativeType)
		{
			case UnmanagedType::Bool: return BoxValue(ZONG_CACHED_CLASS("System.Boolean"), (MonoBoolean)false);
			case UnmanagedType::Int8: return BoxValue(ZONG_CACHED_CLASS("System.SByte"), 0);
			case UnmanagedType::Int16: return BoxValue(ZONG_CACHED_CLASS("System.Int16"), 0);
			case UnmanagedType::Int32: return BoxValue(ZONG_CACHED_CLASS("System.Int32"), 0);
			case UnmanagedType::Int64: return BoxValue(ZONG_CACHED_CLASS("System.Int64"), 0);
			case UnmanagedType::UInt8: return BoxValue(ZONG_CACHED_CLASS("System.Byte"), 0);
			case UnmanagedType::UInt16: return BoxValue(ZONG_CACHED_CLASS("System.UInt16"), 0);
			case UnmanagedType::UInt32: return BoxValue(ZONG_CACHED_CLASS("System.UInt32"), 0);
			case UnmanagedType::UInt64: return BoxValue(ZONG_CACHED_CLASS("System.UInt64"), 0);
			case UnmanagedType::Float: return BoxValue(ZONG_CACHED_CLASS("System.Single"), 0);
			case UnmanagedType::Double: return BoxValue(ZONG_CACHED_CLASS("System.Double"), 0);
			case UnmanagedType::String: return (MonoObject*)UTF8StringToMono("");
			case UnmanagedType::AssetHandle: return BoxValue(ZONG_CACHED_CLASS("Hazel.AssetHandle"), 0);
			case UnmanagedType::Vector2: return BoxValue(ZONG_CACHED_CLASS("Hazel.Vector2"), glm::vec2(0.0f));
			case UnmanagedType::Vector3: return BoxValue(ZONG_CACHED_CLASS("Hazel.Vector3"), glm::vec3(0.0f));
			case UnmanagedType::Vector4: return BoxValue(ZONG_CACHED_CLASS("Hazel.Vector4"), glm::vec4(0.0f));
			case UnmanagedType::Prefab: return nullptr;
			case UnmanagedType::Entity: return nullptr;
			case UnmanagedType::Mesh: return nullptr;
			case UnmanagedType::StaticMesh: return nullptr;
			case UnmanagedType::Material: return nullptr;
			case UnmanagedType::PhysicsMaterial: return nullptr;
			case UnmanagedType::Texture2D: return nullptr;
		}

		return nullptr;
	}*/

	FieldType ScriptUtils::GetFieldTypeFromMonoType(MonoType* monoType)
	{
		int32_t typeEncoding = mono_type_get_type(monoType);
		MonoClass* typeClass = mono_type_get_class(monoType);

		switch (typeEncoding)
		{
			case MONO_TYPE_VOID:		return FieldType::Void;
			case MONO_TYPE_BOOLEAN:		return FieldType::Bool;
			case MONO_TYPE_CHAR:		return FieldType::UInt16;
			case MONO_TYPE_I1:			return FieldType::Int8;
			case MONO_TYPE_I2:			return FieldType::Int16;
			case MONO_TYPE_I4:			return FieldType::Int32;
			case MONO_TYPE_I8:			return FieldType::Int64;
			case MONO_TYPE_U1:			return FieldType::UInt8;
			case MONO_TYPE_U2:			return FieldType::UInt16;
			case MONO_TYPE_U4:			return FieldType::UInt32;
			case MONO_TYPE_U8:			return FieldType::UInt64;
			case MONO_TYPE_R4:			return FieldType::Float;
			case MONO_TYPE_R8:			return FieldType::Double;
			case MONO_TYPE_STRING:		return FieldType::String;
			case MONO_TYPE_VALUETYPE:
			{
				if (mono_class_is_enum(typeClass))
					return GetFieldTypeFromMonoType(mono_type_get_underlying_type(monoType));

				if (ZONG_CORE_CLASS(AssetHandle) && typeClass == ZONG_CORE_CLASS(AssetHandle)->Class)
					return FieldType::AssetHandle;

				if (ZONG_CORE_CLASS(Vector2) && typeClass == ZONG_CORE_CLASS(Vector2)->Class)
					return FieldType::Vector2;

				if (ZONG_CORE_CLASS(Vector3) && typeClass == ZONG_CORE_CLASS(Vector3)->Class)
					return FieldType::Vector3;

				if (ZONG_CORE_CLASS(Vector4) && typeClass == ZONG_CORE_CLASS(Vector4)->Class)
					return FieldType::Vector4;

				break;
			}
			case MONO_TYPE_CLASS:
			{
				auto entityClass = ZONG_CORE_CLASS(Entity);

				if (entityClass && mono_class_is_assignable_from(typeClass, entityClass->Class))
					return FieldType::Entity;

				if (ZONG_CORE_CLASS(Prefab) && typeClass == ZONG_CORE_CLASS(Prefab)->Class)
					return FieldType::Prefab;

				if (ZONG_CORE_CLASS(Mesh) && typeClass == ZONG_CORE_CLASS(Mesh)->Class)
					return FieldType::Mesh;

				if (ZONG_CORE_CLASS(StaticMesh) && typeClass == ZONG_CORE_CLASS(StaticMesh)->Class)
					return FieldType::StaticMesh;

				if (ZONG_CORE_CLASS(Material) && typeClass == ZONG_CORE_CLASS(Material)->Class)
					return FieldType::Material;

				if (ZONG_CORE_CLASS(PhysicsMaterial) && typeClass == ZONG_CORE_CLASS(PhysicsMaterial)->Class)
					return FieldType::PhysicsMaterial;

				if (ZONG_CORE_CLASS(Texture2D) && typeClass == ZONG_CORE_CLASS(Texture2D)->Class)
					return FieldType::Texture2D;

				if (ZONG_CORE_CLASS(Scene) && typeClass == ZONG_CORE_CLASS(Scene)->Class)
					return FieldType::Scene;

				break;
			}
			case MONO_TYPE_SZARRAY:
			case MONO_TYPE_ARRAY:
			{
				MonoClass* elementClass = mono_class_get_element_class(typeClass);
				if (elementClass == nullptr)
					break;

				ManagedClass* managedElementClass = ScriptCache::GetManagedClass(elementClass);
				if (managedElementClass == nullptr)
					break;

				return GetFieldTypeFromMonoType(mono_class_get_type(elementClass));
			}
		}

		return FieldType::Void;
	}

	std::string ScriptUtils::ResolveMonoClassName(MonoClass* monoClass)
	{
		const char* classNamePtr = mono_class_get_name(monoClass);
		std::string className = classNamePtr != nullptr ? classNamePtr : "";

		if (className.empty())
			return "Unknown Class";

		MonoClass* nestingClass = mono_class_get_nesting_type(monoClass);
		if (nestingClass != nullptr)
		{
			className = ResolveMonoClassName(nestingClass) + "/" + className;
		}
		else
		{
			const char* classNamespacePtr = mono_class_get_namespace(monoClass);
			if (classNamespacePtr)
				className = std::string(classNamespacePtr) + "." + className;
		}

		MonoType* classType = mono_class_get_type(monoClass);
		if (mono_type_get_type(classType) == MONO_TYPE_SZARRAY || mono_type_get_type(classType) == MONO_TYPE_ARRAY)
			Utils::String::Erase(className, "[]");

		return className;
	}

	MonoString* ScriptUtils::EmptyMonoString()
	{
		return mono_string_empty(ScriptEngine::GetScriptDomain());
	}

	std::string ScriptUtils::MonoStringToUTF8(MonoString* monoString)
	{
		if (monoString == nullptr || mono_string_length(monoString) == 0)
			return "";

		MonoError error;
		char* utf8 = mono_string_to_utf8_checked(monoString, &error);
		if (ScriptUtils::CheckMonoError(error))
			return "";
		std::string result(utf8);
		mono_free(utf8);
		return result;
	}

	MonoString* ScriptUtils::UTF8StringToMono(const std::string& str)
	{
		return mono_string_new(ScriptEngine::GetScriptDomain(), str.c_str());
	}

	MonoObject* ScriptUtils::BoxValue(MonoClass* valueClass, const void* value)
	{
		return mono_value_box(ScriptEngine::GetScriptDomain(), valueClass, const_cast<void*>(value));
	}

	std::string ScriptUtils::GetCurrentStackTrace()
	{
		MonoObject* stackTraceInstance = ScriptEngine::CreateManagedObject("System.Diagnostics.StackTrace");
		MonoObject* exception = nullptr;
		MonoString* stackTraceString = mono_object_to_string(stackTraceInstance, &exception);
		HandleException(exception);
		return MonoStringToUTF8(stackTraceString);
	}

	void* ScriptUtils::GetUnmanagedMethodTunk(MonoMethod* managedMethod)
	{
		return mono_method_get_unmanaged_thunk(managedMethod);
	}

	// ManagedArrayUtils
	//Utils::ValueWrapper ManagedArrayUtils::GetValue(MonoArray* arr, uintptr_t index)
	//{
	//	ZONG_CORE_VERIFY(arr, "Can't get a value from a nullptr array");

	//	uintptr_t length = mono_array_length(arr);
	//	ZONG_CORE_VERIFY(index < length, "index out of range");

	//	MonoClass* arrayClass = mono_object_get_class((MonoObject*)arr);
	//	MonoClass* elementClass = mono_class_get_element_class(arrayClass);
	//	int32_t elementSize = mono_array_element_size(arrayClass);
	//	ManagedType elementType = ManagedType::FromClass(elementClass);

	//	if (elementType.IsReferenceType())
	//	{
	//		MonoObject* elem = mono_array_get(arr, MonoObject*, index);
	//		return ScriptUtils::MonoObjectToValue(elem, elementType);
	//	}

	//	// Value type
	//	char* src = mono_array_addr_with_size(arr, elementSize, index);
	//	return Utils::ValueWrapper(src, elementSize);
	//}

	uintptr_t ManagedArrayUtils::Length(MonoArray* arr)
	{
		ZONG_CORE_VERIFY(arr);
		return mono_array_length(arr);
	}

	void ManagedArrayUtils::Resize(MonoArray** arr, uintptr_t newLength)
	{
		if (arr == nullptr || *arr == nullptr)
			return;

		MonoClass* arrayClass = mono_object_get_class((MonoObject*)*arr);
		MonoClass* elementClass = mono_class_get_element_class(arrayClass);

		MonoArray* newArray = mono_array_new(ScriptEngine::GetScriptDomain(), elementClass, newLength);

		uintptr_t length = mono_array_length(*arr);
		uintptr_t copyLength = newLength < length ? newLength : length;

		char* src = mono_array_addr_with_size(*arr, mono_array_element_size(arrayClass), 0);
		char* dst = mono_array_addr_with_size(newArray, mono_array_element_size(arrayClass), 0);
		memcpy(dst, src, copyLength * mono_array_element_size(arrayClass));

		*arr = newArray;
	}

	//void ManagedArrayUtils::RemoveAt(MonoArray** arr, uintptr_t index)
	//{
	//	ZONG_CORE_VERIFY(arr && *arr, "Cannot remove elements from nullptr array");

	//	uintptr_t length = mono_array_length(*arr);
	//	ZONG_CORE_VERIFY(index < length, "Index out of range");

	//	if (index == length - 1)
	//	{
	//		Resize(arr, length - 1);
	//		return;
	//	}

	//	MonoClass* arrayClass = mono_object_get_class((MonoObject*)*arr);
	//	MonoClass* elementClass = mono_class_get_element_class(arrayClass);
	//	int32_t elementSize = mono_array_element_size(arrayClass);
	//	MonoArray* temp = mono_array_new(ScriptEngine::GetScriptDomain(), elementClass, length - 1);
	//	ZONG_CORE_VERIFY(temp);

	//	if (index != 0)
	//	{
	//		char* src = mono_array_addr_with_size(*arr, elementSize, 0);
	//		char* dst = mono_array_addr_with_size(temp, elementSize, 0);
	//		memcpy(dst, src, index * elementSize);

	//		src = mono_array_addr_with_size(*arr, elementSize, index + 1);
	//		dst = mono_array_addr_with_size(temp, elementSize, index);
	//		memcpy(dst, src, (length - index - 1) * elementSize);
	//	}
	//	else
	//	{
	//		char* src = mono_array_addr_with_size(*arr, elementSize, 1);
	//		char* dst = mono_array_addr_with_size(temp, elementSize, 0);
	//		memcpy(dst, src, (length - 1) * elementSize);
	//	}

	//	*arr = temp;
	//}

	MonoArray* ManagedArrayUtils::Copy(MonoArray* arr)
	{
		ZONG_CORE_VERIFY(arr);

		// TODO: Maybe attempt a deep copy? mono_array_clone only creates a shallow copy, for now that's fine though.
		return mono_array_clone(arr);
	}

	MonoArray* ManagedArrayUtils::Create(const std::string& arrayClass, uintptr_t length)
	{
		ZONG_CORE_VERIFY(!arrayClass.empty(), "Cannot create managed array of no type");

		ManagedClass* klass = ScriptCache::GetManagedClassByName(arrayClass);
		ZONG_CORE_VERIFY(klass, "Unable to find array class");

		return mono_array_new(ScriptEngine::GetScriptDomain(), klass->Class, length);
	}

	MonoArray* ManagedArrayUtils::Create(ManagedClass* arrayClass, uintptr_t length)
	{
		ZONG_CORE_VERIFY(arrayClass, "Cannot create managed array of no type");
		return mono_array_new(ScriptEngine::GetScriptDomain(), arrayClass->Class, length);
	}

	void ManagedArrayUtils::SetValueInternal(MonoArray* arr, uintptr_t index, void* data)
	{
		ZONG_CORE_VERIFY(arr);

		uintptr_t length = mono_array_length(arr);
		
		if (index >= length)
		{
			ZONG_CORE_WARN_TAG("ScriptEngine", "Index out of bounds in C# array!");
			return;
		}

		MonoClass* arrayClass = mono_object_get_class((MonoObject*)arr);
		MonoClass* elementClass = mono_class_get_element_class(arrayClass);
		int32_t elementSize = mono_array_element_size(arrayClass);
		MonoType* elementType = mono_class_get_type(elementClass);

		if (mono_type_is_reference(elementType) || mono_type_is_byref(elementType))
		{
			MonoObject* boxed = ScriptUtils::ValueToMonoObject(data, ScriptUtils::GetFieldTypeFromMonoType(elementType));
			mono_array_setref(arr, index, boxed);
		}
		else
		{
			char* dst = mono_array_addr_with_size(arr, elementSize, index);
			memcpy(dst, data, elementSize);
		}
	}

	void ManagedArrayUtils::SetValueInternal(MonoArray* arr, uintptr_t index, MonoObject* value)
	{
		ZONG_CORE_VERIFY(arr);

		uintptr_t length = mono_array_length(arr);
		ZONG_CORE_VERIFY(index < length, "index out of range");

		MonoClass* arrayClass = mono_object_get_class((MonoObject*)arr);
		MonoClass* elementClass = mono_class_get_element_class(arrayClass);
		int32_t elementSize = mono_array_element_size(arrayClass);
		MonoType* elementType = mono_class_get_type(elementClass);

		if (mono_type_is_reference(elementType) || mono_type_is_byref(elementType))
			mono_array_setref(arr, index, value);
		else
			mono_array_set(arr, MonoObject*, index, value);
	}

	// Managed Thunks
	void MethodThunks::OnEntityCreate(GCHandle entityHandle)
	{
		ZONG_PROFILE_FUNC();

		MonoException* exception = nullptr;
		MonoObject* entityObject = GCManager::GetReferencedObject(entityHandle);
		s_MethodThunks->Entity_OnCreate.Invoke(entityObject, &exception);
		ScriptUtils::HandleException((MonoObject*)exception);
	}

	void MethodThunks::OnEntityUpdate(GCHandle entityHandle, float ts)
	{
		ZONG_PROFILE_FUNC();

		MonoException* exception = nullptr;
		MonoObject* entityObject = GCManager::GetReferencedObject(entityHandle);
		s_MethodThunks->Entity_OnUpdate.Invoke(entityObject, ts, &exception);
		ScriptUtils::HandleException((MonoObject*)exception);
	}

	void MethodThunks::OnEntityPhysicsUpdate(GCHandle entityHandle, float ts)
	{
		ZONG_PROFILE_FUNC();

		MonoException* exception = nullptr;
		MonoObject* entityObject = GCManager::GetReferencedObject(entityHandle);
		s_MethodThunks->Entity_OnPhysicsUpdate.Invoke(entityObject, ts, &exception);
		ScriptUtils::HandleException((MonoObject*)exception);
	}

	void MethodThunks::OnEntityDestroyed(GCHandle entityHandle)
	{
		ZONG_PROFILE_FUNC();

		MonoException* exception = nullptr;
		MonoObject* entityObject = GCManager::GetReferencedObject(entityHandle);
		s_MethodThunks->Entity_OnDestroyInternal.Invoke(entityObject, &exception);
		ScriptUtils::HandleException((MonoObject*)exception);
	}

	void MethodThunks::IEditorRunnable_OnInstantiate(GCHandle scriptHandle)
	{
#ifndef ZONG_DIST
		ZONG_PROFILE_FUNC();

		MonoException* exception = nullptr;
		MonoObject* scriptObject = GCManager::GetReferencedObject(scriptHandle);
		s_MethodThunks->IEditorRunnable_OnInstantiate.Invoke(scriptObject, &exception);
		ScriptUtils::HandleException((MonoObject*)exception);
#endif
	}

	void MethodThunks::IEditorRunnable_OnUIRender(GCHandle scriptHandle)
	{
#ifndef ZONG_DIST
		ZONG_PROFILE_FUNC();

		MonoException* exception = nullptr;
		MonoObject* scriptObject = GCManager::GetReferencedObject(scriptHandle);
		s_MethodThunks->IEditorRunnable_OnUIRender.Invoke(scriptObject, &exception);
		ScriptUtils::HandleException((MonoObject*)exception);
#endif
	}

	void MethodThunks::IEditorRunnable_OnShutdown(GCHandle scriptHandle)
	{
#ifndef ZONG_DIST
		ZONG_PROFILE_FUNC();

		MonoException* exception = nullptr;
		MonoObject* scriptObject = GCManager::GetReferencedObject(scriptHandle);
		s_MethodThunks->IEditorRunnable_OnShutdown.Invoke(scriptObject, &exception);
		ScriptUtils::HandleException((MonoObject*)exception);
#endif
	}

}

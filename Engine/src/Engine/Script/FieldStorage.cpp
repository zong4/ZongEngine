#include "pch.h"
#include "FieldStorage.h"
#include "ScriptUtils.h"
#include "ScriptEngine.h"

#include <mono/metadata/appdomain.h>
#include <mono/metadata/class.h>
#include <mono/metadata/object.h>

namespace Engine {

	bool FieldStorage::GetValueRuntime(Buffer& outBuffer) const
	{
		if (m_RuntimeInstance == nullptr)
			return false;

		MonoObject* runtimeObject = GCManager::GetReferencedObject(m_RuntimeInstance);
		if (runtimeObject == nullptr)
			return false;

		outBuffer = ScriptUtils::GetFieldValue(runtimeObject, m_FieldInfo->Name, m_FieldInfo->Type, m_FieldInfo->IsProperty);
		return true;
	}

	void FieldStorage::SetValueRuntime(const void* data)
	{
		if (m_RuntimeInstance == nullptr)
			return;

		MonoObject* runtimeObject = GCManager::GetReferencedObject(m_RuntimeInstance);
		ScriptUtils::SetFieldValue(runtimeObject, m_FieldInfo, data);
	}

	void ArrayFieldStorage::GetValueRuntime(uint32_t index, void* data) const
	{
		if (m_RuntimeInstance == nullptr)
			return;

		MonoObject* runtimeObject = GCManager::GetReferencedObject(m_RuntimeInstance);
		MonoClassField* field = mono_class_get_field_from_name(mono_object_get_class(runtimeObject), m_FieldInfo->Name.c_str());
		MonoArray* arrayObject = (MonoArray*)mono_field_get_value_object(ScriptEngine::GetScriptDomain(), field, runtimeObject);

		if (!arrayObject)
			return;

		MonoType* fieldType = mono_field_get_type(field);
		MonoClass* fieldTypeClass = mono_type_get_class(fieldType);
		MonoClass* fieldElementClass = mono_class_get_element_class(fieldTypeClass);
		MonoType* elementType = mono_class_get_type(fieldElementClass);

		char* src = mono_array_addr_with_size(arrayObject, m_FieldInfo->Size, index);
		memcpy(data, src, m_FieldInfo->Size);

		if (mono_type_is_reference(elementType) || mono_type_is_byref(elementType))
		{
			MonoObject* obj = mono_array_get(arrayObject, MonoObject*, index);
			Buffer valueBuffer = ScriptUtils::MonoObjectToValue(obj, m_FieldInfo->Type);
			memcpy(data, valueBuffer.Data, valueBuffer.Size);
			valueBuffer.Release();
		}
		else
		{
			char* src = mono_array_addr_with_size(arrayObject, (int)m_FieldInfo->Size, index);
			memcpy(data, src, m_FieldInfo->Size);
		}
	}

	void ArrayFieldStorage::SetValueRuntime(uint32_t index, const void* data)
	{
		if (m_RuntimeInstance == nullptr)
			return;

		MonoObject* runtimeObject = GCManager::GetReferencedObject(m_RuntimeInstance);
		MonoClassField* field = mono_class_get_field_from_name(mono_object_get_class(runtimeObject), m_FieldInfo->Name.c_str());
		MonoArray* arrayObject = (MonoArray*)mono_field_get_value_object(ScriptEngine::GetScriptDomain(), field, runtimeObject);

		MonoType* fieldType = mono_field_get_type(field);
		MonoClass* fieldTypeClass = mono_type_get_class(fieldType);
		MonoClass* fieldElementClass = mono_class_get_element_class(fieldTypeClass);
		MonoType* elementType = mono_class_get_type(fieldElementClass);

		if (mono_type_is_reference(elementType) || mono_type_is_byref(elementType))
		{
			MonoObject* boxed = ScriptUtils::ValueToMonoObject(data, ScriptUtils::GetFieldTypeFromMonoType(elementType));
			mono_array_setref(arrayObject, index, boxed);
		}
		else
		{
			char* dst = mono_array_addr_with_size(arrayObject, (int)m_FieldInfo->Size, index);
			memcpy(dst, data, m_FieldInfo->Size);
		}
	}

	uint32_t ArrayFieldStorage::GetLengthRuntime() const
	{
		if (m_RuntimeInstance == nullptr)
			return 0;

		MonoObject* runtimeObject = GCManager::GetReferencedObject(m_RuntimeInstance);
		MonoClassField* field = mono_class_get_field_from_name(mono_object_get_class(runtimeObject), m_FieldInfo->Name.c_str());
		MonoArray* arrayObject = (MonoArray*)mono_field_get_value_object(ScriptEngine::GetScriptDomain(), field, runtimeObject);

		if (arrayObject == nullptr)
			return 0;

		return (uint32_t)mono_array_length(arrayObject);
	}

	void ArrayFieldStorage::Resize(uint32_t newLength)
	{
		if (m_FieldInfo->Type == FieldType::String)
			return;

		if (m_RuntimeInstance == nullptr)
		{
			if (!m_DataBuffer)
			{
				m_DataBuffer.Allocate(newLength * m_FieldInfo->Size);
				m_DataBuffer.ZeroInitialize();
			}
			else
			{
				Buffer newBuffer;
				newBuffer.Allocate(newLength * m_FieldInfo->Size);

				uint32_t copyLength = newLength < m_Length ? newLength : m_Length;
				memcpy(newBuffer.Data, m_DataBuffer.Data, copyLength * m_FieldInfo->Size);

				if (newLength > m_Length)
					memset((byte*)newBuffer.Data + (m_Length * m_FieldInfo->Size), 0, (newLength - m_Length) * m_FieldInfo->Size);

				m_DataBuffer.Release();
				m_DataBuffer = newBuffer;
			}

			m_Length = newLength;
		}
		else
		{
			MonoObject* runtimeObject = GCManager::GetReferencedObject(m_RuntimeInstance);
			MonoClassField* field = mono_class_get_field_from_name(mono_object_get_class(runtimeObject), m_FieldInfo->Name.c_str());
			MonoClass* elementClass = mono_class_get_element_class(mono_type_get_class(mono_field_get_type(field)));
			MonoArray* arrayObject = (MonoArray*)mono_field_get_value_object(ScriptEngine::GetScriptDomain(), field, runtimeObject);
			MonoArray* newArray = mono_array_new(ScriptEngine::GetScriptDomain(), elementClass, newLength);

			if (arrayObject != nullptr)
			{
				uintptr_t length = mono_array_length(arrayObject);
				uintptr_t copyLength = newLength < length ? newLength : length;

				char* src = mono_array_addr_with_size(arrayObject, m_FieldInfo->Size, 0);
				char* dst = mono_array_addr_with_size(newArray, m_FieldInfo->Size, 0);
				memcpy(dst, src, copyLength * m_FieldInfo->Size);
			}

			mono_field_set_value(runtimeObject, field, newArray);
		}
	}

	void ArrayFieldStorage::RemoveAt(uint32_t index)
	{
		if (m_RuntimeInstance == nullptr)
		{
			Buffer newBuffer;
			newBuffer.Allocate((m_Length - 1) * m_FieldInfo->Size);

			if (index != 0)
			{
				memcpy(newBuffer.Data, m_DataBuffer.Data, index * m_FieldInfo->Size);
				memcpy((byte*)newBuffer.Data + (index * m_FieldInfo->Size), (byte*)m_DataBuffer.Data + ((index + 1) * m_FieldInfo->Size), (m_Length - index - 1) * m_FieldInfo->Size);
			}
			else
			{
				memcpy(newBuffer.Data, (byte*)m_DataBuffer.Data + m_FieldInfo->Size, (m_Length - 1) * m_FieldInfo->Size);
			}

			m_DataBuffer.Release();
			m_DataBuffer = Buffer::Copy(newBuffer);
			m_Length--;
		}
		else
		{
			MonoObject* runtimeObject = GCManager::GetReferencedObject(m_RuntimeInstance);
			MonoClassField* field = mono_class_get_field_from_name(mono_object_get_class(runtimeObject), m_FieldInfo->Name.c_str());
			MonoArray* arrayObject = (MonoArray*)mono_field_get_value_object(ScriptEngine::GetScriptDomain(), field, runtimeObject);
			uint32_t length = (uint32_t)mono_array_length(arrayObject);
			ZONG_CORE_VERIFY(index < length, "Index out of range");

			if (index == length - 1)
			{
				Resize(length - 1);
				return;
			}

			MonoClass* arrayClass = mono_object_get_class((MonoObject*)arrayObject);
			MonoClass* elementClass = mono_class_get_element_class(arrayClass);
			int32_t elementSize = mono_array_element_size(arrayClass);
			MonoArray* newArray = mono_array_new(ScriptEngine::GetScriptDomain(), elementClass, length - 1);

			if (index != 0)
			{
				char* src = mono_array_addr_with_size(arrayObject, elementSize, 0);
				char* dst = mono_array_addr_with_size(newArray, elementSize, 0);
				memcpy(dst, src, index * elementSize);

				src = mono_array_addr_with_size(arrayObject, elementSize, index + 1);
				dst = mono_array_addr_with_size(newArray, elementSize, index);
				memcpy(dst, src, (length - index - 1) * elementSize);
			}
			else
			{
				char* src = mono_array_addr_with_size(arrayObject, elementSize, 1);
				char* dst = mono_array_addr_with_size(newArray, elementSize, 0);
				memcpy(dst, src, (length - 1) * elementSize);
			}

			mono_field_set_value(runtimeObject, field, newArray);
		}
	}

	bool ArrayFieldStorage::GetRuntimeArray(Buffer& outData) const
	{
		if (m_RuntimeInstance == nullptr)
			return false;

		MonoObject* runtimeObject = GCManager::GetReferencedObject(m_RuntimeInstance);
		MonoClassField* field = mono_class_get_field_from_name(mono_object_get_class(runtimeObject), m_FieldInfo->Name.c_str());
		MonoArray* arrayObject = (MonoArray*)mono_field_get_value_object(ScriptEngine::GetScriptDomain(), field, runtimeObject);
		
		if (arrayObject == nullptr)
			return false;

		MonoType* fieldType = mono_field_get_type(field);
		MonoClass* fieldTypeClass = mono_type_get_class(fieldType);
		MonoClass* fieldElementClass = mono_class_get_element_class(fieldTypeClass);
		MonoType* elementType = mono_class_get_type(fieldElementClass);

		uint32_t arrayLength = (uint32_t)mono_array_length(arrayObject);
		uint32_t bufferSize = arrayLength * m_FieldInfo->Size;
		outData.Allocate(bufferSize);
		outData.ZeroInitialize();

		if (mono_type_is_reference(elementType) || mono_type_is_byref(elementType))
		{
			for (uint32_t i = 0; i < arrayLength; i++)
			{
				MonoObject* obj = mono_array_get(arrayObject, MonoObject*, i);
				Buffer valueBuffer = ScriptUtils::MonoObjectToValue(obj, m_FieldInfo->Type);
				outData.Write(valueBuffer.Data, valueBuffer.Size, (uint64_t)(i * m_FieldInfo->Size));
				valueBuffer.Release();
			}
		}
		else
		{
			for (uint32_t i = 0; i < arrayLength; i++)
			{
				char* src = mono_array_addr_with_size(arrayObject, (int)m_FieldInfo->Size, i);
				outData.Write((const void*)src, (uint64_t)m_FieldInfo->Size, (uint64_t)(i * m_FieldInfo->Size));
			}
		}

		return true;
	}

	void ArrayFieldStorage::SetRuntimeArray(const Buffer& data)
	{
		if (m_RuntimeInstance == nullptr)
			return;

		if (!data)
			return;

		MonoObject* runtimeObject = GCManager::GetReferencedObject(m_RuntimeInstance);

		if (runtimeObject == nullptr)
			return;

		MonoClass* instanceClass = mono_object_get_class(runtimeObject);
		MonoClassField* field = mono_class_get_field_from_name(instanceClass, m_FieldInfo->Name.c_str());
		MonoType* fieldType = mono_field_get_type(field);
		MonoClass* fieldTypeClass = mono_type_get_class(fieldType);
		MonoClass* fieldElementClass = mono_class_get_element_class(fieldTypeClass);
		MonoType* elementType = mono_class_get_type(fieldElementClass);

		MonoArray* arr = mono_array_new(ScriptEngine::GetScriptDomain(), fieldElementClass, m_Length);

		if (mono_type_is_reference(elementType) || mono_type_is_byref(elementType))
		{
			for (uint32_t i = 0; i < m_Length; i++)
			{
				if (m_FieldInfo->Type == FieldType::String)
				{
					Buffer* valueData = static_cast<Buffer*>(data.Data) + i;
					MonoObject* boxed = ScriptUtils::ValueToMonoObject(valueData->Data, ScriptUtils::GetFieldTypeFromMonoType(elementType));
					mono_array_setref(arr, i, boxed);
				}
				else
				{
					auto* element = static_cast<std::byte*>(data.Data) + i * m_FieldInfo->Size;
					MonoObject* boxed = ScriptUtils::ValueToMonoObject(element, ScriptUtils::GetFieldTypeFromMonoType(elementType));
					mono_array_setref(arr, i, boxed);
				}
			}
		}
		else
		{
			for (uint32_t i = 0; i < m_Length; i++)
			{
				char* dst = mono_array_addr_with_size(arr, (int)m_FieldInfo->Size, i);
				memcpy(dst, static_cast<std::byte*>(data.Data) + i * m_FieldInfo->Size, m_FieldInfo->Size);
			}
		}

		mono_field_set_value(runtimeObject, field, arr);
	}

}

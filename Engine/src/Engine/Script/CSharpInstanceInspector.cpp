#include "pch.h"
#include "CSharpInstanceInspector.h"

#include "ScriptUtils.h"

#include <mono/metadata/object.h>

namespace Engine {

	CSharpInstanceInspector::CSharpInstanceInspector(MonoObject* instance)
		: m_Instance(instance)
	{
		MonoClass* instanceClass = mono_object_get_class(instance);
	}

	bool CSharpInstanceInspector::HasName(std::string_view fullName) const
	{
		MonoClass* instanceClass = mono_object_get_class(m_Instance);

		std::string_view className = mono_class_get_name(instanceClass);
		std::string_view classNamespace = mono_class_get_namespace(instanceClass);

		bool providedNamespace = fullName.find_first_of(".") != std::string_view::npos;

		if (providedNamespace)
		{
			if (classNamespace.empty())
				return false;

			if (fullName.find(classNamespace) == std::string_view::npos)
				return false;
		}

		return fullName.find(className) != std::string_view::npos;
	}

	bool CSharpInstanceInspector::InheritsFrom(std::string_view fullName) const
	{
		bool result = false;

		MonoClass* instanceClass = mono_object_get_class(m_Instance);
		MonoClass* parentClass = mono_class_get_parent(instanceClass);

		bool providedNamespace = fullName.find_first_of(".") != std::string_view::npos;

		while (parentClass != nullptr)
		{
			std::string_view parentNamespace = mono_class_get_namespace(parentClass);

			if (providedNamespace)
			{
				if (parentNamespace.empty())
				{
					// fullName can't possibly match this parents name
					result = false;
					parentClass = mono_class_get_parent(parentClass);
					continue;
				}

				if (fullName.find(parentNamespace) == std::string_view::npos)
				{
					// Namespace doesn't match
					result = false;
					parentClass = mono_class_get_parent(parentClass);
					continue;
				}
			}

			std::string_view parentName = mono_class_get_name(parentClass);
			if (fullName.find(parentName) != std::string_view::npos)
			{
				// Match
				result = true;
				break;
			}

			parentClass = mono_class_get_parent(parentClass);
		}

		return result;
	}

	const char* CSharpInstanceInspector::GetName() const
	{
		MonoClass* instanceClass = mono_object_get_class(m_Instance);
		return mono_class_get_name(instanceClass);
	}

	Buffer CSharpInstanceInspector::GetFieldBuffer(std::string_view fieldName) const
	{
		MonoClass* instanceClass = mono_object_get_class(m_Instance);
		MonoClassField* field = mono_class_get_field_from_name(instanceClass, fieldName.data());
		MonoProperty* prop = mono_class_get_property_from_name(instanceClass, fieldName.data());

		if (field == nullptr && prop == nullptr)
		{
			// No field or property with this name was found
			return {};
		}

		MonoType* fieldMonoType = nullptr;

		if (field != nullptr)
		{
			fieldMonoType = mono_field_get_type(field);
		}
		else if (prop != nullptr)
		{
			MonoMethod* getterMethod = mono_property_get_get_method(prop);
			MonoMethod* setterMethod = mono_property_get_set_method(prop);

			if (getterMethod != nullptr)
			{
				fieldMonoType = mono_signature_get_return_type(mono_method_signature(getterMethod));
			}
			else if (setterMethod != nullptr)
			{
				void* paramIter = nullptr;
				fieldMonoType = mono_signature_get_params(mono_method_signature(setterMethod), &paramIter);
			}
			else
			{
				ZONG_CORE_VERIFY(false);
			}
		}
		else
		{
			ZONG_CORE_VERIFY(false);
		}
		
		FieldType fieldType = ScriptUtils::GetFieldTypeFromMonoType(fieldMonoType);
		return ScriptUtils::GetFieldValue(m_Instance, fieldName, fieldType, prop != nullptr);
	}

}

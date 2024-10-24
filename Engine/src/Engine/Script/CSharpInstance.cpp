#include "pch.h"
#include "CSharpInstance.h"
#include "ScriptCache.h"
#include "ScriptUtils.h"

#include <mono/metadata/appdomain.h>
#include <mono/metadata/object.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/attrdefs.h>

namespace Engine {

	CSharpInstance::CSharpInstance(std::string_view fullName)
		: m_FullName(fullName)
	{
		FindMonoClass();
	}

	void CSharpInstance::FindMonoClass()
	{
		ManagedClass* cachedClass = ScriptCache::GetManagedClassByName(m_FullName);

		if (cachedClass == nullptr || cachedClass->Class == nullptr)
			return;

		m_Class = cachedClass->Class;
	}

	bool CSharpInstance::CanInstantiate() const
	{
		uint32_t classFlags = mono_class_get_flags(m_Class);

		if (classFlags & MONO_TYPE_ATTR_ABSTRACT)
			return false;

		return true;
	}

	bool CSharpInstance::IsValueType() const
	{
		return mono_class_is_valuetype(m_Class);
	}

	MonoObject* CSharpInstance::AllocateInstance() const
	{
		return mono_object_new(mono_domain_get(), m_Class);
	}

	void CSharpInstance::DefaultConstruct(MonoObject* instance) const
	{
		mono_runtime_object_init(instance);
	}

	void CSharpInstance::Construct(MonoObject* instance, void** params, uint32_t paramCount) const
	{
		MonoMethod* monoMethod = nullptr;
		void* methodPtr = 0;

		bool foundSuitableConstructor = false;

		while ((monoMethod = mono_class_get_methods(m_Class, &methodPtr)) != nullptr)
		{
			char* methodName = mono_method_full_name(monoMethod, 0);

			if (strstr(methodName, ".ctor") == nullptr)
			{
				mono_free(methodName);
				continue;
			}

			mono_free(methodName);

			MonoMethodSignature* signature = mono_method_signature(monoMethod);
			if (mono_signature_get_param_count(signature) != paramCount)
				continue;

			// Found the constructor
			foundSuitableConstructor = true;

			MonoObject* exception = nullptr;
			mono_runtime_invoke(monoMethod, instance, params, &exception);
			ScriptUtils::HandleException(exception);
			break;
		}

		if (!foundSuitableConstructor)
			ZONG_CORE_WARN_TAG("ScriptEngine", "Failed to call constructor for {} with {} parameters!", m_FullName, paramCount);
	}

}

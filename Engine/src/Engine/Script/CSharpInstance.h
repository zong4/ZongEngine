#pragma once

#include "ScriptEngine.h"

extern "C" {
	typedef struct _MonoClass MonoClass;
	typedef struct _MonoObject MonoObject;
}

namespace Hazel {

	class CSharpInstance
	{
	public:
		CSharpInstance() = default;
		CSharpInstance(std::string_view fullName);

		template<typename TArg, size_t I>
		void AddToArrayI(void** data, TArg&& arg)
		{
			if constexpr (std::is_pointer_v<std::remove_reference_t<TArg>>)
				data[I] = reinterpret_cast<void*>(arg);
			else
				data[I] = reinterpret_cast<void*>(&arg);
		}

		template<typename... TArgs, size_t... Indices>
		void AddToArray(void** data, TArgs&&... args, const std::index_sequence<Indices...>&)
		{
			(AddToArrayI<TArgs, Indices>(data, std::forward<TArgs>(args)), ...);
		}

		template<typename... TArgs>
		MonoObject* Instantiate(TArgs&&... args)
		{
			if (m_Class == nullptr)
				return nullptr;

			if (!CanInstantiate())
				return nullptr;

			MonoObject* instance = AllocateInstance();

			if (instance != nullptr && !IsValueType())
			{
				constexpr size_t argumentCount = sizeof...(args);

				if constexpr (argumentCount > 0)
				{
					void* data[argumentCount];
					AddToArray<TArgs...>(data, std::forward<TArgs>(args)..., std::make_index_sequence<argumentCount>{});
					Construct(instance, data, argumentCount);
				}
				else
				{
					DefaultConstruct(instance);
				}
			}

			return instance;
		}

		operator bool() const { return !m_FullName.empty() && m_Class != nullptr; }

	private:
		void FindMonoClass();
		bool CanInstantiate() const;
		bool IsValueType() const;
		MonoObject* AllocateInstance() const;

		void DefaultConstruct(MonoObject* instance) const;
		void Construct(MonoObject* instance, void** params, uint32_t paramCount) const;

	private:
		std::string m_FullName = "";
		MonoClass* m_Class = nullptr;
	};

}

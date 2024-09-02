#pragma once

#if __has_include(<cxxabi.h>)
#define HZ_ABI_SUPPORTED
#include <cxxabi.h>
#endif

#include <type_traits>
#include <string>

#include "Hazel/Core/Hash.h"

namespace Hazel {

	// NOTE(Peter): StringType is used because on non-MSVC compilers we have to make a copy of the buffer we get back from abi::__cxa_demangle
	//				This is because that function allocates a heap buffer that we're responsible for freeing.
	//				But on Windows we can simply return a std::string_view since no heap allocation is necessary.
	//				We could of course handle non-MSVC compilers explicitly just like we do with MSVC, but it's not worth it imo. I may end up merging this with Jay's TypeDescriptor
	//				file if he thinks they fit together.

#if defined(HZ_ABI_SUPPORTED)
	using TypeNameString = std::string;

	template<typename T, bool ExcludeNamespace>
	struct TypeInfoBase
	{
	protected:
		TypeNameString DemangleTypeName(const std::string& typeName) const
		{
			size_t bufferLength = 0;
			int status = 0;
			char* buffer = abi::__cxa_demangle(typeName.c_str(), NULL, &bufferLength, &status);
			TypeNameString result = TypeNameString(buffer);
			free(buffer);

			if constexpr (ExcludeNamespace)
			{
				size_t namespacePos = result.find("::");

#ifndef HZ_PLATFORM_WINDOWS
				if (namespacePos != TypeNameString::npos)
					return result.substr(namespacePos + 2);
#endif
			}

			return result;
		}
	};
#else
	using TypeNameString = std::string_view;

	template<typename T, bool ExcludeNamespace>
	struct TypeInfoBase
	{
	protected:
		TypeNameString DemangleTypeName(std::string_view typeName) const
		{
			size_t spacePos = typeName.find(' ');
			if (spacePos != std::string_view::npos)
				typeName.remove_prefix(spacePos + 1);

			if constexpr (ExcludeNamespace)
			{
				size_t namespacePos = typeName.find("::");
				if (namespacePos != std::string_view::npos)
					typeName.remove_prefix(namespacePos + 2);
			}

			return typeName;
		}
	};

#endif

	template<typename T, bool ExcludeNamespace = false>
	struct TypeInfo : TypeInfoBase<T, ExcludeNamespace>
	{
	public:
		using Base = TypeInfoBase<T, ExcludeNamespace>;

	public:
		TypeInfo()
			: m_DemangledName(Base::DemangleTypeName(typeid(T).name()))
		{
		}

		TypeNameString Name() { return m_DemangledName; }
		const TypeNameString& Name() const { return m_DemangledName; }
		uint32_t HashCode() const { return Hash::GenerateFNVHash(m_DemangledName.data()); }

	private:
		TypeNameString m_DemangledName;
	};
}

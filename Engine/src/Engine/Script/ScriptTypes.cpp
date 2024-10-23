#include "hzpch.h"
#include "ScriptTypes.h"

#include "ScriptCache.h"
#include "ScriptUtils.h"

#include <mono/metadata/class.h>
#include <mono/metadata/tokentype.h>
#include <mono/metadata/reflection.h>

namespace Hazel {

	namespace TypeUtils {
		bool ContainsAttribute(void* attributeList, const std::string& attributeName)
		{
			const ManagedClass* attributeClass = ScriptCache::GetManagedClassByName(attributeName);

			if (attributeClass == nullptr)
				return false;

			if (attributeList == nullptr)
				return false;

			return mono_custom_attrs_has_attr((MonoCustomAttrInfo*)attributeList, attributeClass->Class);
		}
	}

}

#include "hzpch.h"
#include "NodeGraphUtils.h"
#include "Nodes.h"
#include "Hazel/Reflection/TypeName.h"

namespace Hazel::Utils {

	std::string GetTypeName(const choc::value::Value& v)
	{
		bool isArray = v.isArray();
		choc::value::Type type = isArray ? v.getType().getElementType() : v.getType();

		if (type.isFloat())                                                return "Float";
		else if (type.isInt32())                                           return "Int";
		else if (type.isBool())                                            return "Bool";
		else if (type.isString())                                          return "String";
		else if (type.isVoid())                                            return "Trigger";
		else if (IsAssetHandle<AssetType::Audio>(type))                    return "AudioAsset";
		else if (IsAssetHandle<AssetType::Animation>(type))                return "AnimationAsset";
		else if (IsAssetHandle<AssetType::Skeleton>(type))                 return "SkeletonAsset";
		else if (type.isObjectWithClassName(type::type_name<glm::vec3>())) return "Vec3";
		else HZ_CORE_ERROR("Custom object type encountered.  This needs to be handled!");

		return "invalid";
	}


	choc::value::Type GetTypeFromName(const std::string_view name)
	{
		if (name == "Float")               return choc::value::Type::createFloat32();
		else if (name == "Int")            return choc::value::Type::createInt32();
		else if (name == "Bool")           return choc::value::Type::createBool();
		else if (name == "String")         return choc::value::Type::createString();
		else if (name == "Trigger")        return choc::value::Type();
		else if (name == "AudioAsset")     return CreateAssetHandleType<AssetType::Audio>();
		else if (name == "AnimationAsset") return CreateAssetHandleType<AssetType::Animation>();
		else if (name == "SkeletonAsset")  return CreateAssetHandleType<AssetType::Skeleton>();
		else if (name == "Vec3")           return ValueFrom(glm::vec3{}).getType();
		else HZ_CORE_ERROR("Unsupported type");
		return choc::value::Type::createInt32();

	}

}

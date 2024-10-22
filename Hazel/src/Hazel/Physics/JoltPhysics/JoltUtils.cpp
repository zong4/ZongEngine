#include "hzpch.h"
#include "JoltUtils.h"

namespace Hazel::JoltUtils {

	JPH::Vec3 ToJoltVector(const glm::vec3& vector) { return JPH::Vec3(vector.x, vector.y, vector.z); }
	JPH::Quat ToJoltQuat(const glm::quat& quat) { return JPH::Quat(quat.x, quat.y, quat.z, quat.w); }

	glm::vec3 FromJoltVector(const JPH::Vec3& vector) { return *(glm::vec3*)&vector; }
	glm::quat FromJoltQuat(const JPH::Quat& quat) { return *(glm::quat*)&quat; }

	JPH::EMotionQuality ToJoltMotionQuality(ECollisionDetectionType collisionType)
	{
		switch (collisionType)
		{
			case ECollisionDetectionType::Discrete: return JPH::EMotionQuality::Discrete;
			case ECollisionDetectionType::Continuous: return JPH::EMotionQuality::LinearCast;
		}

		return JPH::EMotionQuality::Discrete;
	}

	JPH::EMotionType ToJoltMotionType(EBodyType bodyType)
	{
		switch (bodyType)
		{
			case EBodyType::Static: return JPH::EMotionType::Static;
			case EBodyType::Dynamic: return JPH::EMotionType::Dynamic;
			case EBodyType::Kinematic: return JPH::EMotionType::Kinematic;
		}

		return JPH::EMotionType::Static;
	}

}

#pragma once

struct Pose;
class Skeleton;

namespace Hazel::Utils::Animation {

	void BlendBoneTransforms(const Pose* pose0, const Pose* pose1, const float w, Pose* result);
	void BlendBoneTransforms(const Pose* pose0, const Pose* pose1, const float w, const Skeleton* skeleton, const uint32_t blendRootBone, Pose* result);
	void BlendRootMotion(const Pose* pose0, const Pose* pose1, const float w, Pose* result);

	void AdditiveBlendBoneTransforms(const Pose* pose0, const Pose* pose1, const float w, const Skeleton* skeleton, const uint32_t blendRootBone, Pose* result);
	void AdditiveBlendRootMotion(const Pose* pose0, const Pose* pose1, const float w, Pose* result);

}

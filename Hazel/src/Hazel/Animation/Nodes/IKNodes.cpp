#include "hzpch.h"
#include "IKNodes.h"

#include "Hazel/Debug/Profiler.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/common.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

namespace Hazel::AnimationGraph {

	// Aim IK translated from ozz::animation and related sample code
	// https://guillaumeblanc.github.io/ozz-animation/


	// Computes the transformation of a Quaternion and a vector _v.
	// This is equivalent to carrying out the quaternion multiplications:
	// _q.conjugate() * (*this) * _q
	inline static glm::vec3 TransformVector(const glm::quat& q, const glm::vec3& v)
	{
		// http://www.neil.dantam.name/note/dantam-quaternion.pdf
		// _v + 2.f * cross(_q.xyz, cross(_q.xyz, _v) + _q.w * _v);
		const glm::vec3 a = {
			q.y * v.z - q.z * v.y + v.x * q.w,
			q.z * v.x - q.x * v.z + v.y * q.w,
			q.x * v.y - q.y * v.x + v.z * q.w
		};
		const glm::vec3 b = {
			q.y * a.z - q.z * a.y,
			q.z * a.x - q.x * a.z,
			q.x * a.y - q.y * a.x
		};
		return { v.x + b.x + b.x, v.y + b.y + b.y, v.z + b.z + b.z };
	}


	inline static glm::vec3 TransformVector(const glm::mat4& m, const glm::vec3& v)
	{
		return glm::vec3(m * glm::vec4(v, 0.0f));
	}


	inline static glm::vec3 TransformPoint(const glm::mat4& m, const glm::vec3& v)
	{
		return glm::vec3(m * glm::vec4(v, 1.0f));
	}


	// When there's an offset, the forward vector needs to be recomputed.
	// The idea is to find the vector that will allow the point at offset position
	// to aim at target position. This vector starts at joint position. It ends on a
	// line perpendicular to pivot-offset line, at the intersection with the sphere
	// defined by target position (centered on joint position). See geogebra
	// diagram: media/doc/src/ik_aim_offset.ggb
	inline static bool ComputeOffsettedForward(glm::vec3 forward, glm::vec3 offset, glm::vec3 target, glm::vec3* offsettedForward)
	{
		// AO is projected offset vector onto the normalized forward vector.
		const float AOl = glm::dot(forward, offset);

		// Compute square length of ac using Pythagorean theorem.
		const float ACl2 = glm::length2(offset) - AOl * AOl;

		// Square length of target vector, aka circle radius.
		const float r2 = glm::length2(target);

		// If offset is outside of the sphere defined by target length, the target isn't reachable.
		if (ACl2 > r2)
		{
			return false;
		}

		// AIl is the length of the vector from offset to sphere intersection.
		const float AIl = glm::sqrt(r2 - ACl2);

		// The distance from offset position to the intersection with the sphere is
		// (AIl - AOl) Intersection point on the sphere can thus be computed.
		*offsettedForward = offset + (AIl - AOl) * forward;

		return true;
	}


	inline static glm::quat FromAxisAndCosAngle(const glm::vec3& axis, float cosAngle)
	{
		const float halfCos2 = 0.5f * (1.0f + cosAngle);
		const float halfSin2 = 1.0f - halfCos2;
		const glm::vec2 halfSinCos2(halfSin2, halfCos2);
		const glm::vec2 halfSinCos = glm::sqrt(halfSinCos2);
		return { halfSinCos.y, halfSinCos.x * axis };
	}


	AimIK::AimIK(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}


	void AimIK::Init(const Skeleton* skeleton)
	{
		EndpointUtilities::InitializeInputs(this);

		m_Skeleton = skeleton;
		Pose defaultPose;
		HZ_CORE_ASSERT(sizeof(Pose) == out_Pose.getRawDataSize());
		memcpy(out_Pose.getRawData(), &defaultPose, sizeof(Pose));
	}


	float AimIK::Process(float timestep)
	{
		HZ_PROFILE_FUNC();

		Pose* pose = static_cast<Pose*>(out_Pose.getRawData());

		if (in_Pose)
		{
			memcpy(pose, in_Pose, sizeof(Pose));
		}

		// Early out if weight is zero
		if (*in_Weight <= 0.0f || !m_Skeleton)
		{
			return 0.0f;
		}

		// Convert pose to model-space matrices
		// Remember: boneTransform 0 is the root, which is not part of the skeleton.  Skeleton bones are 1..num bones.
		HZ_CORE_ASSERT(pose->NumBones == m_Skeleton->GetNumBones() + 1);
		std::vector<glm::mat4> boneTransforms(pose->NumBones);
		for (uint32_t i = 0; i < pose->NumBones; ++i)
		{
			LocalTransform& transform = pose->BoneTransforms[i];
			glm::mat4 boneTransform = glm::translate(glm::mat4(1.0f), transform.Translation)
				* glm::toMat4(transform.Rotation)
				* glm::scale(glm::mat4(1.0f), transform.Scale);
			auto parentIndex = i == 0? Skeleton::NullIndex : m_Skeleton->GetParentBoneIndex(i-1) + 1;
			boneTransforms[i] = (parentIndex == Skeleton::NullIndex) ? boneTransform : boneTransforms[parentIndex] * boneTransform;
		}

		// The algorithm iteratively updates from the first joint (closer to the
		// head) to the last (the further ancestor, closer to the pelvis). Joints
		// order is already validated. For the first joint, aim IK is applied with
		// the global forward and offset, so the forward vector aligns in direction
		// of the target. If a weight lower that 1 is provided to the first joint,
		// then it will not fully align to the target. In this case further joint
		// will need to be updated. For the remaining joints, forward vector and
		// offset position are computed in each joint local-space, before IK is
		// applied:
		// 1. Rotates forward and offset position based on the result of the
		// previous joint IK.
		// 2. Brings forward and offset back in joint local-space.
		// Aim is iteratively applied up to the last selected joint of the
		// hierarchy. A weight of 1 is given to the last joint so we can guarantee
		// target is reached. Note that model-space transform of each joint doesn't
		// need to be updated between each pass, as joints are ordered from child to
		// parent.
		uint32_t prevBone = Skeleton::NullIndex;
		uint32_t bone = *in_Bone;

		glm::vec3 offset;
		glm::vec3 forward;
		glm::quat correction;
		int i = 0;
		while ((i < *in_ChainLength) && (bone != 0))
		{
			// TODO: check for singular matrix
			glm::mat4 invBoneTransform = glm::inverse(boneTransforms[bone]);
			glm::vec3 up = TransformVector(invBoneTransform, *in_PoleVector);

			// Setup offset and forward vector for the current joint being processed.
			if (i == 0)
			{
				// First joint, uses global forward and offset.
				offset = *in_AimOffset;
				forward = *in_AimAxis;
			}
			else
			{
				// Applies previous correction to "forward" and "offset", before bringing them to model-space
				const glm::vec3 correctedForward = TransformVector(boneTransforms[prevBone], TransformVector(correction, forward));
				const glm::vec3 correctedOffset = TransformPoint(boneTransforms[prevBone], TransformVector(correction, offset));

				// Brings "forward" and "offset" to bone-space
				forward = TransformVector(invBoneTransform, correctedForward);
				offset = TransformPoint(invBoneTransform, correctedOffset);
			}

			// Computes joint to target vector, in bone space.
			const glm::vec3 boneToTarget = TransformPoint(invBoneTransform, *in_Target);
			const float boneToTargetLen2 = glm::length2(boneToTarget);

			// Recomputes forward vector to account for offset.
			// If the offset is further than target, it won't be reachable.
			glm::vec3 offsettedForward;
			correction = glm::identity<glm::quat>();
			if (boneToTargetLen2 > 0.0000001 && ComputeOffsettedForward(forward, offset, boneToTarget, &offsettedForward))
			{
				// Calculates boneToTargetRotation quaternion which solves for offsetted_forward vector rotating onto the target.
				const glm::quat boneToTargetRotation(offsettedForward, boneToTarget);

				// Calculates rotate_plane quaternion which aligns joint up to the pole vector.
				const glm::vec3 correctedUp = TransformVector(boneToTargetRotation, up);

				// Compute (and normalize) reference and pole planes normals.
				const glm::vec3 poleVector = TransformVector(invBoneTransform, { 0.0f, 1.0f, 0.0f });  // TODO: should model-space pole vector be a parameter?
				const glm::vec3 refBoneNormal = glm::cross(poleVector, boneToTarget);
				const glm::vec3 boneNormal = glm::cross(correctedUp, boneToTarget);
				const float refBoneNormalLen2 = glm::length2(refBoneNormal);
				const float boneNormalLen2 = glm::length2(boneNormal);

				glm::vec3 planeRotationAxis;
				glm::quat planeRotation;
				// Computing rotation axis and plane requires valid normals.
				if ((boneToTargetLen2 > 0.0000001) && (boneNormalLen2 > 0.0000001) && (refBoneNormalLen2 > 0.0000001))
				{
					const glm::vec3 rsqrts = glm::inversesqrt(glm::vec3{ boneToTargetLen2, boneNormalLen2, refBoneNormalLen2 });

					// Computes rotation axis, which is either boneToTarget or // -boneToTarget depending on rotation direction.
					planeRotationAxis = rsqrts.x * boneToTarget;

					// Computes angle cosine between the 2 normalized plane normals.
					const float planeRotationCosAngle = glm::dot(boneNormal * rsqrts.y, refBoneNormal * rsqrts.z);
					const bool axisFlip = glm::dot(refBoneNormal, correctedUp) < 0.0f;
					const glm::vec3 planeRotationAxisFlipped = axisFlip ? -planeRotationAxis : planeRotationAxis;

					// Builds quaternion along rotation axis.
					planeRotation = FromAxisAndCosAngle(planeRotationAxisFlipped, planeRotationCosAngle);
				}
				else
				{
					planeRotationAxis = boneToTarget * glm::inversesqrt(boneToTargetLen2);
					planeRotation = glm::identity<glm::quat>();
				}

				correction = planeRotation * boneToTargetRotation;

				// Weights output quaternion.

				// Fix up quaternions so w is always positive, which is required for NLerp
				// (with identity quaternion) to lerp the shortest path.
				correction *= correction.w < 0.0f ? -1.0f : 1.0f;

				float weight = glm::clamp(*in_Weight, 0.0f, 1.0f) * ((i == *in_ChainLength - 1)? 1.0f : *in_ChainFactor);
				if (weight < 1.0f)
				{
					// NLerp start and mid joint rotations.
					correction = glm::normalize(glm::mix(glm::identity<glm::quat>(), correction, weight));
				}
			}

			// apply correction to pose.
			pose->BoneTransforms[bone].Rotation = correction * pose->BoneTransforms[bone].Rotation;

			prevBone = bone;
			bone = m_Skeleton->GetParentBoneIndex(bone - 1) + 1;
			++i;
		}

		return 0.0f;
	}

}

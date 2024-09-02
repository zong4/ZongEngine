#include "hzpch.h"
#include "BlendUtils.h"

#include "Animation.h"
#include "Skeleton.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/common.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/gtx/norm.hpp>

namespace Hazel::Utils::Animation {

	void BlendBoneTransforms(const Pose* pose0, const Pose* pose1, const float w, Pose* result)
	{
		HZ_CORE_ASSERT(pose0->NumBones == pose1->NumBones, "Poses have different number of bones");

		uint32_t i = 0;
		uint32_t N = glm::min(pose0->NumBones, pose1->NumBones);

		// Note (0x): lerp for root bone translation is not ideal (e.g. speed will not be preserved when combining forwards movement and rightwards movement to make a diagonal)
		result->BoneTransforms[0].Translation = glm::mix(pose0->BoneTransforms[0].Translation, pose1->BoneTransforms[0].Translation, w);
		result->BoneTransforms[0].Rotation = glm::slerp(pose0->BoneTransforms[0].Rotation, pose1->BoneTransforms[0].Rotation, w);
		result->BoneTransforms[0].Scale = glm::mix(pose0->BoneTransforms[0].Scale, pose1->BoneTransforms[0].Scale, w);
		for (i = 1; i < N; ++i)
		{
			result->BoneTransforms[i].Translation = glm::mix(pose0->BoneTransforms[i].Translation, pose1->BoneTransforms[i].Translation, w);
			result->BoneTransforms[i].Rotation = glm::slerp(pose0->BoneTransforms[i].Rotation, pose1->BoneTransforms[i].Rotation, w);
			result->BoneTransforms[i].Scale = glm::mix(pose0->BoneTransforms[i].Scale, pose1->BoneTransforms[i].Scale, w);
		}
	}


	void BlendBoneTransforms(const Pose* pose0, const Pose* pose1, const float w, const Skeleton* skeleton, const uint32_t blendRootBone, Pose* result)
	{
		HZ_CORE_ASSERT(pose0->NumBones == pose1->NumBones, "Poses have different number of bones");

		uint32_t i = 0;
		uint32_t N = glm::min(pose0->NumBones, pose1->NumBones);

		if (blendRootBone == 0)
		{
			// Note (0x): lerp for root bone translation is not ideal (e.g. speed will not be preserved when combining forwards movement and rightwards movement to make a diagonal)
			result->BoneTransforms[0].Translation = glm::mix(pose0->BoneTransforms[0].Translation, pose1->BoneTransforms[0].Translation, w);
			result->BoneTransforms[0].Rotation = glm::slerp(pose0->BoneTransforms[0].Rotation, pose1->BoneTransforms[0].Rotation, w);
			result->BoneTransforms[0].Scale = glm::mix(pose0->BoneTransforms[0].Scale, pose1->BoneTransforms[0].Scale, w);
			for (i = 1; i < N; ++i)
			{
				result->BoneTransforms[i].Translation = glm::mix(pose0->BoneTransforms[i].Translation, pose1->BoneTransforms[i].Translation, w);
				result->BoneTransforms[i].Rotation = glm::slerp(pose0->BoneTransforms[i].Rotation, pose1->BoneTransforms[i].Rotation, w);
				result->BoneTransforms[i].Scale = glm::mix(pose0->BoneTransforms[i].Scale, pose1->BoneTransforms[i].Scale, w);
			}
			return;
		}

		for (; i < blendRootBone; ++i)
		{
			result->BoneTransforms[i] = pose0->BoneTransforms[i];
		}

		uint32_t parentBoneIndex = skeleton->GetParentBoneIndex(blendRootBone - 1) + 1;

		for (; i < N; ++i)
		{
			if ((skeleton->GetParentBoneIndex(i - 1) + 1 <= parentBoneIndex) && (i > blendRootBone))
				break;

			result->BoneTransforms[i].Translation = glm::mix(pose0->BoneTransforms[i].Translation, pose1->BoneTransforms[i].Translation, w);
			result->BoneTransforms[i].Rotation = glm::slerp(pose0->BoneTransforms[i].Rotation, pose1->BoneTransforms[i].Rotation, w);
			result->BoneTransforms[i].Scale = glm::mix(pose0->BoneTransforms[i].Scale, pose1->BoneTransforms[i].Scale, w);
		}

		for (; i < N; ++i)
		{
			result->BoneTransforms[i] = pose0->BoneTransforms[i];
		}
	}


	void BlendRootMotion(const Pose* pose0, const Pose* pose1, const float w, Pose* result)
	{
		// Note (0x): The root motion translation is actually a heading, and lerp is not ideal as it does not preserve the length of the heading.
		result->RootMotion.Translation = glm::mix(pose0->RootMotion.Translation, pose1->RootMotion.Translation, w);
		result->RootMotion.Rotation = glm::slerp(pose0->RootMotion.Rotation, pose1->RootMotion.Rotation, w);
		result->RootMotion.Scale = glm::mix(pose0->RootMotion.Scale, pose1->RootMotion.Scale, w);
	}


	void AdditiveBlendBoneTransforms(const Pose* pose0, const Pose* pose1, const float w, const Skeleton* skeleton, const uint32_t blendRootBone, Pose* result)
	{
		HZ_CORE_ASSERT(pose0->NumBones == pose1->NumBones, "Poses have different number of bones");

		uint32_t i = 0;
		uint32_t N = glm::min(pose0->NumBones, pose1->NumBones);

		if (blendRootBone == 0)
		{
			result->BoneTransforms[i].Translation = pose0->BoneTransforms[i].Translation + w * pose1->BoneTransforms[i].Translation;
			glm::quat weightedRotation = glm::slerp(glm::identity<glm::quat>(), pose1->BoneTransforms[i].Rotation, w);
			result->BoneTransforms[i].Rotation = pose0->BoneTransforms[i].Rotation * weightedRotation;
			result->BoneTransforms[i].Scale = pose0->BoneTransforms[i].Scale + w * (pose1->BoneTransforms[i].Scale - glm::one<glm::vec3>());
			++i;
			for (; i < N; ++i)
			{
				// result is pose0 + w(pose1 - restPose)
				result->BoneTransforms[i].Translation = pose0->BoneTransforms[i].Translation + w * (pose1->BoneTransforms[i].Translation - skeleton->GetBoneTranslations().at(i - 1));
				glm::quat restRotation = skeleton->GetBoneRotations().at(i - 1);
				glm::quat weightedRotation = glm::slerp(restRotation, pose1->BoneTransforms[i].Rotation, w);
				result->BoneTransforms[i].Rotation = pose0->BoneTransforms[i].Rotation * glm::conjugate(restRotation) * weightedRotation;
				result->BoneTransforms[i].Scale = pose0->BoneTransforms[i].Scale + w * (pose1->BoneTransforms[i].Scale - skeleton->GetBoneScales().at(i - 1));
			}
			return;
		}

		for (; i < blendRootBone; ++i)
		{
			result->BoneTransforms[i] = pose0->BoneTransforms[i];
		}

		uint32_t parentBoneIndex = skeleton->GetParentBoneIndex(blendRootBone - 1) + 1;

		for (; i < N; ++i)
		{
			if ((skeleton->GetParentBoneIndex(i - 1) + 1 <= parentBoneIndex) && (i > blendRootBone))
				break;

			// result is pose0 + w(pose1 - restPose)
			result->BoneTransforms[i].Translation = pose0->BoneTransforms[i].Translation + w * (pose1->BoneTransforms[i].Translation - skeleton->GetBoneTranslations().at(i - 1));
			glm::quat restRotation = skeleton->GetBoneRotations().at(i - 1);
			glm::quat weightedRotation = glm::slerp(restRotation, pose1->BoneTransforms[i].Rotation, w);
			result->BoneTransforms[i].Rotation = pose0->BoneTransforms[i].Rotation * glm::conjugate(restRotation) * weightedRotation;
			result->BoneTransforms[i].Scale = pose0->BoneTransforms[i].Scale + w * (pose1->BoneTransforms[i].Scale - skeleton->GetBoneScales().at(i - 1));
		}

		for (; i < N; ++i)
		{
			result->BoneTransforms[i] = pose0->BoneTransforms[i];
		}
	}


	void AdditiveBlendRootMotion(const Pose* pose0, const Pose* pose1, const float w, Pose* result)
	{
		result->RootMotion.Translation = pose0->RootMotion.Translation + w * pose1->RootMotion.Translation;
		result->RootMotion.Rotation = pose0->RootMotion.Rotation * (w * pose1->RootMotion.Rotation);
		result->RootMotion.Scale = pose0->RootMotion.Scale + w * (pose1->RootMotion.Scale - glm::one<glm::vec3>());
	}

}

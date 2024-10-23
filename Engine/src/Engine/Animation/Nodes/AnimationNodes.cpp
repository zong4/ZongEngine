#include "pch.h"
#include "AnimationNodes.h"

#include "NodeDescriptors.h"
#include "Engine/Animation/Animation.h"
#include "Engine/Asset/AssetManager.h"
#include "Engine/Debug/Profiler.h"

namespace Hazel::AnimationGraph {

	AnimationPlayer::AnimationPlayer(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}


	void AnimationPlayer::Init()
	{
		EndpointUtilities::InitializeInputs(this);
		m_PreviousAnimation = 0;
		m_AnimationTimePos = *in_Offset;
		m_PreviousAnimationTimePos = m_AnimationTimePos;
		m_PreviousOffset = *in_Offset;

		Pose defaultPose;
		ZONG_CORE_ASSERT(sizeof(Pose) == out_Pose.getRawDataSize());
		memcpy(out_Pose.getRawData(), &defaultPose, sizeof(Pose));
	}


	float AnimationPlayer::Process(float timestep)
	{
		ZONG_PROFILE_FUNC();

		Pose* pose = reinterpret_cast<Pose*>(out_Pose.getRawData());

		if (m_PreviousAnimation != *in_Animation)
		{
			auto animationAsset = AssetManager::GetAsset<AnimationAsset>(*in_Animation);
			if (animationAsset && animationAsset->IsValid())
			{
				m_Animation = animationAsset->GetAnimation();
			}
			if (m_Animation)
			{
				m_TranslationCache.Reset(m_Animation->GetNumTracks(), m_Animation->GetTranslationKeys());
				m_RotationCache.Reset(m_Animation->GetNumTracks(), m_Animation->GetRotationKeys());
				m_ScaleCache.Reset(m_Animation->GetNumTracks(), m_Animation->GetScaleKeys());
				ZONG_CORE_ASSERT(m_Animation->GetTranslationKeys()[0].FrameTime == 0.0f);
				ZONG_CORE_ASSERT(m_Animation->GetTranslationKeys()[0].Track == 0);
				m_RootTranslationStart = m_Animation->GetRootTranslationStart();
				m_RootRotationStart = m_Animation->GetRootRotationStart();
				m_RootTranslationEnd = m_Animation->GetRootTranslationEnd();
				m_RootRotationEnd = m_Animation->GetRootRotationEnd();
				pose->BoneTransforms[0].Translation = m_RootTranslationStart;
				pose->BoneTransforms[0].Rotation = m_RootRotationStart;
				pose->AnimationDuration = m_Animation->GetDuration();
				pose->NumBones = m_Animation->GetNumTracks();
			}
			m_AnimationTimePos = *in_Offset;
			m_PreviousAnimationTimePos = m_AnimationTimePos;
			m_PreviousAnimation = *in_Animation;
		}

		if (m_Animation)
		{
			glm::vec3 previousRootTranslation = pose->BoneTransforms[0].Translation;
			glm::quat previousRootRotation = pose->BoneTransforms[0].Rotation;

			// start offset is 0 = beginning of anim, 1 = end of anim.
			// wrapping outside of [0, 1] due to offset changing is allowed, and does not ping looped events.
			m_AnimationTimePos += *in_Offset - m_PreviousOffset;
			m_PreviousOffset = *in_Offset;
			m_AnimationTimePos -= floorf(m_AnimationTimePos);

			// m_UpdateValue is in seconds, and needs to be normalized to the animation duration
			m_AnimationTimePos += timestep * *in_PlaybackSpeed / m_Animation->GetDuration();

			if (*in_PlaybackSpeed >= 0.0f)
			{
				while (m_AnimationTimePos > 1.0f)
				{
					if (*in_Loop)
					{
						m_AnimationTimePos -= 1.0f;
						out_OnLoop(IDs::Loop);
					}
					else
					{
						m_AnimationTimePos = 1.0f;
						out_OnFinish(IDs::Finish);
					}
				}
			}
			else
			{
				while (m_AnimationTimePos < 0.0f)
				{
					if (*in_Loop)
					{
						m_AnimationTimePos += 1.0f;
						out_OnLoop(IDs::Loop);
					}
					else
					{
						m_AnimationTimePos = 0.0f;
						out_OnFinish(IDs::Finish);
					}
				}
			}

			m_TranslationCache.Step(m_AnimationTimePos, m_Animation->GetTranslationKeys());
			m_RotationCache.Step(m_AnimationTimePos, m_Animation->GetRotationKeys());
			m_ScaleCache.Step(m_AnimationTimePos, m_Animation->GetScaleKeys());

			m_TranslationCache.Interpolate<0>(m_AnimationTimePos, &pose->BoneTransforms[0], [](const glm::vec3& a, const glm::vec3& b, const float t) {return glm::mix(a, b, t); });
			m_RotationCache.Interpolate<1>(m_AnimationTimePos, &pose->BoneTransforms[0], [](const glm::quat& a, const glm::quat& b, const float t) {return glm::slerp(a, b, t); });
			m_ScaleCache.Interpolate<2>(m_AnimationTimePos, &pose->BoneTransforms[0], [](const glm::vec3& a, const glm::vec3& b, const float t) {return glm::mix(a, b, t); });

			// Work out root motion by looking at change in pose of root bone.
			// Bear in mind some tricky cases:
			// 1) The animation might have looped back around to the beginning.
			// 2) We might be playing the animation backwards.
			// 3) We might have paused the animation, and be "debugging" it, stepping backwards (or forwards) deliberately.
			glm::vec3 newRootTranslation = pose->BoneTransforms[0].Translation;
			glm::quat newRootRotation = pose->BoneTransforms[0].Rotation;
			if (*in_PlaybackSpeed >= 0.0f)
			{
				if (m_AnimationTimePos >= m_PreviousAnimationTimePos)
				{
					// Animation playing forwards, and has not looped
					pose->RootMotion.Translation = newRootTranslation - previousRootTranslation;
					pose->RootMotion.Rotation = glm::conjugate(previousRootRotation) * newRootRotation;
				}
				else
				{
					// Animation playing forwards, and has looped
					pose->RootMotion.Translation = m_RootTranslationEnd - previousRootTranslation + newRootTranslation - m_RootTranslationStart;
					pose->RootMotion.Rotation = (glm::conjugate(previousRootRotation) * m_RootRotationEnd) * (glm::conjugate(m_RootRotationStart) * newRootRotation);
				}
			}
			else
			{
				if (m_AnimationTimePos <= m_PreviousAnimationTimePos)
				{
					// Animation playing backwards, and has not looped
					pose->RootMotion.Translation = newRootTranslation - previousRootTranslation;
					pose->RootMotion.Rotation = glm::conjugate(previousRootRotation) * newRootRotation;
				}
				else
				{
					// Animation playing backwards, and has looped
					pose->RootMotion.Translation = m_RootTranslationStart - previousRootTranslation + newRootTranslation - m_RootTranslationEnd;
					pose->RootMotion.Rotation = (glm::conjugate(previousRootRotation) * m_RootRotationStart) * (glm::conjugate(m_RootRotationEnd) * newRootRotation);
				}
			}

			m_PreviousAnimationTimePos = m_AnimationTimePos;
		}
		pose->RootMotion.Translation *= *in_PlaybackSpeed;
		pose->AnimationTimePos = m_AnimationTimePos;
		return 0.0f;
	}


	void AnimationPlayer::SetAnimationTimePos(float time)
	{
		m_PreviousAnimationTimePos = time;
		m_AnimationTimePos = time;
	}


	RangedBlend::RangedBlend(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		// Inputs
		EndpointUtilities::RegisterEndpoints(this);
	}


	void RangedBlend::Init()
	{
		EndpointUtilities::InitializeInputs(this);
		m_PreviousAnimationA = 0;
		m_PreviousAnimationB = 0;
		m_AnimationTimePosA = *in_OffsetA;
		m_AnimationTimePosB = *in_OffsetB;
		m_PreviousAnimationTimePosA = m_AnimationTimePosA;
		m_PreviousAnimationTimePosB = m_AnimationTimePosB;

		Pose defaultPose;
		ZONG_CORE_ASSERT(sizeof(Pose) == out_Pose.getRawDataSize());
		memcpy(out_Pose.getRawData(), &defaultPose, sizeof(Pose));
	}


	float RangedBlend::Process(float timestep)
	{
		ZONG_PROFILE_FUNC();

		float w = glm::clamp((*in_Value - *in_RangeA) / (*in_RangeB - *in_RangeA), 0.0f, 1.0f);
		float weightedDuration = 0.0f;

		if (m_PreviousAnimationA != *in_AnimationA)
		{
			auto animationAsset = AssetManager::GetAsset<AnimationAsset>(*in_AnimationA);
			if (animationAsset && animationAsset->IsValid())
			{
				m_AnimationA = animationAsset->GetAnimation();
			}
			if (m_AnimationA)
			{
				m_TranslationCacheA.Reset(m_AnimationA->GetNumTracks(), m_AnimationA->GetTranslationKeys());
				m_RotationCacheA.Reset(m_AnimationA->GetNumTracks(), m_AnimationA->GetRotationKeys());
				m_ScaleCacheA.Reset(m_AnimationA->GetNumTracks(), m_AnimationA->GetScaleKeys());
				ZONG_CORE_ASSERT(m_AnimationA->GetTranslationKeys()[0].FrameTime == 0.0f);
				ZONG_CORE_ASSERT(m_AnimationA->GetTranslationKeys()[0].Track == 0);
				m_RootTranslationStartA = m_AnimationA->GetRootTranslationStart();
				m_RootRotationStartA = m_AnimationA->GetRootRotationStart();
				m_RootTranslationEndA = m_AnimationA->GetRootTranslationEnd();
				m_RootRotationEndA = m_AnimationA->GetRootRotationEnd();
				m_PoseA.BoneTransforms[0].Translation = m_RootTranslationStartA;
				m_PoseA.BoneTransforms[0].Rotation = m_RootRotationStartA;
			}
			m_AnimationTimePosA = *in_OffsetA;
			m_PreviousAnimationTimePosA = m_AnimationTimePosA;
			m_PreviousAnimationA = *in_AnimationA;
		}

		if (m_PreviousAnimationB != *in_AnimationB)
		{
			auto animationAsset = AssetManager::GetAsset<AnimationAsset>(*in_AnimationB);
			if (animationAsset && animationAsset->IsValid())
			{
				m_AnimationB = animationAsset->GetAnimation();
			}
			if (m_AnimationB)
			{
				m_TranslationCacheB.Reset(m_AnimationB->GetNumTracks(), m_AnimationB->GetTranslationKeys());
				m_RotationCacheB.Reset(m_AnimationB->GetNumTracks(), m_AnimationB->GetRotationKeys());
				m_ScaleCacheB.Reset(m_AnimationB->GetNumTracks(), m_AnimationB->GetScaleKeys());
				ZONG_CORE_ASSERT(m_AnimationB->GetTranslationKeys()[0].FrameTime == 0.0f);
				ZONG_CORE_ASSERT(m_AnimationB->GetTranslationKeys()[0].Track == 0);
				m_RootTranslationStartB = m_AnimationB->GetRootTranslationStart();
				m_RootRotationStartB = m_AnimationB->GetRootRotationStart();
				m_RootTranslationEndB = m_AnimationB->GetRootTranslationEnd();
				m_RootRotationEndB = m_AnimationB->GetRootRotationEnd();
				m_PoseB.BoneTransforms[0].Translation = m_RootTranslationStartB;
				m_PoseB.BoneTransforms[0].Rotation = m_RootRotationStartB;
			}
			m_AnimationTimePosB = *in_OffsetB;
			m_PreviousAnimationTimePosB = m_AnimationTimePosB;
			m_PreviousAnimationB = *in_AnimationB;
		}

		if (m_AnimationA && m_AnimationB)
		{
			weightedDuration = glm::mix(m_AnimationA->GetDuration(), m_AnimationB->GetDuration(), w);
			float weightedPlaybackSpeed = glm::mix(*in_PlaybackSpeedA, *in_PlaybackSpeedB, w);

			// start offset is 0 = beginning of anim, 1 = end of anim.
			// wrapping outside of [0, 1] due to offset changing is allowed, and does not ping looped events.
			m_AnimationTimePosA += *in_OffsetA - m_PreviousOffsetA;
			m_PreviousOffsetA = *in_OffsetA;
			m_AnimationTimePosA -= floorf(m_AnimationTimePosA);
			
			if (*in_Synchronize)
			{
				m_AnimationTimePosA += timestep * weightedPlaybackSpeed / weightedDuration;
				m_AnimationTimePosB = m_AnimationTimePosA;
			}
			else
			{
				m_AnimationTimePosA += timestep * *in_PlaybackSpeedA / m_AnimationA->GetDuration();
				m_AnimationTimePosB += timestep * *in_PlaybackSpeedB / m_AnimationB->GetDuration();
			}

			if (*in_PlaybackSpeedA >= 0.0f)
			{
				while (m_AnimationTimePosA > 1.0f)
				{
					if (*in_Loop)
					{
						m_AnimationTimePosA -= 1.0f;
						out_OnLoopA(IDs::LoopA);
					}
					else
					{
						m_AnimationTimePosA = 1.0f;
						out_OnFinishA(IDs::FinishA);
					}
				}
			}
			else
			{
				while (m_AnimationTimePosA < 0.0f)
				{
					if (*in_Loop)
					{
						m_AnimationTimePosA += 1.0f;
						out_OnLoopA(IDs::LoopA);
					}
					else
					{
						m_AnimationTimePosA = 0.0f;
						out_OnFinishA(IDs::FinishA);
					}
				}
			}
			m_TranslationCacheA.Step(m_AnimationTimePosA, m_AnimationA->GetTranslationKeys());
			m_RotationCacheA.Step(m_AnimationTimePosA, m_AnimationA->GetRotationKeys());
			m_ScaleCacheA.Step(m_AnimationTimePosA, m_AnimationA->GetScaleKeys());

			glm::vec3 previousRootTranslationA = m_PoseA.BoneTransforms[0].Translation;
			glm::quat previousRootRotationA = m_PoseA.BoneTransforms[0].Rotation;
			m_TranslationCacheA.Interpolate<0>(m_AnimationTimePosA, &m_PoseA.BoneTransforms[0], [](const glm::vec3& a, const glm::vec3& b, const float t) {return glm::mix(a, b, t); });
			m_RotationCacheA.Interpolate<1>(m_AnimationTimePosA, &m_PoseA.BoneTransforms[0], [](const glm::quat& a, const glm::quat& b, const float t) {return glm::slerp(a, b, t); });
			m_ScaleCacheA.Interpolate<2>(m_AnimationTimePosA, &m_PoseA.BoneTransforms[0], [](const glm::vec3& a, const glm::vec3& b, const float t) {return glm::mix(a, b, t); });

			// Work out root motion by looking at change in pose of root bone.
			// Bear in mind some tricky cases:
			// 1) The animation might have looped back around to the beginning.
			// 2) We might be playing the animation backwards.
			// 3) We might have paused the animation, and be "debugging" it, stepping backwards (or forwards) deliberately.
			glm::vec3 newRootTranslation = m_PoseA.BoneTransforms[0].Translation;
			glm::quat newRootRotation = m_PoseA.BoneTransforms[0].Rotation;
			if (*in_PlaybackSpeedA >= 0.0f)
			{
				if (m_AnimationTimePosA >= m_PreviousAnimationTimePosA)
				{
					// Animation playing forwards, and has not looped
					m_PoseA.RootMotion.Translation = newRootTranslation - previousRootTranslationA;
					m_PoseA.RootMotion.Rotation = glm::conjugate(previousRootRotationA) * newRootRotation;
				}
				else
				{
					// Animation playing forwards, and has looped
					m_PoseA.RootMotion.Translation = m_RootTranslationEndA - previousRootTranslationA + newRootTranslation - m_RootTranslationStartA;
					m_PoseA.RootMotion.Rotation = (glm::conjugate(previousRootRotationA) * m_RootRotationEndA) * (glm::conjugate(m_RootRotationStartA) * newRootRotation);
				}
			}
			else
			{
				if (m_AnimationTimePosA <= m_PreviousAnimationTimePosA)
				{
					// Animation playing backwards, and has not looped
					m_PoseA.RootMotion.Translation = newRootTranslation - previousRootTranslationA;
					m_PoseA.RootMotion.Rotation = glm::conjugate(previousRootRotationA) * newRootRotation;
				}
				else
				{
					// Animation playing backwards, and has looped
					m_PoseA.RootMotion.Translation = m_RootTranslationStartA - previousRootTranslationA + newRootTranslation - m_RootTranslationEndA;
					m_PoseA.RootMotion.Rotation = (glm::conjugate(previousRootRotationA) * m_RootRotationStartA) * (glm::conjugate(m_RootRotationEndA) * newRootRotation);
				}
			}
			m_PreviousAnimationTimePosA = m_AnimationTimePosA;

			// start offset is 0 = beginning of anim, 1 = end of anim.
			// wrapping outside of [0, 1] due to offset changing is allowed, and does not ping looped events.
			m_AnimationTimePosB += *in_OffsetB - m_PreviousOffsetB;
			m_PreviousOffsetB = *in_OffsetB;
			m_AnimationTimePosB -= floorf(m_AnimationTimePosB);


			if (*in_PlaybackSpeedB >= 0.0f)
			{
				while (m_AnimationTimePosB > 1.0f)
				{
					if (*in_Loop)
					{
						m_AnimationTimePosB -= 1.0f;
						out_OnLoopB(IDs::LoopB);
					}
					else
					{
						m_AnimationTimePosB = 1.0f;
						out_OnFinishB(IDs::FinishB);
					}
				}
			}
			else
			{
				while (m_AnimationTimePosB < 0.0f)
				{
					if (*in_Loop)
					{
						m_AnimationTimePosB += 1.0f;
						out_OnLoopB(IDs::LoopB);
					}
					else
					{
						m_AnimationTimePosB = 0.0f;
						out_OnFinishB(IDs::FinishB);
					}
				}
			}
			m_TranslationCacheB.Step(m_AnimationTimePosB, m_AnimationB->GetTranslationKeys());
			m_RotationCacheB.Step(m_AnimationTimePosB, m_AnimationB->GetRotationKeys());
			m_ScaleCacheB.Step(m_AnimationTimePosB, m_AnimationB->GetScaleKeys());

			glm::vec3 previousRootTranslationB = m_PoseB.BoneTransforms[0].Translation;
			glm::quat previousRootRotationB = m_PoseB.BoneTransforms[0].Rotation;
			m_TranslationCacheB.Interpolate<0>(m_AnimationTimePosB, &m_PoseB.BoneTransforms[0], [](const glm::vec3& a, const glm::vec3& b, const float t) {return glm::mix(a, b, t); });
			m_RotationCacheB.Interpolate<1>(m_AnimationTimePosB, &m_PoseB.BoneTransforms[0], [](const glm::quat& a, const glm::quat& b, const float t) {return glm::slerp(a, b, t); });
			m_ScaleCacheB.Interpolate<2>(m_AnimationTimePosB, &m_PoseB.BoneTransforms[0], [](const glm::vec3& a, const glm::vec3& b, const float t) {return glm::mix(a, b, t); });

			// Work out root motion by looking at change in pose of root bone.
			// Bear in mind some tricky cases:
			// 1) The animation might have looped back around to the beginning.
			// 2) We might be playing the animation backwards.
			// 3) We might have paused the animation, and be "debugging" it, stepping backwards (or forwards) deliberately.
			newRootTranslation = m_PoseB.BoneTransforms[0].Translation;
			newRootRotation = m_PoseB.BoneTransforms[0].Rotation;
			if (*in_PlaybackSpeedB >= 0.0f)
			{
				if (m_AnimationTimePosB >= m_PreviousAnimationTimePosB)
				{
					// Animation playing forwards, and has not looped
					m_PoseB.RootMotion.Translation = newRootTranslation - previousRootTranslationB;
					m_PoseB.RootMotion.Rotation = glm::conjugate(previousRootRotationB) * newRootRotation;
				}
				else
				{
					// Animation playing forwards, and has looped
					m_PoseB.RootMotion.Translation = m_RootTranslationEndB - previousRootTranslationB + newRootTranslation - m_RootTranslationStartB;
					m_PoseB.RootMotion.Rotation = (glm::conjugate(previousRootRotationB) * m_RootRotationEndB) * (glm::conjugate(m_RootRotationStartB) * newRootRotation);
				}
			}
			else
			{
				if (m_AnimationTimePosB <= m_PreviousAnimationTimePosB)
				{
					// Animation playing backwards, and has not looped
					m_PoseB.RootMotion.Translation = newRootTranslation - previousRootTranslationB;
					m_PoseB.RootMotion.Rotation = glm::conjugate(previousRootRotationB) * newRootRotation;
				}
				else
				{
					// Animation playing backwards, and has looped
					m_PoseB.RootMotion.Translation = m_RootTranslationStartB - previousRootTranslationB + newRootTranslation - m_RootTranslationEndB;
					m_PoseB.RootMotion.Rotation = (glm::conjugate(previousRootRotationB) * m_RootRotationStartB) * (glm::conjugate(m_RootRotationEndB) * newRootRotation);
				}
			}
			m_PreviousAnimationTimePosB = m_AnimationTimePosB;
		}

		Pose* result = reinterpret_cast<Pose*>(out_Pose.getRawData());
		ZONG_CORE_ASSERT(m_AnimationA->GetNumTracks() == m_AnimationB->GetNumTracks());
		result->NumBones = m_AnimationA->GetNumTracks();
		for (uint32_t i = 0; i < result->NumBones; i++)
		{
			result->BoneTransforms[i].Translation = glm::mix(m_PoseA.BoneTransforms[i].Translation, m_PoseB.BoneTransforms[i].Translation, w);
			result->BoneTransforms[i].Rotation = glm::slerp(m_PoseA.BoneTransforms[i].Rotation, m_PoseB.BoneTransforms[i].Rotation, w);
			result->BoneTransforms[i].Scale = glm::mix(m_PoseA.BoneTransforms[i].Scale, m_PoseB.BoneTransforms[i].Scale, w);
		}
		result->RootMotion.Translation = glm::mix(m_PoseA.RootMotion.Translation, m_PoseB.RootMotion.Translation, w);
		if (result->RootMotion.Translation != glm::vec3(0.0f))
		{
			int i = 42;
		}
		result->RootMotion.Rotation = glm::slerp(m_PoseA.RootMotion.Rotation, m_PoseB.RootMotion.Rotation, w);
		result->RootMotion.Scale = glm::mix(m_PoseA.RootMotion.Scale, m_PoseB.RootMotion.Scale, w);
		result->AnimationDuration = weightedDuration;
		result->AnimationTimePos = glm::mix(m_AnimationTimePosA, m_AnimationTimePosB, w);
		return 0.0f;
	}


	void RangedBlend::SetAnimationTimePos(float timePos)
	{
		m_PreviousAnimationTimePosA = timePos;
		m_PreviousAnimationTimePosB = timePos;
		m_AnimationTimePosA = timePos;
		m_AnimationTimePosB = timePos;
	}
}

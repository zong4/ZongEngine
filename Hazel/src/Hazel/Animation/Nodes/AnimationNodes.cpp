#include "hzpch.h"
#include "AnimationNodes.h"

#include "Hazel/Animation/Animation.h"
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Debug/Profiler.h"

namespace Hazel::AnimationGraph {

	AnimationPlayer::AnimationPlayer(const char* dbgName, UUID id)
	: NodeProcessor(dbgName, id)
	, m_TrackWriter(static_cast<Pose*>(out_Pose.getRawData()))
	{
		EndpointUtilities::RegisterEndpoints(this);
	}


	void AnimationPlayer::Init(const Skeleton*)
	{
		EndpointUtilities::InitializeInputs(this);
		m_PreviousAnimation = 0;
		m_AnimationTimePos = *in_Offset;
		m_PreviousAnimationTimePos = m_AnimationTimePos;
		m_PreviousOffset = *in_Offset;

		Pose defaultPose;
		HZ_CORE_ASSERT(sizeof(Pose) == out_Pose.getRawDataSize());
		memcpy(out_Pose.getRawData(), &defaultPose, sizeof(Pose));

		Process(0.0f);
	}


	float AnimationPlayer::Process(float timestep)
	{
		HZ_PROFILE_FUNC();

		Pose* pose = static_cast<Pose*>(out_Pose.getRawData());
		float animationDuration = 0.0f;

		if (m_PreviousAnimation != *in_Animation)
		{
			if (auto animationAsset = AssetManager::GetAsset<AnimationAsset>(*in_Animation); animationAsset)
			{
				m_Animation = animationAsset->GetAnimation();
			}
			if (m_Animation)
			{
				m_Context.initialize(*static_cast<const acl::compressed_tracks*>(m_Animation->GetData()));
				m_RootTranslationStart = m_Animation->GetRootTranslationStart();
				m_RootRotationStart = m_Animation->GetRootRotationStart();
				m_RootTranslationEnd = m_Animation->GetRootTranslationEnd();
				m_RootRotationEnd = m_Animation->GetRootRotationEnd();
				pose->BoneTransforms[0].Translation = m_RootTranslationStart;
				pose->BoneTransforms[0].Rotation = m_RootRotationStart;
				pose->NumBones = m_Animation->GetNumTracks();
			}
			m_AnimationTimePos = *in_Offset;
			m_PreviousAnimationTimePos = m_AnimationTimePos;
			m_PreviousAnimation = *in_Animation;
			m_IsFinished = false;
		}

		if (m_Animation)
		{
			glm::vec3 previousRootTranslation = pose->BoneTransforms[0].Translation;
			glm::quat previousRootRotation = pose->BoneTransforms[0].Rotation;
			animationDuration = m_Animation->GetDuration();

			if (!m_IsFinished)
			{
				// start offset is 0 = beginning of anim, 1 = end of anim.
				// wrapping outside of [0, 1] due to offset changing is allowed, and does not ping looped events.
				m_AnimationTimePos += *in_Offset - m_PreviousOffset;
				m_PreviousOffset = *in_Offset;
				m_AnimationTimePos -= floorf(m_AnimationTimePos);

				// timestep is in seconds, and needs to be normalized to the animation duration.
				m_AnimationTimePos += timestep * *in_PlaybackSpeed / animationDuration;

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
							m_IsFinished = true;
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
							m_IsFinished = true;
							out_OnFinish(IDs::Finish);
						}
					}
				}
			}

			m_Context.seek(m_AnimationTimePos * animationDuration, acl::sample_rounding_policy::none);
			m_Context.decompress_tracks(m_TrackWriter);

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
		if (auto absPlaybackSpeed = fabs(*in_PlaybackSpeed); absPlaybackSpeed > 0.0000001f)
		{
			pose->AnimationDuration = animationDuration / absPlaybackSpeed;
		}
		else
		{
			// kind-of doesn't matter what duration we return here.
			// if playback speed is zero, the animation isnt going to progress anyway
			pose->AnimationDuration = animationDuration;
		}

		return 0.0f;
	}


	void AnimationPlayer::SetAnimationTimePos(float time)
	{
		m_PreviousAnimationTimePos = time;
		m_AnimationTimePos = time;
		m_IsFinished = false;
	}


	SampleAnimation::SampleAnimation(const char* dbgName, UUID id)
	: NodeProcessor(dbgName, id)
	, m_TrackWriter(static_cast<Pose*>(out_Pose.getRawData()))
	{
		EndpointUtilities::RegisterEndpoints(this);
	}
	

	void SampleAnimation::Init(const Skeleton*)
	{
		EndpointUtilities::InitializeInputs(this);

		Pose defaultPose;
		HZ_CORE_ASSERT(sizeof(Pose) == out_Pose.getRawDataSize());
		memcpy(out_Pose.getRawData(), &defaultPose, sizeof(Pose));

		Process(0.0f);
	}


	float SampleAnimation::Process(float timestep)
	{
		HZ_PROFILE_FUNC();

		Pose* pose = static_cast<Pose*>(out_Pose.getRawData());

		if (m_PreviousAnimation != *in_Animation)
		{
			if(auto animationAsset = AssetManager::GetAsset<AnimationAsset>(*in_Animation); animationAsset)
			{
				m_Animation = animationAsset->GetAnimation();
			}
			if (m_Animation)
			{
				context.initialize(*static_cast<const acl::compressed_tracks*>(m_Animation->GetData()));
				pose->AnimationDuration = m_Animation->GetDuration();
				pose->NumBones = m_Animation->GetNumTracks();
			}
			m_PreviousAnimation = *in_Animation;
		}

		float timePos = glm::clamp(*in_Ratio, 0.0f, 1.0f);
		if (m_Animation)
		{
			context.seek(timePos * pose->AnimationDuration, acl::sample_rounding_policy::none);
			context.decompress_tracks(m_TrackWriter);
		}
		pose->AnimationTimePos = timePos;
		return 0.0f;
	}

}

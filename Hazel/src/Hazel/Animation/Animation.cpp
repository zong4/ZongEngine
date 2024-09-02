#include "hzpch.h"
#include "Animation.h"

#include "Hazel/Renderer/Mesh.h"

namespace Hazel {

	Animation::Animation(const float duration, const uint32_t numTracks)
	: m_Duration(duration)
	, m_NumTracks(numTracks)
	, m_RootTranslationStart(0.0f, 0.0f, 0.0f)
	, m_RootTranslationEnd(0.0f, 0.0f, 0.0f)
	, m_RootRotationStart(1.0f, 0.0f, 0.0f, 0.0f)
	, m_RootRotationEnd(1.0f, 0.0f, 0.0f, 0.0f)
	{}


	void Animation::SetKeys(std::vector<TranslationKey> translations, std::vector<RotationKey> rotations, std::vector<ScaleKey> scales)
	{
		m_TranslationKeys = std::move(translations);
		m_RotationKeys = std::move(rotations);
		m_ScaleKeys = std::move(scales);
		m_RootTranslationStart = m_TranslationKeys[0].Value;
		m_RootRotationStart = m_RotationKeys[0].Value;
		for (auto key = m_TranslationKeys.rbegin(); key != m_TranslationKeys.rend(); ++key)
		{
			if (key->Track == 0)
			{
				m_RootTranslationEnd = key->Value;
				break;
			}
		}
		for (auto key = m_RotationKeys.rbegin(); key != m_RotationKeys.rend(); ++key)
		{
			if (key->Track == 0)
			{
				m_RootRotationEnd = key->Value;
				break;
			}
		}
	}


	AnimationAsset::AnimationAsset(Ref<MeshSource> animationSource, Ref<MeshSource> skeletonSource, const uint32_t animationIndex, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask)
	: m_RootTranslationMask(rootTranslationMask)
	, m_RootRotationMask(rootRotationMask)
	, m_AnimationSource(animationSource)
	, m_SkeletonSource(skeletonSource)
	, m_AnimationIndex(animationIndex)
	, m_IsMaskedRootMotion(isMaskedRootMotion)
	{
		HZ_CORE_ASSERT(rootRotationMask == 0.0f || rootRotationMask == 1.0f);
		HZ_CORE_ASSERT(rootTranslationMask.x == 0.0f || rootTranslationMask.x == 1.0f);
		HZ_CORE_ASSERT(rootTranslationMask.y == 0.0f || rootTranslationMask.y == 1.0f);
		HZ_CORE_ASSERT(rootTranslationMask.z == 0.0f || rootTranslationMask.z == 1.0f);
	}


	Ref<MeshSource> AnimationAsset::GetAnimationSource() const
	{
		return m_AnimationSource;
	}


	Ref<MeshSource> AnimationAsset::GetSkeletonSource() const
	{
		return m_SkeletonSource;
	}


	uint32_t AnimationAsset::GetAnimationIndex() const
	{
		return m_AnimationIndex;
	}


	bool AnimationAsset::IsMaskedRootMotion() const
	{
		return m_IsMaskedRootMotion;
	}


	const glm::vec3& AnimationAsset::GetRootTranslationMask() const
	{
		return m_RootTranslationMask;
	}


	float AnimationAsset::GetRootRotationMask() const
	{
		return m_RootRotationMask;
	}


	const Hazel::Animation* AnimationAsset::GetAnimation() const
	{
		if (m_AnimationSource && m_SkeletonSource && m_SkeletonSource->HasSkeleton())
		{
			return m_AnimationSource->GetAnimation(m_AnimationIndex, m_SkeletonSource->GetSkeleton(), m_IsMaskedRootMotion, m_RootTranslationMask, m_RootRotationMask);
		}
		return nullptr;
	}

}

#pragma once

#include "Hazel/Asset/Asset.h"
#include "Hazel/Core/Ref.h"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>

namespace Hazel {

	class MeshSource;

	// Animation is a collection of keyed values for translation, rotation, and scale of a number of "tracks"
	// Typically one "track" = one bone of a skeleton.
	// (but later we may also want to animate other things, so a "track" isn't necessarily a bone)
	class Animation
	{
	public:
		Animation() = default;
		Animation(const float duration, const uint32_t numTracks, void* data);

		Animation(const Animation&) = delete; // copying animation is not allowed (because we don't want to copy the data)
		Animation& operator=(const Animation&) = delete;

		Animation(Animation&& other);
		Animation& operator=(Animation&& other);

		~Animation();

		float GetDuration() const { return m_Duration; }
		uint32_t GetNumTracks() const { return m_NumTracks; }

		const void* GetData() const { return m_Data; }

		const glm::vec3& GetRootTranslationStart() const { return m_RootTranslationStart; }
		const glm::vec3& GetRootTranslationEnd() const { return m_RootTranslationEnd; }
		const glm::quat& GetRootRotationStart() const { return m_RootRotationStart; }
		const glm::quat& GetRootRotationEnd() const { return m_RootRotationEnd; }

	public:
		static constexpr uint32_t MAXBONES = 101;  // 100 is max dimension of bone transform array in shaders +1 for artificial root bone at index 0

	private:
		void* m_Data; // compressed track data. type erased so we do not leak acl headers into the engine
		glm::vec3 m_RootTranslationStart;
		glm::vec3 m_RootTranslationEnd;
		glm::quat m_RootRotationStart;
		glm::quat m_RootRotationEnd;
		float m_Duration;
		uint32_t m_NumTracks;
	};


	// Animation asset.
	// Animations ultimately come from a mesh source (so named because it just so happened that meshes were
	// the first thing that "sources" were used for).   A source is an externally authored digital content file)
	// One AnimationAsset == one Animation
	// (so several AnimationAssets might all be referring to the same DCC file (different animations within that file)
	class AnimationAsset : public Asset
	{
	public:
		AnimationAsset(AssetHandle animationSource, AssetHandle skeletonSource, const std::string_view animationName, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, const float rootRotationMask);

		static AssetType GetStaticType() { return AssetType::Animation; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		AssetHandle GetAnimationSource() const;
		AssetHandle GetSkeletonSource() const;
		const std::string& GetAnimationName() const;

		bool IsMaskedRootMotion() const;
		const glm::vec3& GetRootTranslationMask() const;
		float GetRootRotationMask() const;

		// Note: can return nullptr (e.g. if dependent assets (e.g. the skeleton source, or the animation source) are deleted _after_ this AnimationAsset has been created.
		const Animation* GetAnimation() const;

		void OnDependencyUpdated(AssetHandle handle) override;

	private:
		glm::vec3 m_RootTranslationMask;
		float m_RootRotationMask;
		AssetHandle m_AnimationSource; // Animation clips don't necessarily have to come from the same DCC file as the skeleton they are animating.
		AssetHandle m_SkeletonSource;  // For example, the clips could be in one file, and the "skin" (with skeleton) in a separate file.
		std::string m_AnimationName;   // The name of the animation within the DCC file (more robust against changes to the DCC than storing the index)
		bool m_IsMaskedRootMotion;
	};


	struct LocalTransform
	{
		glm::vec3 Translation = glm::zero<glm::vec3>();
		glm::quat Rotation = glm::identity<glm::quat>();
		glm::vec3 Scale = glm::one<glm::vec3>();
	};


	struct Pose
	{
		LocalTransform RootMotion;
		std::array<LocalTransform, Animation::MAXBONES> BoneTransforms;
		float AnimationDuration = 0.0f;
		float AnimationTimePos = 0.0f;
		uint32_t NumBones = 0;
	};

}

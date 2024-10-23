#pragma once

#include "Engine/Asset/Asset.h"
#include "Engine/Core/Ref.h"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>

namespace Hazel {

	class MeshSource;

	// A single keyed (by frame time and track number) value for an animation
	template<typename T>
	struct AnimationKey
	{
		T Value;
		float FrameTime;       // 0.0f = beginning of animation clip, 1.0f = end of animation clip 
		uint32_t Track;

		AnimationKey() = default;
		AnimationKey(const float frameTime, const uint32_t track, const T& value) : FrameTime(frameTime) , Track(track) , Value(value) {}
	};
	using TranslationKey = AnimationKey<glm::vec3>;
	using RotationKey = AnimationKey<glm::quat>;
	using ScaleKey = AnimationKey<glm::vec3>;


	// Animation is a collection of keyed values for translation, rotation, and scale of a number of "tracks"
	// Typically one "track" = one bone of a skeleton.
	// (but later we may also want to animate other things, so a "track" isn't necessarily a bone)
	class Animation
	{
	public:
		Animation() = default;
		Animation(const float duration, const uint32_t numTracks);

		float GetDuration() const { return m_Duration; }
		uint32_t GetNumTracks() const { return m_NumTracks; }

		void SetKeys(std::vector<TranslationKey> translations, std::vector<RotationKey> rotations, std::vector<ScaleKey> scales);

		const auto& GetTranslationKeys() const { return m_TranslationKeys; }
		const auto& GetRotationKeys() const { return m_RotationKeys; }
		const auto& GetScaleKeys() const { return m_ScaleKeys; }

		const glm::vec3& GetRootTranslationStart() const { return m_RootTranslationStart; }
		const glm::vec3& GetRootTranslationEnd() const { return m_RootTranslationEnd; }
		const glm::quat& GetRootRotationStart() const { return m_RootRotationStart; }
		const glm::quat& GetRootRotationEnd() const { return m_RootRotationEnd; }

	public:
		static constexpr uint32_t MAXBONES = 101;  // 100 is max dimension of bone transform array in shaders +1 for artificial root bone at index 0

	private:
		std::vector<TranslationKey> m_TranslationKeys;
		std::vector<RotationKey> m_RotationKeys;
		std::vector<ScaleKey> m_ScaleKeys;
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
		AnimationAsset(Ref<MeshSource> animationSource, Ref<MeshSource> skeletonSource, const uint32_t animationIndex, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask);

		static AssetType GetStaticType() { return AssetType::Animation; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		Ref<MeshSource> GetAnimationSource() const;
		Ref<MeshSource> GetSkeletonSource() const;
		uint32_t GetAnimationIndex() const;

		bool IsMaskedRootMotion() const;
		const glm::vec3& GetRootTranslationMask() const;
		float GetRootRotationMask() const;

		// Note: can return nullptr (e.g. if dependent assets (e.g. the skeleton source, or the animation source) are deleted _after_ this AnimationAsset has been created.
		const Animation* GetAnimation() const;

	private:
		glm::vec3 m_RootTranslationMask;
		float m_RootRotationMask;
		Ref<MeshSource> m_AnimationSource; // Animation clips don't necessarily have to come from the same DCC file as the skeleton they are animating.
		Ref<MeshSource> m_SkeletonSource;  // For example, the clips could be in one file, and the "skin" (with skeleton) in a separate file.
		uint32_t m_AnimationIndex;
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


	// Caches results of sampling animations to take advantage of fact that usually animations play forwards
	// Everything will still work if you play an animation backwards, or skip around in it,
	// its just that in the usual situation (playing forwards) we can optimize some things
	template<typename T>
	class SamplingCache
	{
	public:
		uint32_t GetSize() const { return static_cast<uint32_t>(m_Values.size()) / 2; }

		void Resize(const uint32_t numTracks)
		{
			m_Values.resize(numTracks * 2, T());       // Values vector stores the current and next key for each track, interleaved.  These are the values that we interpolate to sample animation at a given time
			m_FrameTimes.resize(numTracks * 2, 0.0f);  // FrameTimes vector stores the frame time for current and next key.  This is used to figure out the interpolation between values.
		}

		void Reset(const uint32_t numTracks, const std::vector<AnimationKey<T>>& keys)
		{
			Resize(numTracks);
			for (uint32_t i = 0; i < numTracks; ++i)
			{
				m_Values[NextIndex(i)] = keys[i].Value;
				m_FrameTimes[NextIndex(i)] = keys[i].FrameTime;
				HZ_CORE_ASSERT(m_FrameTimes[NextIndex(i)] == 0.0f);
			}
			m_Cursor = 0;
		}

		// step cache forward to given time, using given key frames
		void Step(const float sampleTime, const std::vector<AnimationKey<T>>& keys)
		{
			if ((m_Cursor == static_cast<uint32_t>(keys.size())) || (sampleTime < m_PrevSampleTime))
			{
				Loop();
			}
			auto track = keys[m_Cursor].Track;
			while (m_FrameTimes[NextIndex(track)] <= sampleTime)
			{
				m_Values[CurrentIndex(track)] = m_Values[NextIndex(track)];
				m_Values[NextIndex(track)] = keys[m_Cursor].Value;
				m_FrameTimes[CurrentIndex(track)] = m_FrameTimes[NextIndex(track)];
				m_FrameTimes[NextIndex(track)] = keys[m_Cursor].FrameTime;

				if (++m_Cursor == static_cast<uint32_t>(keys.size()))
				{
					break;
				}
				track = keys[m_Cursor].Track;
			}
			m_PrevSampleTime = sampleTime;
		}

		// loop back to the beginning of animation
		void Loop()
		{
			m_Cursor = 0;
			for (uint32_t track = 0, N = static_cast<uint32_t>(m_Values.size() / 2); track < N; ++track)
			{
				m_FrameTimes[NextIndex(track)] = 0.0;
			}
			m_PrevSampleTime = 0.0f;
		}

		template<int N>
		void Interpolate(const float sampleTime, LocalTransform* localTransform, const std::function<T(const T&, const T&, const float)>& interpolater)
		{
			//static_assert(false, "template parameter N must be 0, 1, or 2");
		}
		
		template<>
		void Interpolate<0>(const float sampleTime, LocalTransform* localTransform, const std::function<T(const T&, const T&, const float)>& interpolater)
		{
			for (uint32_t i = 0, N = static_cast<uint32_t>(m_Values.size()); i < N; i += 2)
			{
				const float t = (sampleTime - m_FrameTimes[i]) / (m_FrameTimes[i + 1] - m_FrameTimes[i]);
				//HZ_CORE_ASSERT(t > -0.0000001f && t < 1.0000001f);
				localTransform[i / 2].Translation = interpolater(m_Values[i], m_Values[i + 1], t);
			}
		}

		template<>
		void Interpolate<1>(const float sampleTime, LocalTransform* localTransform, const std::function<T(const T&, const T&, const float)>& interpolater)
		{
			for (uint32_t i = 0, N = static_cast<uint32_t>(m_Values.size()); i < N; i += 2)
			{
				const float t = (sampleTime - m_FrameTimes[i]) / (m_FrameTimes[i + 1] - m_FrameTimes[i]);
				//HZ_CORE_ASSERT(t > -0.0000001f && t < 1.0000001f);
				localTransform[i / 2].Rotation = interpolater(m_Values[i], m_Values[i + 1], t);
			}
		}

		template<>
		void Interpolate<2>(const float sampleTime, LocalTransform* localTransform, const std::function<T(const T&, const T&, const float)>& interpolater)
		{
			for (uint32_t i = 0, N = static_cast<uint32_t>(m_Values.size()); i < N; i += 2)
			{
				const float t = (sampleTime - m_FrameTimes[i]) / (m_FrameTimes[i + 1] - m_FrameTimes[i]);
				//HZ_CORE_ASSERT(t > -0.0000001f && t < 1.0000001f);
				localTransform[i / 2].Scale = interpolater(m_Values[i], m_Values[i + 1], t);
			}
		}

	public:
		static uint32_t CurrentIndex(const uint32_t i) { return 2 * i; }
		static uint32_t NextIndex(const uint32_t i) { return 2 * i + 1; }

	private:
		std::vector<T> m_Values;
		std::vector<float> m_FrameTimes;

		AssetHandle m_AnimationHandle = 0;
		const Animation* m_Animation = nullptr;
		float m_PrevSampleTime = 0.0f;
		uint32_t m_Cursor = 0;
	};
	using TranslationCache = SamplingCache<glm::vec3>;
	using RotationCache = SamplingCache<glm::quat>;
	using ScaleCache = SamplingCache<glm::vec3>;

}

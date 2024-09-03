#pragma once

#include "Hazel/Animation/Animation.h"
#include "Hazel/Animation/NodeProcessor.h"

#include <glm/glm.hpp>

#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Hazel::AnimationGraph {

	// To create a new type of Animation Node:
	// 1) declare the new node type (derived from NodeProcessor) here.
	// 2) implement the new node type's functions in AnimationNodes.cpp
	// 3) "describe" the new node in NodeDescriptors.h
	// 4) (possibly) init the endpoint funcs for the new node in AnimationNodesImpl.h  (refer to the existing nodes in MathNodes.h/.impl for examples)
	// 5) declare the new node's pin types and default values (for input pins) in Editor/NodeGraphEditor/AnimationGraph/AnimationGraphNodes.cpp
	// 6) register the new node type in the node registry for runtime (Hazel::AnimationGraph::Registry, in Animation/AnimationGraphFactory.cpp)
	// 7) register the new node type in the node registry for editor (Hazel::NodeGraph::Factory<Hazel::AnimationGraph::AnimationGraphNodeFactory>::Registry, in Editor/NodeGraphEditor/AnimationGraph/AnimationGraphNodes.cpp)

	struct AnimationPlayer final : public NodeProcessor
	{
		struct IDs
		{
			DECLARE_ID(Loop);
			DECLARE_ID(Finish);
		private:
			IDs() = delete;
		};

		AnimationPlayer(const char* dbgName, UUID id);

		void Init() override;
		float Process(float timestemp) override;
		void SetAnimationTimePos(float timePos) override;

		// WARNING: changing the names of these variables will break saved graphs
		int64_t* in_Animation = &DefaultAnimation;
		float* in_PlaybackSpeed = &DefaultPlaybackSpeed;
		float* in_Offset = &DefaultOffset;
		bool* in_Loop = &DefaultLoop;

		// Runtime defaults for the above inputs.  Editor defaults are set to the same values (see AnimationGraphNodes.cpp)
		// Individual graphs can override these defaults, in which case the values are saved in this->DefaultValuePlugs
		inline static int64_t DefaultAnimation = 0;
		inline static float DefaultPlaybackSpeed = 1.0f;
		inline static float DefaultOffset = 0.0f;
		inline static bool DefaultLoop = true;

		choc::value::Value out_Pose = choc::value::Value(PoseType);

		OutputEvent out_OnFinish;
		OutputEvent out_OnLoop;

	private:
		TranslationCache m_TranslationCache;
		RotationCache m_RotationCache;
		ScaleCache m_ScaleCache;

		std::vector<glm::vec3> m_LocalTranslations;
		std::vector<glm::quat> m_LocalRotations;
		std::vector<glm::vec3> m_LocalScales;

		glm::vec3 m_RootTranslationStart;
		glm::vec3 m_RootTranslationEnd;
		glm::quat m_RootRotationStart;
		glm::quat m_RootRotationEnd;

		AssetHandle m_PreviousAnimation = 0;
		const Hazel::Animation* m_Animation = nullptr;

		uint32_t m_LoopCount = 0;
		float m_AnimationTimePos = 0.0f;
		float m_PreviousAnimationTimePos = 0.0f;
		float m_PreviousOffset = 0.0f;
	};


	// Blend two animations together
	// Given animation A, animation B and a blend parameter, the animations are blended proprotional to
	// where the blend parameter falls within the range [Range A, Range B]
	struct RangedBlend final : public NodeProcessor
	{
		struct IDs
		{
			DECLARE_ID(LoopA);
			DECLARE_ID(LoopB);
			DECLARE_ID(FinishA);
			DECLARE_ID(FinishB);
		private:
			IDs() = delete;
		};

		RangedBlend(const char* dbgName, UUID id);

		void Init() override;
		float Process(float timestep) override;
		void SetAnimationTimePos(float timePos) override;

		// WARNING: changing the names of these variables will break saved graphs
		int64_t* in_AnimationA = &DefaultAnimation;
		int64_t* in_AnimationB = &DefaultAnimation;
		float* in_PlaybackSpeedA = &DefaultPlaybackSpeed;
		float* in_PlaybackSpeedB = &DefaultPlaybackSpeed;
		bool* in_Synchronize = &DefaultSynchronize;
		float* in_OffsetA = &DefaultOffset;
		float* in_OffsetB = &DefaultOffset;
		bool* in_Loop = &DefaultLoop;
		float* in_RangeA = &DefaultRangeA;
		float* in_RangeB = &DefaultRangeB;
		float* in_Value = &DefaultValue;

		// Runtime defaults for the above inputs.  Editor defaults are set to the same values (see AnimationGraphNodes.cpp)
		// Individual graphs can override these defaults, in which case the values are saved in this->DefaultValuePlugs
		inline static int64_t DefaultAnimation = 0;
		inline static float DefaultPlaybackSpeed = 1.0f;
		inline static bool DefaultSynchronize = true;
		inline static float DefaultOffset = 0.0f;
		inline static bool DefaultLoop = true;
		inline static float DefaultRangeA = 0.0f;
		inline static float DefaultRangeB = 1.0f;
		inline static float DefaultValue = 0.0f;

		choc::value::Value out_Pose = choc::value::Value(PoseType);

		OutputEvent out_OnFinishA;
		OutputEvent out_OnLoopA;
		OutputEvent out_OnFinishB;
		OutputEvent out_OnLoopB;

	private:
		Pose m_PoseA;
		Pose m_PoseB;
		TranslationCache m_TranslationCacheA;
		RotationCache m_RotationCacheA;
		ScaleCache m_ScaleCacheA;
		TranslationCache m_TranslationCacheB;
		RotationCache m_RotationCacheB;
		ScaleCache m_ScaleCacheB;

		std::vector<glm::vec3> m_LocalTranslationsA;
		std::vector<glm::quat> m_LocalRotationsA;
		std::vector<glm::vec3> m_LocalScalesA;
		std::vector<glm::vec3> m_LocalTranslationsB;
		std::vector<glm::quat> m_LocalRotationsB;
		std::vector<glm::vec3> m_LocalScalesB;

		glm::vec3 m_RootTranslationStartA;
		glm::vec3 m_RootTranslationEndA;
		glm::quat m_RootRotationStartA;
		glm::quat m_RootRotationEndA;
		glm::vec3 m_RootTranslationStartB;
		glm::vec3 m_RootTranslationEndB;
		glm::quat m_RootRotationStartB;
		glm::quat m_RootRotationEndB;

		AssetHandle m_PreviousAnimationA = 0;
		const Hazel::Animation* m_AnimationA = nullptr;
		AssetHandle m_PreviousAnimationB = 0;
		const Hazel::Animation* m_AnimationB = nullptr;

		float m_AnimationTimePosA = 0.0f;
		float m_PreviousAnimationTimePosA = 0.0f;
		float m_PreviousOffsetA = 0.0f;

		float m_AnimationTimePosB = 0.0f;
		float m_PreviousAnimationTimePosB = 0.0f;
		float m_PreviousOffsetB = 0.0f;
	};

} // namespace Hazel::AnimationGraph

#undef DECLARE_ID

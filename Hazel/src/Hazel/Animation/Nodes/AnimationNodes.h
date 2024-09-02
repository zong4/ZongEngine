#pragma once
#include "Hazel/Animation/Animation.h"
#include "Hazel/Animation/NodeDescriptor.h"
#include "Hazel/Animation/NodeProcessor.h"
#include "Hazel/Animation/PoseTrackWriter.h"

#include <acl/decompression/decompress.h>
#include <glm/glm.hpp>

#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Hazel::AnimationGraph {

	// Play an animation clip at a given speed (possibly backwards) and optionally loop it
	struct AnimationPlayer : public NodeProcessor
	{
		struct IDs
		{
			DECLARE_ID(Loop);
			DECLARE_ID(Finish);
		private:
			IDs() = delete;
		};

		AnimationPlayer(const char* dbgName, UUID id);

		void Init(const Skeleton*) override;
		float Process(float timestep) override;
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

		OutputEvent out_OnFinish;
		OutputEvent out_OnLoop;
		choc::value::Value out_Pose = choc::value::Value(PoseType);

	private:
		PoseTrackWriter m_TrackWriter;
		acl::decompression_context<acl::default_transform_decompression_settings> m_Context;

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
		bool m_IsFinished = false;
	};


	// Sample an animation clip at a given relative position.
	// 0.0 is beginning of animation, and 1.0 is the end of the animation.
	struct SampleAnimation : public NodeProcessor
	{
		SampleAnimation(const char* dbgName, UUID id);

		void Init(const Skeleton*) override;
		float Process(float timestep) override;

		// WARNING: changing the names of these variables will break saved graphs
		int64_t* in_Animation = &DefaultAnimation;
		float* in_Ratio = &DefaultRatio;

		// Runtime defaults for the above inputs.  Editor defaults are set to the same values (see AnimationGraphNodes.cpp)
		// Individual graphs can override these defaults, in which case the values are saved in this->DefaultValuePlugs
		inline static int64_t DefaultAnimation = 0;
		inline static float DefaultRatio = 0.0f;

		choc::value::Value out_Pose = choc::value::Value(PoseType);

	private:
		PoseTrackWriter m_TrackWriter;
		acl::decompression_context<acl::default_transform_decompression_settings> context;

		AssetHandle m_PreviousAnimation = 0;
		const Hazel::Animation* m_Animation = nullptr;
	};

} // namespace Hazel::AnimationGraph


DESCRIBE_NODE(Hazel::AnimationGraph::AnimationPlayer,
	NODE_INPUTS(
		&Hazel::AnimationGraph::AnimationPlayer::in_Animation,
		&Hazel::AnimationGraph::AnimationPlayer::in_PlaybackSpeed,
		&Hazel::AnimationGraph::AnimationPlayer::in_Offset,
		&Hazel::AnimationGraph::AnimationPlayer::in_Loop),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::AnimationPlayer::out_OnFinish,
		&Hazel::AnimationGraph::AnimationPlayer::out_OnLoop,
		& Hazel::AnimationGraph::AnimationPlayer::out_Pose)
);

DESCRIBE_NODE(Hazel::AnimationGraph::SampleAnimation,
	NODE_INPUTS(
		&Hazel::AnimationGraph::SampleAnimation::in_Animation,
		&Hazel::AnimationGraph::SampleAnimation::in_Ratio),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::SampleAnimation::out_Pose)
);

#undef DECLARE_ID

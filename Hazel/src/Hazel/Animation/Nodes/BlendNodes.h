#pragma once
#include "Hazel/Animation/Animation.h"
#include "Hazel/Animation/AnimationGraph.h"
#include "Hazel/Animation/NodeDescriptor.h"
#include "Hazel/Animation/PoseTrackWriter.h"

#include <acl/decompression/decompress.h>
#include <CDT/CDT.h>
#include <glm/glm.hpp>

#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Hazel::AnimationGraph {

	struct Prototype;

	struct BlendSpaceVertex : public NodeProcessor
	{
		BlendSpaceVertex(const char* dbgName, UUID id);

		void Init(const Skeleton*) override;
		void EnsureAnimation();
		void UpdateTime(float timestep);
		float Process(float timestep) override;
		float GetAnimationDuration() const override;
		float GetAnimationTimePos() const override;
		void SetAnimationTimePos(float timePos) override;

		// WARNING: changing the names of these variables will break saved graphs
		int64_t* in_Animation = &DefaultAnimation;
		bool* in_Synchronize = &DefaultSynchronize;

		// Runtime defaults for the above inputs.  Editor defaults are set to the same values (see AnimationGraphNodes.cpp)
		// Individual graphs can override these defaults, in which case the values are saved in this->DefaultValuePlugs
		inline static int64_t DefaultAnimation = 0;
		inline static bool DefaultSynchronize = true;

		choc::value::Value out_Pose = choc::value::Value(PoseType);

		float X = 0.0f;
		float Y = 0.0f;

	private:
		PoseTrackWriter m_TrackWriter;
		acl::decompression_context<acl::default_transform_decompression_settings> context;

		glm::vec3 m_RootTranslationStart;
		glm::vec3 m_RootTranslationEnd;
		glm::quat m_RootRotationStart;
		glm::quat m_RootRotationEnd;

		AssetHandle m_PreviousAnimation = 0;
		const Hazel::Animation* m_Animation = nullptr;

		float m_AnimationTimePos = 0.0f;
		float m_PreviousAnimationTimePos = 0.0f;
	};


	// Blend multiple animations together according to input coordinates in a 2D plane
	struct BlendSpace : public NodeProcessor
	{
		BlendSpace(const char* dbgName, UUID id);

		void Init(const Skeleton* skeleton) override;
		float Process(float timestep) override;
		float GetAnimationDuration() const override;
		float GetAnimationTimePos() const override;
		void SetAnimationTimePos(float timePos) override;

		// WARNING: changing the names of these variables will break saved graphs
		float* in_X = &DefaultX;
		float* in_Y = &DefaultY;

		// Runtime defaults for the above inputs.  Editor defaults are set to the same values (see AnimationGraphNodes.cpp)
		// Individual graphs can override these defaults, in which case the values are saved in this->DefaultValuePlugs
		inline static float DefaultX = 0.0f;
		inline static float DefaultY = 0.0f;

		choc::value::Value out_Pose = choc::value::Value(PoseType);

	private:
		void LerpPosition(float timestep);

		void Blend1(CDT::IndexSizeType i0, float timestep, CDT::IndexSizeType& previousI0, CDT::IndexSizeType& previousI1, CDT::IndexSizeType& previousI2);
		void Blend2(CDT::IndexSizeType i0, CDT::IndexSizeType i1, float u, float v, float timestep, CDT::IndexSizeType& previousI0, CDT::IndexSizeType& previousI1, CDT::IndexSizeType& previousI2);
		void Blend3(CDT::IndexSizeType i0, CDT::IndexSizeType i1, CDT::IndexSizeType i2, float u, float v, float w, float timestep);

		CDT::Triangulation<float> m_Triangulation;
		std::vector<Scope<BlendSpaceVertex>> m_Vertices;

		CDT::IndexSizeType m_PreviousI0 = -1;
		CDT::IndexSizeType m_PreviousI1 = -1;
		CDT::IndexSizeType m_PreviousI2 = -1;

		float m_LerpSecondsPerUnitX = 0.0f; // 0 => instantly snap to new position
		float m_LerpSecondsPerUnitY = 0.0f;

		// Values from the last frame.
		// We do not need to recalculate if inputs are unchanged
		float m_LastX = -FLT_MAX;
		float m_LastY = -FLT_MAX;

		float m_U = 0.0f;
		float m_V = 0.0f;
		float m_W = 0.0f;

		const Skeleton* m_Skeleton;

		friend bool InitBlendSpace(BlendSpace*, const float, const float, const Prototype&);
	};


	// Blend two poses depending on a boolean condition.
	// Note that this node cannot "synchronize" the blend.
	// If you need synchronization, then use a node that blends animation clips rather than poses.
	// (such as BlendSpace or RangedBlend)
	struct ConditionalBlend : public NodeProcessor
	{
		ConditionalBlend(const char* dbgName, UUID id);

		void Init(const Skeleton*) override;
		float Process(float timestep) override;

		// WARNING: changing the names of these variables will break saved graphs
		Pose* in_BasePose = &DefaultPose;
		bool* in_Condition = &DefaultCondition;
		float* in_Weight = &DefaultWeight;
		bool* in_Additive = &DefaultAdditive;
		int* in_BlendRootBone = &DefaultBone;
		float* in_BlendInDuration = &DefaultBlendInDuration;
		float* in_BlendOutDuration = &DefaultBlendOutDuration;

		// Runtime defaults for the above inputs.  Editor defaults are set to the same values (see AnimationGraphNodes.cpp)
		// Individual graphs can override these defaults, in which case the values are saved in this->DefaultValuePlugs
		inline static Pose DefaultPose = {};
		inline static bool DefaultCondition = false;
		inline static float DefaultWeight = 1.0f;
		inline static bool DefaultAdditive = false;
		inline static int DefaultBone = 0;
		inline static float DefaultBlendInDuration = 0.1f;
		inline static float DefaultBlendOutDuration = 0.1f;

		choc::value::Value out_Pose = choc::value::Value(PoseType);

	private:
		Ref<AnimationGraph> m_AnimationGraph = nullptr;
		const Skeleton* m_Skeleton = nullptr;
		float m_TransitionWeight = 0.0f;
		bool m_LastCondition = false;

		friend bool InitConditionalBlend(Ref<AnimationGraph>, ConditionalBlend*, const Prototype&);
	};


	// Blend a secondary pose into a base pose together depending on a trigger.
	// The secondary is automatically blended out when it has played through once.
	// Note that this node is cannot "synchronize" the blend.
	// If you need synchronization, then use a node that blends animation clips rather than poses.
	// (such as BlendSpace or RangedBlend)
	struct OneShot : public NodeProcessor
	{
		struct IDs
		{
			DECLARE_ID(Trigger);
			DECLARE_ID(Finished);
		private:
			IDs() = delete;
		};

		OneShot(const char* dbgName, UUID id);

		void Init(const Skeleton*) override;
		float Process(float timestep) override;

		// WARNING: changing the names of these variables will break saved graphs
		Pose* in_BasePose = &DefaultPose;
		float* in_Weight = &DefaultWeight;
		bool* in_Additive = &DefaultAdditive;
		int* in_BlendRootBone = &DefaultBone;
		float* in_BlendInDuration = &DefaultBlendInDuration;
		float* in_BlendOutDuration = &DefaultBlendOutDuration;

		OutputEvent out_OnFinished;

		void Trigger(Identifier)
		{
			m_Trigger.SetDirty();
		}

		// Runtime defaults for the above inputs.  Editor defaults are set to the same values (see AnimationGraphNodes.cpp)
		// Individual graphs can override these defaults, in which case the values are saved in this->DefaultValuePlugs
		inline static Pose DefaultPose = {};
		inline static float DefaultWeight = 1.0f;
		inline static bool DefaultAdditive = false;
		inline static int DefaultBone = 0;
		inline static float DefaultBlendInDuration = 0.1f;
		inline static float DefaultBlendOutDuration = 0.1f;

		choc::value::Value out_Pose = choc::value::Value(PoseType);

	private:
		Ref<AnimationGraph> m_AnimationGraph = nullptr;
		const Skeleton* m_Skeleton = nullptr;
		float m_TriggeredDuration = 0.0f;
		Flag m_Trigger;
		bool m_Triggered = false;

		friend bool InitOneShot(Ref<AnimationGraph>, OneShot*, const Prototype&);
	};


	// Blend two poses together.
	// Given pose A, pose B and a blend parameter alpha, blend the two poses together.
	//
	// If additive is false it's a lerp:
	//    Result = (1 - alpha) * PoseA + alpha * PoseB
	//
	// If additive is true it's an additive blend:
	//    Result = PoseA + alpha * (PoseB - Rest Pose)
	//
	// The blend starts from the specified bone.
	// (Bones earlier in the hierarchy are just set to same as PoseA)
	struct PoseBlend : public NodeProcessor
	{
		PoseBlend(const char* dbgName, UUID id);

		void Init(const Skeleton*) override;
		float Process(float timestep) override;

		// WARNING: changing the names of these variables will break saved graphs
		Pose* in_PoseA = &DefaultPose;
		Pose* in_PoseB = &DefaultPose;
		float* in_Alpha = &DefaultAlpha;
		bool* in_Additive = &DefaultAdditive;
		int* in_Bone = &DefaultBone;

		// Runtime defaults for the above inputs.  Editor defaults are set to the same values (see AnimationGraphNodes.cpp)
		// Individual graphs can override these defaults, in which case the values are saved in this->DefaultValuePlugs
		inline static Pose DefaultPose;
		inline static float DefaultAlpha = 0.0f;
		inline static bool DefaultAdditive = false;
		inline static int DefaultBone = 0;

		choc::value::Value out_Pose = choc::value::Value(PoseType);

	private:
		const Skeleton* m_Skeleton = nullptr;
	};


	// Blend two animations together
	// Given animation A, animation B and a blend parameter, the animations are blended proportional to
	// where the blend parameter falls within the range [Range A, Range B]
	//
	// The blend is optionally synchronized so that the same relative time position is used
	// when sampling from each clip.
	struct RangedBlend : public NodeProcessor
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

		void Init(const Skeleton*) override;
		float Process(float timestep) override;
		void SetAnimationTimePos(float timePos) override;

		// WARNING: changing the names of these variables will break saved graphs
		int64_t* in_AnimationA = &DefaultAnimation;
		int64_t* in_AnimationB = &DefaultAnimation;
		float* in_PlaybackSpeedA = &DefaultPlaybackSpeed;
		float* in_OffsetA = &DefaultOffset;
		bool* in_LoopA = &DefaultLoop;
		float* in_PlaybackSpeedB = &DefaultPlaybackSpeed;
		float* in_OffsetB = &DefaultOffset;
		bool* in_LoopB = &DefaultLoop;
		bool* in_Synchronize = &DefaultSynchronize;
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

		OutputEvent out_OnFinishA;
		OutputEvent out_OnLoopA;
		OutputEvent out_OnFinishB;
		OutputEvent out_OnLoopB;
		choc::value::Value out_Pose = choc::value::Value(PoseType);

	private:
		Pose m_PoseA;
		Pose m_PoseB;

		PoseTrackWriter m_TrackWriterA;
		acl::decompression_context<acl::default_transform_decompression_settings> contextA;

		PoseTrackWriter m_TrackWriterB;
		acl::decompression_context<acl::default_transform_decompression_settings> contextB;

		glm::vec3 m_RootTranslationStartA;
		glm::vec3 m_RootTranslationEndA;
		glm::quat m_RootRotationStartA;
		glm::quat m_RootRotationEndA;
		glm::vec3 m_RootTranslationStartB;
		glm::vec3 m_RootTranslationEndB;
		glm::quat m_RootRotationStartB;
		glm::quat m_RootRotationEndB;

		AssetHandle m_PreviousAnimationA = 0;
		const Animation* m_AnimationA = nullptr;
		AssetHandle m_PreviousAnimationB = 0;
		const Animation* m_AnimationB = nullptr;

		const Skeleton* m_Skeleton = nullptr;

		float m_AnimationTimePosA = 0.0f;
		float m_PreviousAnimationTimePosA = 0.0f;
		float m_PreviousOffsetA = 0.0f;

		float m_AnimationTimePosB = 0.0f;
		float m_PreviousAnimationTimePosB = 0.0f;
		float m_PreviousOffsetB = 0.0f;

		bool m_IsFinishedA = false;
		bool m_IsFinishedB = false;
	};

} // namespace Hazel::AnimationGraph


DESCRIBE_NODE(Hazel::AnimationGraph::BlendSpace,
	NODE_INPUTS(
		&Hazel::AnimationGraph::BlendSpace::in_X,
		&Hazel::AnimationGraph::BlendSpace::in_Y),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::BlendSpace::out_Pose)
);


DESCRIBE_NODE(Hazel::AnimationGraph::BlendSpaceVertex,
	NODE_INPUTS(
		&Hazel::AnimationGraph::BlendSpaceVertex::in_Animation,
		&Hazel::AnimationGraph::BlendSpaceVertex::in_Synchronize),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::BlendSpaceVertex::out_Pose)
);


DESCRIBE_NODE(Hazel::AnimationGraph::ConditionalBlend,
	NODE_INPUTS(
		&Hazel::AnimationGraph::ConditionalBlend::in_BasePose,
		&Hazel::AnimationGraph::ConditionalBlend::in_Condition,
		&Hazel::AnimationGraph::ConditionalBlend::in_Weight,
		&Hazel::AnimationGraph::ConditionalBlend::in_Additive,
		&Hazel::AnimationGraph::ConditionalBlend::in_BlendRootBone,
		&Hazel::AnimationGraph::ConditionalBlend::in_BlendInDuration,
		&Hazel::AnimationGraph::ConditionalBlend::in_BlendOutDuration),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::ConditionalBlend::out_Pose)
);


DESCRIBE_NODE(Hazel::AnimationGraph::OneShot,
	NODE_INPUTS(
		&Hazel::AnimationGraph::OneShot::in_BasePose,
		&Hazel::AnimationGraph::OneShot::Trigger,
		&Hazel::AnimationGraph::OneShot::in_Weight,
		&Hazel::AnimationGraph::OneShot::in_Additive,
		&Hazel::AnimationGraph::OneShot::in_BlendRootBone,
		&Hazel::AnimationGraph::OneShot::in_BlendInDuration,
		&Hazel::AnimationGraph::OneShot::in_BlendOutDuration),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::OneShot::out_OnFinished,
		&Hazel::AnimationGraph::OneShot::out_Pose)

);


DESCRIBE_NODE(Hazel::AnimationGraph::PoseBlend,
	NODE_INPUTS(
		&Hazel::AnimationGraph::PoseBlend::in_PoseA,
		&Hazel::AnimationGraph::PoseBlend::in_PoseB,
		&Hazel::AnimationGraph::PoseBlend::in_Alpha,
		&Hazel::AnimationGraph::PoseBlend::in_Additive,
		&Hazel::AnimationGraph::PoseBlend::in_Bone),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::PoseBlend::out_Pose)
);


DESCRIBE_NODE(Hazel::AnimationGraph::RangedBlend,
	NODE_INPUTS(
		&Hazel::AnimationGraph::RangedBlend::in_AnimationA,
		&Hazel::AnimationGraph::RangedBlend::in_AnimationB,
		&Hazel::AnimationGraph::RangedBlend::in_PlaybackSpeedA,
		&Hazel::AnimationGraph::RangedBlend::in_OffsetA,
		&Hazel::AnimationGraph::RangedBlend::in_LoopA,
		&Hazel::AnimationGraph::RangedBlend::in_PlaybackSpeedB,
		&Hazel::AnimationGraph::RangedBlend::in_OffsetB,
		&Hazel::AnimationGraph::RangedBlend::in_LoopB,
		&Hazel::AnimationGraph::RangedBlend::in_Synchronize,
		&Hazel::AnimationGraph::RangedBlend::in_RangeA,
		&Hazel::AnimationGraph::RangedBlend::in_RangeB,
		&Hazel::AnimationGraph::RangedBlend::in_Value),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::RangedBlend::out_OnFinishA,
		&Hazel::AnimationGraph::RangedBlend::out_OnLoopA,
		&Hazel::AnimationGraph::RangedBlend::out_OnFinishB,
		&Hazel::AnimationGraph::RangedBlend::out_OnLoopB,
		& Hazel::AnimationGraph::RangedBlend::out_Pose)
	);

#undef DECLARE_ID

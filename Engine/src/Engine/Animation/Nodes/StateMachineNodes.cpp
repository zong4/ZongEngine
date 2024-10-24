#include "pch.h"
#include "NodeDescriptors.h"

#include "Engine/Animation/Animation.h"
#include "Engine/Asset/AssetManager.h"
#include "Engine/Debug/Profiler.h"

#include <glm/common.hpp>
#include <glm/ext/quaternion_common.hpp>

namespace Engine::AnimationGraph {

	TransitionNode::TransitionNode(std::string_view dbgName, UUID id) : AnimationNodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}


	void TransitionNode::Init()
	{
		ZONG_CORE_ASSERT(m_ConditionGraph, "No condition graph set");
		EndpointUtilities::InitializeInputs(this);
		m_ConditionGraph->Init();
	}


	float TransitionNode::Process(float timestep)
	{
		ZONG_PROFILE_FUNC();
		ZONG_CORE_ASSERT(m_SourceState, "No source state set");
		ZONG_CORE_ASSERT(m_DestinationState, "No target state set");

		// if update goes beyond end of transition, then we need to change the current node
		// to the target node.
		// We also need to set the update value to the remainder of the update value.
		// (e.g. if the transition is 0.5 seconds long, and the update value is 0.7, then
		// we need to set the update value to 0.2)
		m_TransitionTime += timestep;
		if (m_TransitionTime > m_Duration)
		{
			m_Parent->SetCurrentState(m_DestinationState);
			timestep -= m_TransitionTime - m_Duration;
			return timestep;
		}

		float blendWeight = m_TransitionTime / m_Duration;
		if (m_IsSynchronized)
		{
			float blendedDuration = ((1.0f - blendWeight) * m_SourceState->GetAnimationDuration()) + (blendWeight * m_DestinationState->GetAnimationDuration());
			m_SourceState->ProcessGraph(timestep * m_SourceState->GetAnimationDuration() / blendedDuration);
			m_DestinationState->ProcessGraph(timestep * m_DestinationState->GetAnimationDuration() / blendedDuration);
		}
		else
		{
			m_SourceState->ProcessGraph(timestep);
			m_DestinationState->ProcessGraph(timestep);
		}

		Blend(m_SourceState->GetPose(), m_DestinationState->GetPose(), blendWeight);

		return 0.0f;
	}


	float TransitionNode::GetAnimationDuration() const
	{
		return m_Duration;
	}


	float TransitionNode::GetAnimationTimePos() const
	{
		return m_Duration == 0.0f? 0.0f : m_TransitionTime / m_Duration;
	}


	void TransitionNode::SetAnimationTimePos(float timePos)
	{
		m_TransitionTime = timePos * m_Duration;
	}


	void TransitionNode::Blend(const choc::value::ValueView source, const choc::value::ValueView destination, float w)
	{
		const Pose* poseA = reinterpret_cast<const Pose*>(source.getRawData());
		const Pose* poseB = reinterpret_cast<const Pose*>(destination.getRawData());
		ZONG_CORE_ASSERT(poseA->NumBones == poseB->NumBones, "Poses have different number of bones");

		Pose* result = reinterpret_cast<Pose*>(out_Pose.getRawData());
		for (uint32_t i = 0, N = glm::min(poseA->NumBones, poseB->NumBones); i < N; i++)
		{
			result->BoneTransforms[i].Translation = glm::mix(poseA->BoneTransforms[i].Translation, poseB->BoneTransforms[i].Translation, w);
			result->BoneTransforms[i].Rotation = glm::slerp(poseA->BoneTransforms[i].Rotation, poseB->BoneTransforms[i].Rotation, w);
			result->BoneTransforms[i].Scale = glm::mix(poseA->BoneTransforms[i].Scale, poseB->BoneTransforms[i].Scale, w);
		}
		result->RootMotion.Translation = glm::mix(poseA->RootMotion.Translation, poseB->RootMotion.Translation, w);
		result->RootMotion.Rotation = glm::slerp(poseA->RootMotion.Rotation, poseB->RootMotion.Rotation, w);
		result->RootMotion.Scale = glm::mix(poseA->RootMotion.Scale, poseB->RootMotion.Scale, w);
		result->AnimationDuration = glm::mix(poseA->AnimationDuration, poseB->AnimationDuration, w);
		result->AnimationTimePos = glm::mix(poseA->AnimationTimePos, poseB->AnimationTimePos, w);
		result->NumBones = poseA->NumBones;
	}


	bool TransitionNode::ShouldTransition()
	{
		ZONG_PROFILE_FUNC();
		ZONG_CORE_ASSERT(m_ConditionGraph, "No condition graph set");

		m_ConditionGraph->Process(0.0f);

		// TODO (0x): We should be able to do this...  (but cannot because of the way graph outputs are implemented.
		//            Graph OutValue ends up pointing to something different to EndpointOutputStreams.
		//            Fix this (probably by removing the EndpointOutputStreams indirection)
		//return m_ConditionGraph->OutValue(Identifier("Transition")).getBool();

		return m_ConditionGraph->EndpointOutputStreams.InValue(IDs::Transition).getBool();
	}


	void TransitionNode::BeginTransition()
	{
		m_TransitionTime = 0.0f;
		m_Duration = m_ConditionGraph->EndpointOutputStreams.InValue(IDs::Duration).getFloat32();
		m_IsSynchronized = m_ConditionGraph->EndpointOutputStreams.InValue(IDs::Synchronize).getBool();

		if (m_IsSynchronized)
		{
			// ensure destination state pose is initialised, otherwise later initialisation will overwrite SetAnimationTimePos
			m_DestinationState->ProcessGraph(0.0f);
			m_DestinationState->SetAnimationTimePos(m_SourceState->GetAnimationTimePos());
		}
		else
		{
			m_DestinationState->SetAnimationTimePos(0.0f);
		}

	}


	StateMachine::StateMachine(std::string_view dbgName, UUID id) : StateBase(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}


	void StateMachine::Init()
	{
		EndpointUtilities::InitializeInputs(this);

		m_CurrentState = m_States.front().get();
		for (auto& transition : m_Transitions)
		{
			transition->Init();
		}

		for (auto& state : m_States)
		{
			state->Init();
		}
	}


	float StateMachine::Process(float timestep)
	{
		ZONG_PROFILE_FUNC();

		// Check for transition out of this state machine
		for (const auto& transition : m_Transitions)
		{
			if (transition->ShouldTransition())
			{
				transition->BeginTransition();
				m_Parent->SetCurrentState(transition.get());
				return timestep;
			}
		}

		return ProcessGraph(timestep);
	}


	float StateMachine::ProcessGraph(float timestep)
	{
		while (timestep > 0.0f)
		{
			ZONG_CORE_ASSERT(m_CurrentState, "No current state set");

			timestep = m_CurrentState->Process(timestep);
		}

		// TODO: Can copying pose here be avoided?
		out_Pose = m_CurrentState->GetPose();
		return timestep;
	}


	float StateMachine::GetAnimationDuration() const
	{
		return m_CurrentState->GetAnimationDuration();
	}


	float StateMachine::GetAnimationTimePos() const
	{
		return m_CurrentState->GetAnimationTimePos();
	}


	void StateMachine::SetAnimationTimePos(float timePos)
	{
		m_CurrentState->SetAnimationTimePos(timePos);
	}


	void StateMachine::SetCurrentState(AnimationNodeProcessor* node)
	{
		m_CurrentState = node;
	}


	State::State(std::string_view dbgName, UUID id) : StateBase(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}


	void State::Init()
	{
		ZONG_CORE_ASSERT(m_AnimationGraph, "No animation graph set");
		EndpointUtilities::InitializeInputs(this);
		for(auto& transition : m_Transitions)
		{
			transition->Init();
		}
		m_AnimationGraph->Init();
	}


	float State::Process(float timestep)
	{
		ZONG_PROFILE_FUNC();
		ZONG_CORE_ASSERT(m_AnimationGraph, "No animation graph set");

		// Check for transition out of this state
		for (const auto& transition : m_Transitions)
		{
			if (transition->ShouldTransition())
			{
				transition->BeginTransition();
				m_Parent->SetCurrentState(transition.get());
				return timestep;
			}
		}

		return ProcessGraph(timestep);
	}


	float State::ProcessGraph(float timestep)
	{
		// Evaluate pose for this state
		timestep = m_AnimationGraph->Process(timestep);

		// TODO: Can copying pose here be avoided?
		out_Pose = m_AnimationGraph->EndpointOutputStreams.InValue(IDs::Pose);
		return timestep;
	}


	float State::GetAnimationDuration() const
	{
		return m_AnimationGraph->GetAnimationDuration();
	}


	float State::GetAnimationTimePos() const
	{
		return m_AnimationGraph->GetAnimationTimePos();
	}


	void State::SetAnimationTimePos(float timePos)
	{
		m_AnimationGraph->SetAnimationTimePos(timePos);
	}


	QuickState::QuickState(std::string_view dbgName, UUID id) : StateBase(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}


	float QuickState::Process(float timestep)
	{
		ZONG_PROFILE_FUNC();

		// Check for transition out of this state
		for (const auto& transition : m_Transitions)
		{
			if (transition->ShouldTransition())
			{
				transition->BeginTransition();
				m_Parent->SetCurrentState(transition.get());
				return timestep;
			}
		}

		return ProcessGraph(timestep);
	}


	float QuickState::ProcessGraph(float timestep)
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
			m_AnimationTimePos = 0.0f;
			m_PreviousAnimationTimePos = m_AnimationTimePos;
			m_PreviousAnimation = *in_Animation;
		}

		if (m_Animation)
		{
			glm::vec3 previousRootTranslation = pose->BoneTransforms[0].Translation;
			glm::quat previousRootRotation = pose->BoneTransforms[0].Rotation;

			// m_UpdateValue is in seconds, and needs to be normalized to the animation duration
			m_AnimationTimePos += timestep / m_Animation->GetDuration();
			m_AnimationTimePos -= floorf(m_AnimationTimePos);

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

			m_PreviousAnimationTimePos = m_AnimationTimePos;
		}
		pose->AnimationTimePos = m_AnimationTimePos;
		return 0.0f;
	}


	float QuickState::GetAnimationDuration() const
	{
		return m_Animation ? m_Animation->GetDuration() : 0.0f;
	}


	float QuickState::GetAnimationTimePos() const
	{
		return m_AnimationTimePos;
	}


	void QuickState::SetAnimationTimePos(float time)
	{
		m_PreviousAnimationTimePos = time;
		m_AnimationTimePos = time;
	}


	void QuickState::Init()
	{
		EndpointUtilities::InitializeInputs(this);
		m_PreviousAnimation = 0;
		m_AnimationTimePos = 0.0f;
		m_PreviousAnimationTimePos = m_AnimationTimePos;

		Pose defaultPose;
		ZONG_CORE_ASSERT(sizeof(Pose) == out_Pose.getRawDataSize());
		memcpy(out_Pose.getRawData(), &defaultPose, sizeof(Pose));

		for (auto& transition : m_Transitions)
		{
			transition->Init();
		}
	}

}

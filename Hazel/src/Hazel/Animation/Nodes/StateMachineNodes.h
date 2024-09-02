#pragma once

#include "Hazel/Animation/Animation.h"
#include "Hazel/Animation/AnimationGraph.h"
#include "Hazel/Animation/NodeDescriptor.h"
#include "Hazel/Animation/PoseTrackWriter.h"

#include <acl/decompression/decompress.h>
#include <glm/glm.hpp>

#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Hazel::AnimationGraph {

	struct Prototype;

	// Sometimes we need to reuse NodeProcessor types for different nodes,
	// so that they have different names and other editor UI properties
	// but the same NodeProcessor type in the backend.
	// In such case we declare alias ID here and define it in the AnimationGraphNodes.cpp,
	// adding and extra entry to the registry of the same NodeProcessor type,
	// but with a different name.
	// Actual NodeProcessor type for the alias is assigned in AnimationGraphFactory.cpp
	//! Aliases must already be "User Friendly Type Name" in format: "Get Random (Float)" instead of "GetRandom<float>"
	namespace Alias {
		static constexpr auto Transition = "Transition";
	}


	struct StateBase;
	struct StateMachine;

	// A node processor that produces an animation pose, and can be nested inside a state machine.
	// This is common functionality that is shared between StateMachine, State, and Transition nodes.
	struct StateMachineNodeProcessor : public NodeProcessor
	{
		using NodeProcessor::NodeProcessor;

		choc::value::Value out_Pose = choc::value::Value(PoseType);

		choc::value::ValueView GetPose() const { return out_Pose; }

#if 0
		bool SendInputEvent(Identifier endpointID, float value)
		{
			auto endpoint = InGraphEvs.find(endpointID);

			if (endpoint == InGraphEvs.end())
				return false;

			endpoint->second(value);
			return true;
		}
#endif

	protected:
#if 0	
		std::unordered_map<Identifier, OutputEvent&> InGraphEvs = { {IDs::Update, in_Update} };
#endif
		StateMachine* m_Parent = nullptr;
	};


	struct TransitionNode : public StateMachineNodeProcessor
	{
		struct IDs
		{
			DECLARE_ID(Duration);
			DECLARE_ID(Synchronize);
			DECLARE_ID(Transition);
		private:
			IDs() = delete;
		};

		TransitionNode(std::string_view dbgName, UUID id);

		void Init(const Skeleton*) override;
		float Process(float timestep) override;

		float GetAnimationDuration() const override;
		float GetAnimationTimePos() const override;
		void SetAnimationTimePos(float time) override;

		bool ShouldTransition();
		void BeginTransition();

	private:
		void Blend(const choc::value::ValueView source, const choc::value::ValueView destination, float w);

	private:
		StateBase* m_SourceState = nullptr;
		StateBase* m_DestinationState = nullptr;
		Ref<Graph> m_ConditionGraph = nullptr;

		float m_Duration = 0.0f;
		float m_TransitionTime = 0.0f;
		bool m_IsSynchronized = false;
		friend Scope<TransitionNode> CreateTransitionInstance(Ref<AnimationGraph>, UUID, const Prototype&, StateBase*, StateBase*);
		friend bool InitStateMachine(Ref<AnimationGraph>, StateMachine*, const Prototype&);
	};


	struct StateBase : public StateMachineNodeProcessor
	{
		using StateMachineNodeProcessor::StateMachineNodeProcessor;

		virtual float ProcessGraph(float timestep) = 0;

	protected:
		// transitions out of this state
		std::vector<Scope<TransitionNode>> m_Transitions;

		friend bool InitStateMachine(Ref<AnimationGraph> root, StateMachine* stateMachine, const Prototype& prototype);
		friend bool InitInstance(Ref<Graph>, Ref<AnimationGraph>, const Prototype&);
	};


	struct StateMachine : public StateBase
	{
		StateMachine(std::string_view dbgName, UUID id);

		void Init(const Skeleton*) override;
		float Process(float timestep) override;
		float ProcessGraph(float timestep) override;

		float GetAnimationDuration() const override;
		float GetAnimationTimePos() const override;
		void SetAnimationTimePos(float time) override;

		void SetCurrentState(StateMachineNodeProcessor* node);

	private:
		// this state machine's states
		std::vector<Scope<StateBase>> m_States;

		// the currently active state or transition
		StateMachineNodeProcessor* m_CurrentState = nullptr;

		friend bool InitStateMachine(Ref<AnimationGraph>, StateMachine*, const Prototype&);
	};


	struct State : public StateBase
	{
		State(std::string_view dbgName, UUID id);

		void Init(const Skeleton*) override;
		float Process(float timestep) override;
		float ProcessGraph(float timestep) override;

		float GetAnimationDuration() const override;
		float GetAnimationTimePos() const override;
		void SetAnimationTimePos(float time) override;

	private:
		// this state's animation graph
		Ref<AnimationGraph> m_AnimationGraph = nullptr;

		friend Scope<State> CreateStateInstance(Ref<AnimationGraph>, UUID id, const Prototype&);
		friend bool InitInstance(Ref<Graph>, Ref<AnimationGraph>, const Prototype&);
	};


	struct QuickState : public StateBase
	{
		QuickState(std::string_view dbgName, UUID id);

		void Init(const Skeleton*) override;
		float Process(float timestep) override;
		float ProcessGraph(float timestep) override;

		float GetAnimationDuration() const override;
		float GetAnimationTimePos() const override;
		void SetAnimationTimePos(float time) override;

		// WARNING: changing the names of this variable will break saved graphs
		int64_t* in_Animation = &DefaultAnimation;

		inline static int64_t DefaultAnimation = 0;

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

} // namespace Hazel::AnimationGraph


DESCRIBE_NODE(Hazel::AnimationGraph::StateMachine,
	NODE_INPUTS(),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::StateMachine::out_Pose)
);

DESCRIBE_NODE(Hazel::AnimationGraph::State,
	NODE_INPUTS(),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::State::out_Pose)
);

DESCRIBE_NODE(Hazel::AnimationGraph::QuickState,
	NODE_INPUTS(
		&Hazel::AnimationGraph::QuickState::in_Animation),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::QuickState::out_Pose)
);

DESCRIBE_NODE(Hazel::AnimationGraph::TransitionNode,
	NODE_INPUTS(),
	NODE_OUTPUTS()
);


#undef DECLARE_ID

#pragma once

#include "Hazel/Animation/Animation.h"
#include "Hazel/Animation/AnimationGraph.h"
#include "Hazel/Animation/AnimationGraphPrototype.h"
#include "Hazel/Animation/AnimationNodeProcessor.h"

#include <glm/glm.hpp>

#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Hazel::AnimationGraph {

	struct StateBase;

	struct TransitionNode final : public AnimationNodeProcessor 
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

		void Init() override;
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


	struct StateBase : public AnimationNodeProcessor
	{
		using AnimationNodeProcessor::AnimationNodeProcessor;

		virtual float ProcessGraph(float timestep) = 0;

	protected:
		// transitions out of this state
		std::vector<Scope<TransitionNode>> m_Transitions;

		friend bool InitStateMachine(Ref<AnimationGraph> root, StateMachine* stateMachine, const Prototype& prototype);
		friend bool InitInstance(Ref<Graph>, Ref<AnimationGraph>, const Prototype&);
	};


	struct StateMachine final : public StateBase
	{
		StateMachine(std::string_view dbgName, UUID id);

		void Init() final;
		float Process(float timestep) override;
		float ProcessGraph(float timestep) override;

		float GetAnimationDuration() const override;
		float GetAnimationTimePos() const override;
		void SetAnimationTimePos(float time) override;

		void SetCurrentState(AnimationNodeProcessor* node);

	private:
		// this state machine's states
		std::vector<Scope<StateBase>> m_States;

		// the currently active state or transition
		AnimationNodeProcessor* m_CurrentState = nullptr;

		friend bool InitStateMachine(Ref<AnimationGraph>, StateMachine*, const Prototype&);

	};


	struct State final : public StateBase
	{
		State(std::string_view dbgName, UUID id);

		void Init() override;
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


	struct QuickState final : public StateBase
	{
		QuickState(std::string_view dbgName, UUID id);

		void Init() override;
		float Process(float timestep) override;
		float ProcessGraph(float timestep) override;

		float GetAnimationDuration() const override;
		float GetAnimationTimePos() const override;
		void SetAnimationTimePos(float time) override;

		// WARNING: changing the names of this variable will break saved graphs
		int64_t* in_Animation = &DefaultAnimation;

		inline static int64_t DefaultAnimation = 0;

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

		float m_AnimationTimePos = 0.0f;
		float m_PreviousAnimationTimePos = 0.0f;
	};

} // namespace Hazel::AnimationGraph

#undef DECLARE_ID

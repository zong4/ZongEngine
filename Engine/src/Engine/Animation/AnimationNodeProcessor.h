#pragma once
#include "Animation.h"
#include "NodeProcessor.h"


#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Hazel::AnimationGraph {

	struct StateMachine;

	// A node processor that produces an animation pose, and can be nested inside a state machine.
	// This is common functionality that is shared between StateMachine, State, and Transition nodes.
	struct AnimationNodeProcessor : public NodeProcessor
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

} // namespace Hazel::AnimationGraph

#undef DECLARE_ID

#include "pch.h"
#include "TriggerNodes.h"

#include "NodeDescriptors.h"

namespace Hazel::AnimationGraph {

	BoolTrigger::BoolTrigger(const char* dbgName, UUID id)
		: NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}


	void BoolTrigger::Init()
	{
		EndpointUtilities::InitializeInputs(this);
	}


	float BoolTrigger::Process(float ts)
	{
		if (*in_Value)
			out_OnTrue(IDs::True);
		else
			out_OnFalse(IDs::False);

		return ts;
	}

} // namespace Hazel::AnimationGraph

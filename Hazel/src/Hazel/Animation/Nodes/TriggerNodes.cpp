#include "hzpch.h"
#include "TriggerNodes.h"

namespace Hazel::AnimationGraph {

	BoolTrigger::BoolTrigger(const char* dbgName, UUID id)
		: NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}


	void BoolTrigger::Init(const Skeleton*)
	{
		EndpointUtilities::InitializeInputs(this);
	}


	float BoolTrigger::Process(float ts)
	{
		if ((*in_Value && *in_TriggerIfTrue) || (!*in_Value && *in_TriggerIfFalse))
		{
			out_OnTrigger(IDs::Trigger);
		}
		return ts;
	}

} // namespace Hazel::AnimationGraph

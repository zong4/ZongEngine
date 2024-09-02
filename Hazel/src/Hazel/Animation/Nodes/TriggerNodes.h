#pragma once
#include "Hazel/Animation/NodeDescriptor.h"
#include "Hazel/Animation/NodeProcessor.h"

#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Hazel::AnimationGraph {

	struct BoolTrigger : public NodeProcessor
	{
		struct IDs
		{
			DECLARE_ID(Trigger);
		private:
			IDs() = delete;
		};

		explicit BoolTrigger(const char* dbgName, UUID id);

		bool* in_Value = &DefaultValue;
		bool* in_TriggerIfTrue = &DefaultTriggerIfTrue;
		bool* in_TriggerIfFalse = &DefaultTriggerIfFalse;

		// Runtime default for the above inputs.  Editor defaults are set to the same values (see AnimationGraphNodes.cpp)
		// Individual graphs can override these defaults, in which case the values are saved in this->DefaultValuePlugs
		inline static bool DefaultValue = false;
		inline static bool DefaultTriggerIfTrue = true;
		inline static bool DefaultTriggerIfFalse = false;

		OutputEvent out_OnTrigger;

	public:
		void Init(const Skeleton*) override;
		float Process(float) override;
	};

} // namespace Hazel::AnimationGraph


DESCRIBE_NODE(Hazel::AnimationGraph::BoolTrigger,
	NODE_INPUTS(
		&Hazel::AnimationGraph::BoolTrigger::in_Value,
		&Hazel::AnimationGraph::BoolTrigger::in_TriggerIfTrue,
		& Hazel::AnimationGraph::BoolTrigger::in_TriggerIfFalse),
	NODE_OUTPUTS(
		&Hazel::AnimationGraph::BoolTrigger::out_OnTrigger)
);


#undef DECLARE_ID

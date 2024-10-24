#pragma once

#include "Engine/Animation/NodeProcessor.h"

#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Engine::AnimationGraph {

	struct BoolTrigger : public NodeProcessor
	{
		struct IDs
		{
			DECLARE_ID(False);
			DECLARE_ID(True);
		private:
			IDs() = delete;
		};

		explicit BoolTrigger(const char* dbgName, UUID id);

		bool* in_Value = &DefaultValue;

		// Runtime default for the above inputs.  Editor defaults are set to the same values (see AnimationGraphNodes.cpp)
		// Individual graphs can override these defaults, in which case the values are saved in this->DefaultValuePlugs
		inline static bool DefaultValue = false;

		OutputEvent out_OnTrue;
		OutputEvent out_OnFalse;

	public:
		void Init() override;
		float Process(float) override;
	};

} // namespace Engine::AnimationGraph

#undef DECLARE_ID

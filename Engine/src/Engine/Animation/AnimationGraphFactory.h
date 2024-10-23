#pragma once

#include "AnimationNodeProcessor.h"

#include "Engine/Core/Base.h"
#include "Engine/Core/Identifier.h"
#include "Engine/Core/Ref.h"
#include "Engine/Core/UUID.h"

namespace Hazel::AnimationGraph {

	struct AnimationGraph;
	struct Prototype;

	class Factory
	{
		Factory() = delete;
	public:
		[[nodiscard]] static NodeProcessor* Create(Identifier nodeTypeID, UUID nodeID);
		static bool Contains(Identifier nodeTypeID);
	};

	Ref<AnimationGraph> CreateInstance(const Prototype& prototype);

} // namespace Hazel::AnimationGraph

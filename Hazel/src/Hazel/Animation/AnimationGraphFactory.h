#pragma once

#include "AnimationNodeProcessor.h"

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Identifier.h"
#include "Hazel/Core/Ref.h"
#include "Hazel/Core/UUID.h"

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

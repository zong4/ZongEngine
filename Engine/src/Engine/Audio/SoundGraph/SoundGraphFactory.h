#pragma once
#include "NodeProcessor.h"
#include "Engine/Core/Identifier.h"

namespace Engine::SoundGraph
{
	class Factory
	{
		Factory() = delete;
	public:
		[[nodiscard]] static NodeProcessor* Create(Identifier nodeTypeID, UUID nodeID);
		static bool Contains(Identifier nodeTypeID);
	};

} // namespace Engine::SoundGraph

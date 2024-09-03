#pragma once
#include "NodeProcessor.h"
#include "Hazel/Core/Identifier.h"

namespace Hazel::SoundGraph
{
	class Factory
	{
		Factory() = delete;
	public:
		[[nodiscard]] static NodeProcessor* Create(Identifier nodeTypeID, UUID nodeID);
		static bool Contains(Identifier nodeTypeID);
	};

} // namespace Hazel::SoundGraph

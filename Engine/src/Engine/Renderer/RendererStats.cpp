#include "pch.h"
#include "RendererStats.h"

namespace Engine {

	namespace RendererUtils {

		static ResourceAllocationCounts s_ResourceAllocationCounts;
		ResourceAllocationCounts& GetResourceAllocationCounts()
		{
			return s_ResourceAllocationCounts;
		}

	}

}

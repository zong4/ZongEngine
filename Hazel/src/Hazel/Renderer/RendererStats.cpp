#include "hzpch.h"
#include "RendererStats.h"

namespace Hazel {

	namespace RendererUtils {

		static ResourceAllocationCounts s_ResourceAllocationCounts;
		ResourceAllocationCounts& GetResourceAllocationCounts()
		{
			return s_ResourceAllocationCounts;
		}

	}

}

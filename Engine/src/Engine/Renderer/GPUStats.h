#pragma once

#include <stdint.h>

namespace Engine {

	struct GPUMemoryStats
	{
		uint64_t Used = 0;
		uint64_t TotalAvailable = 0;
		uint64_t AllocationCount = 0;

		uint64_t BufferAllocationSize = 0;
		uint64_t BufferAllocationCount = 0;

		uint64_t ImageAllocationSize = 0;
		uint64_t ImageAllocationCount = 0;
	};

}

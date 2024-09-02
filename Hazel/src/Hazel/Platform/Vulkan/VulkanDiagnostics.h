#pragma once

#include "Vulkan.h"

namespace Hazel::Utils {

	struct VulkanCheckpointData
	{
		char Data[64 + 1] {};
	};

	void SetVulkanCheckpoint(VkCommandBuffer commandBuffer, const std::string& data);

}


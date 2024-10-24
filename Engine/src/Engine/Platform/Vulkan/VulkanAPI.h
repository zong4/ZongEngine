#pragma once

#include "vulkan/vulkan.h"

namespace Engine::Vulkan {

	VkDescriptorSetAllocateInfo DescriptorSetAllocInfo(const VkDescriptorSetLayout* layouts, uint32_t count = 1, VkDescriptorPool pool = nullptr);

	VkSampler CreateSampler(VkSamplerCreateInfo samplerCreateInfo);
	void DestroySampler(VkSampler sampler);

}

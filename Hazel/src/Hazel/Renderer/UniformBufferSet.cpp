#include "hzpch.h"
#include "UniformBufferSet.h"

#include "Hazel/Renderer/Renderer.h"

#include "Hazel/Platform/Vulkan/VulkanUniformBufferSet.h"

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<UniformBufferSet> UniformBufferSet::Create(uint32_t size, uint32_t framesInFlight)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:   return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanUniformBufferSet>::Create(size, framesInFlight);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
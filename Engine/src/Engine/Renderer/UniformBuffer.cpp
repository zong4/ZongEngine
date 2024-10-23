#include "pch.h"
#include "UniformBuffer.h"

#include "Engine/Renderer/Renderer.h"

#include "Engine/Platform/Vulkan/VulkanUniformBuffer.h"

#include "Engine/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<UniformBuffer> UniformBuffer::Create(uint32_t size)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:     return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanUniformBuffer>::Create(size);
		}

		ZONG_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}

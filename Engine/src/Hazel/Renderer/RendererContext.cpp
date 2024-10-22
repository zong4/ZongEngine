#include "hzpch.h"
#include "RendererContext.h"

#include "Hazel/Renderer/RendererAPI.h"

#include "Hazel/Platform/Vulkan/VulkanContext.h"

namespace Hazel {

	Ref<RendererContext> RendererContext::Create()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanContext>::Create();
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
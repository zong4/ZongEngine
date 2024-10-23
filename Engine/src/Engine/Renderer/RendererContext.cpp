#include "pch.h"
#include "RendererContext.h"

#include "Engine/Renderer/RendererAPI.h"

#include "Engine/Platform/Vulkan/VulkanContext.h"

namespace Hazel {

	Ref<RendererContext> RendererContext::Create()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanContext>::Create();
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
#include "pch.h"
#include "RenderCommandBuffer.h"

#include "Engine/Platform/Vulkan/VulkanRenderCommandBuffer.h"

#include "Engine/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<RenderCommandBuffer> RenderCommandBuffer::Create(uint32_t count, const std::string& debugName)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanRenderCommandBuffer>::Create(count, debugName);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<RenderCommandBuffer> RenderCommandBuffer::CreateFromSwapChain(const std::string& debugName)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanRenderCommandBuffer>::Create(debugName, true);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
#include "hzpch.h"
#include "RenderPass.h"

#include "Renderer.h"

#include "Hazel/Platform/Vulkan/VulkanRenderPass.h"

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanRenderPass>::Create(spec);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
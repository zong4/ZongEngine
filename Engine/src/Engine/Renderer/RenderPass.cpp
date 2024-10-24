#include "pch.h"
#include "RenderPass.h"

#include "Renderer.h"

#include "Engine/Platform/Vulkan/VulkanRenderPass.h"

#include "Engine/Renderer/RendererAPI.h"

namespace Engine {

	Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    ZONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanRenderPass>::Create(spec);
		}

		ZONG_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
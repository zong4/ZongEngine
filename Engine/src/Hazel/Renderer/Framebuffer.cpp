#include "hzpch.h"
#include "Framebuffer.h"

#include "Hazel/Platform/Vulkan/VulkanFramebuffer.h"

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		Ref<Framebuffer> result = nullptr;

		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:		return nullptr;
			case RendererAPIType::Vulkan:	result = Ref<VulkanFramebuffer>::Create(spec); break;
		}
		return result;
	}

}

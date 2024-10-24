#include "pch.h"
#include "Framebuffer.h"

#include "Engine/Platform/Vulkan/VulkanFramebuffer.h"

#include "Engine/Renderer/RendererAPI.h"

namespace Engine {

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

#include "hzpch.h"
#include "ComputePass.h"

#include "Renderer.h"

#include "Engine/Platform/Vulkan/VulkanComputePass.h"

#include "Engine/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<ComputePass> ComputePass::Create(const ComputePassSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanComputePass>::Create(spec);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}

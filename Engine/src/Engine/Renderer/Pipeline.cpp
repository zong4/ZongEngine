#include "pch.h"
#include "Pipeline.h"

#include "Renderer.h"

#include "Engine/Platform/Vulkan/VulkanPipeline.h"

#include "Engine/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<Pipeline> Pipeline::Create(const PipelineSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanPipeline>::Create(spec);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
#include "pch.h"
#include "PipelineCompute.h"

#include "Engine/Renderer/RendererAPI.h"
#include "Engine/Platform/Vulkan/VulkanComputePipeline.h"

namespace Engine {

	Ref<PipelineCompute> PipelineCompute::Create(Ref<Shader> computeShader)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanComputePipeline>::Create(computeShader);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
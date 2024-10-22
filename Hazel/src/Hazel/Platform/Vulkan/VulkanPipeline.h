#pragma once

#include "Hazel/Renderer/Pipeline.h"

#include "Vulkan.h"
#include "VulkanShader.h"
#include <map>

namespace Hazel {

	class VulkanPipeline : public Pipeline
	{
	public:
		VulkanPipeline(const PipelineSpecification& spec);
		virtual ~VulkanPipeline();

		virtual PipelineSpecification& GetSpecification() { return m_Specification; }
		virtual const PipelineSpecification& GetSpecification() const { return m_Specification; }

		virtual void Invalidate() override;

		virtual Ref<Shader> GetShader() const override { return m_Specification.Shader; }

		bool IsDynamicLineWidth() const;

		VkPipeline GetVulkanPipeline() { return m_VulkanPipeline; }
		VkPipelineLayout GetVulkanPipelineLayout() { return m_PipelineLayout; }
	private:
		PipelineSpecification m_Specification;

		VkPipelineLayout m_PipelineLayout = nullptr;
		VkPipeline m_VulkanPipeline = nullptr;
		VkPipelineCache m_PipelineCache = nullptr;
	};

}

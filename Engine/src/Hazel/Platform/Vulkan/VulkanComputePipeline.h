#pragma once

#include "Hazel/Renderer/PipelineCompute.h"

#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanRenderCommandBuffer.h"

#include "vulkan/vulkan.h"

namespace Hazel {

	class VulkanComputePipeline : public PipelineCompute
	{
	public:
		VulkanComputePipeline(Ref<Shader> computeShader);

		void Execute(VkDescriptorSet* descriptorSets, uint32_t descriptorSetCount, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

		virtual void Begin(Ref<RenderCommandBuffer> renderCommandBuffer = nullptr) override;
		virtual void RT_Begin(Ref<RenderCommandBuffer> renderCommandBuffer = nullptr) override;
		void Dispatch(const glm::uvec3& workGroups) const;
		virtual void End() override;

		virtual Ref<Shader> GetShader() const override { return m_Shader; }
		
		VkCommandBuffer GetActiveCommandBuffer() { return m_ActiveComputeCommandBuffer; }
		VkPipelineLayout GetLayout() const { return m_ComputePipelineLayout; }

		void SetPushConstants(Buffer constants) const;
		void CreatePipeline();

		virtual void BufferMemoryBarrier(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<StorageBuffer> storageBuffer, ResourceAccessFlags fromAccess, ResourceAccessFlags toAccess) override;
		virtual void BufferMemoryBarrier(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<StorageBuffer> storageBuffer, PipelineStage fromStage, ResourceAccessFlags fromAccess, PipelineStage toStage, ResourceAccessFlags toAccess) override;

		virtual void ImageMemoryBarrier(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> image, ResourceAccessFlags fromAccess, ResourceAccessFlags toAccess) override;
		virtual void ImageMemoryBarrier(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> image, PipelineStage fromStage, ResourceAccessFlags fromAccess, PipelineStage toStage, ResourceAccessFlags toAccess) override;
	private:
		void RT_CreatePipeline();
	private:
		Ref<VulkanShader> m_Shader;

		VkPipelineLayout m_ComputePipelineLayout = nullptr;
		VkPipelineCache m_PipelineCache = nullptr;
		VkPipeline m_ComputePipeline = nullptr;

		VkCommandBuffer m_ActiveComputeCommandBuffer = nullptr;

		bool m_UsingGraphicsQueue = false;
	};

}

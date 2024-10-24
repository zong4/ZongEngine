#pragma once

#include "Engine/Renderer/ComputePass.h"
#include "VulkanRenderPass.h"
#include "DescriptorSetManager.h"

#include "vulkan/vulkan.h"

#include <set>

namespace Engine {

	class VulkanComputePass : public ComputePass
	{
	public:
		VulkanComputePass(const ComputePassSpecification& spec);
		virtual ~VulkanComputePass();

		virtual ComputePassSpecification& GetSpecification() override { return m_Specification; }
		virtual const ComputePassSpecification& GetSpecification() const override { return m_Specification; }

		virtual Ref<Shader> GetShader() const override { return m_Specification.Pipeline->GetShader(); }

		virtual void SetInput(std::string_view name, Ref<UniformBufferSet> uniformBufferSet) override;
		virtual void SetInput(std::string_view name, Ref<UniformBuffer> uniformBuffer) override;

		virtual void SetInput(std::string_view name, Ref<StorageBufferSet> storageBufferSet) override;
		virtual void SetInput(std::string_view name, Ref<StorageBuffer> storageBuffer) override;

		virtual void SetInput(std::string_view name, Ref<Texture2D> texture) override;
		virtual void SetInput(std::string_view name, Ref<TextureCube> textureCube) override;
		virtual void SetInput(std::string_view name, Ref<Image2D> image) override;

		virtual Ref<Image2D> GetOutput(uint32_t index) override;
		virtual Ref<Image2D> GetDepthOutput() override;
		virtual bool HasDescriptorSets() const override;
		virtual uint32_t GetFirstSetIndex() const override;

		virtual bool Validate() override;
		virtual void Bake() override;
		virtual bool Baked() const override { return (bool)m_DescriptorSetManager.GetDescriptorPool(); }
		virtual void Prepare() override;

		const std::vector<VkDescriptorSet>& GetDescriptorSets(uint32_t frameIndex) const;

		virtual Ref<PipelineCompute> GetPipeline() const;

		bool IsInputValid(std::string_view name) const;
		const RenderPassInputDeclaration* GetInputDeclaration(std::string_view name) const;
	private:
		std::set<uint32_t> HasBufferSets() const;
		bool IsInvalidated(uint32_t set, uint32_t binding) const;
	private:
		ComputePassSpecification m_Specification;
		DescriptorSetManager m_DescriptorSetManager;
	};

}

#pragma once

#include "Hazel/Renderer/RenderPass.h"

#include "DescriptorSetManager.h"

#include "vulkan/vulkan.h"

namespace Hazel {


	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass(const RenderPassSpecification& spec);
		virtual ~VulkanRenderPass();

		virtual RenderPassSpecification& GetSpecification() override { return m_Specification; }
		virtual const RenderPassSpecification& GetSpecification() const override { return m_Specification; }

		virtual void SetInput(std::string_view name, Ref<UniformBufferSet> uniformBufferSet) override;
		virtual void SetInput(std::string_view name, Ref<UniformBuffer> uniformBuffer) override;

		virtual void SetInput(std::string_view name, Ref<StorageBufferSet> storageBufferSet) override;
		virtual void SetInput(std::string_view name, Ref<StorageBuffer> storageBuffer) override;

		virtual void SetInput(std::string_view name, Ref<Texture2D> texture) override;
		virtual void SetInput(std::string_view name, Ref<TextureCube> textureCube) override;
		virtual void SetInput(std::string_view name, Ref<Image2D> image) override;

		virtual Ref<Image2D> GetOutput(uint32_t index) override;
		virtual Ref<Image2D> GetDepthOutput() override;
		virtual uint32_t GetFirstSetIndex() const override;

		virtual Ref<Framebuffer> GetTargetFramebuffer() const override;
		virtual Ref<Pipeline> GetPipeline() const override;

		virtual bool Validate() override;
		virtual void Bake() override;
		virtual bool Baked() const override { return (bool)m_DescriptorSetManager.GetDescriptorPool(); }
		virtual void Prepare() override;

		bool HasDescriptorSets() const;
		const std::vector<VkDescriptorSet>& GetDescriptorSets(uint32_t frameIndex) const;

		bool IsInputValid(std::string_view name) const;
		const RenderPassInputDeclaration* GetInputDeclaration(std::string_view name) const;
	private:
		bool IsInvalidated(uint32_t set, uint32_t binding) const;
	private:
		RenderPassSpecification m_Specification;
		DescriptorSetManager m_DescriptorSetManager;
	};

}

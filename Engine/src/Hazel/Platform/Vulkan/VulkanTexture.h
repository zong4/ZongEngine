#pragma once

#include "Hazel/Renderer/Texture.h"

#include "Vulkan.h"

#include "VulkanImage.h"

namespace Hazel {

	class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(const TextureSpecification& specification, const std::filesystem::path& filepath);
		VulkanTexture2D(const TextureSpecification& specification, Buffer data = Buffer());
		~VulkanTexture2D() override;
		
		virtual void Resize(const glm::uvec2& size) override;
		virtual void Resize(uint32_t width, uint32_t height) override;

		void Invalidate();

		virtual ImageFormat GetFormat() const override { return m_Specification.Format; }
		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }
		virtual glm::uvec2 GetSize() const override { return { m_Specification.Width, m_Specification.Height }; }

		virtual void Bind(uint32_t slot = 0) const override;

		virtual Ref<Image2D> GetImage() const override { return m_Image; }
		virtual ResourceDescriptorInfo GetDescriptorInfo() const override { return m_Image.As<VulkanImage2D>()->GetDescriptorInfo(); }
		const VkDescriptorImageInfo& GetDescriptorInfoVulkan() const { return *(VkDescriptorImageInfo*)GetDescriptorInfo(); }

		void Lock() override;
		void Unlock() override;

		Buffer GetWriteableBuffer() override;
		bool Loaded() const override { return m_ImageData; }
		const std::filesystem::path& GetPath() const override;
		uint32_t GetMipLevelCount() const override;
		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

		void GenerateMips();

		virtual uint64_t GetHash() const { return (uint64_t)m_Image.As<VulkanImage2D>()->GetDescriptorInfoVulkan().imageView; }

		void CopyToHostBuffer(Buffer& buffer);
	private:
		std::filesystem::path m_Path;
		TextureSpecification m_Specification;

		Buffer m_ImageData;

		Ref<Image2D> m_Image;
	};

	class VulkanTextureCube : public TextureCube
	{
	public:
		VulkanTextureCube(const TextureSpecification& specification, Buffer data = nullptr);
		virtual ~VulkanTextureCube();
		
		void Release();

		virtual void Bind(uint32_t slot = 0) const override {}

		virtual ImageFormat GetFormat() const override { return m_Specification.Format; }

		virtual uint32_t GetWidth() const override{ return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }
		virtual glm::uvec2 GetSize() const override { return { m_Specification.Width, m_Specification.Height}; }

		virtual uint32_t GetMipLevelCount() const override;
		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

		virtual uint64_t GetHash() const override { return (uint64_t)m_Image; }

		virtual ResourceDescriptorInfo GetDescriptorInfo() const override { return (ResourceDescriptorInfo)&m_DescriptorImageInfo; }
		const VkDescriptorImageInfo& GetDescriptorInfoVulkan() const { return *(VkDescriptorImageInfo*)GetDescriptorInfo(); }

		VkImageView CreateImageViewSingleMip(uint32_t mip);

		void GenerateMips(bool readonly = false);

		void CopyToHostBuffer(Buffer& buffer);
		void CopyFromBuffer(const Buffer& buffer, uint32_t mips);
	private:
		void Invalidate();
	private:
		TextureSpecification m_Specification;

		bool m_MipsGenerated = false;

		Buffer m_LocalStorage;
		VmaAllocation m_MemoryAlloc;
		uint64_t m_GPUAllocationSize = 0;
		VkImage m_Image { nullptr };
		VkDescriptorImageInfo m_DescriptorImageInfo = {};
	};

}

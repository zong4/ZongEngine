#include "hzpch.h"
#include "VulkanTexture.h"

#include "VulkanContext.h"

#include "VulkanImage.h"
#include "VulkanRenderer.h"
#include "VulkanAPI.h"

#include "Engine/Asset/TextureImporter.h"

namespace Hazel {

	namespace Utils {

		static VkSamplerAddressMode VulkanSamplerWrap(TextureWrap wrap)
		{
			switch (wrap)
			{
				case TextureWrap::Clamp:   return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				case TextureWrap::Repeat:  return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			}
			HZ_CORE_ASSERT(false, "Unknown wrap mode");
			return (VkSamplerAddressMode)0;
		}

		static VkFilter VulkanSamplerFilter(TextureFilter filter)
		{
			switch (filter)
			{
				case TextureFilter::Linear:   return VK_FILTER_LINEAR;
				case TextureFilter::Nearest:  return VK_FILTER_NEAREST;
				case TextureFilter::Cubic:   return VK_FILTER_CUBIC_IMG;
			}
			HZ_CORE_ASSERT(false, "Unknown filter");
			return (VkFilter)0;
		}

		static size_t GetMemorySize(ImageFormat format, uint32_t width, uint32_t height)
		{
			switch (format)
			{
				case ImageFormat::RED16UI: return width * height * sizeof(uint16_t);
				case ImageFormat::RG16F: return width * height * 2 * sizeof(uint16_t);
				case ImageFormat::RG32F: return width * height * 2 * sizeof(float);
				case ImageFormat::RED32F: return width * height * sizeof(float);
				case ImageFormat::RED8UN: return width * height;
				case ImageFormat::RED8UI: return width * height;
				case ImageFormat::RGBA: return width * height * 4;
				case ImageFormat::RGBA32F: return width * height * 4 * sizeof(float);
				case ImageFormat::B10R11G11UF: return width * height * sizeof(float);
			}
			HZ_CORE_ASSERT(false);
			return 0;
		}

		static bool ValidateSpecification(const TextureSpecification& specification)
		{
			bool result = true;

			result = specification.Width > 0 && specification.Height > 0 && specification.Width < 65536 && specification.Height < 65536;
			HZ_CORE_VERIFY(result);

			return result;
		}

	}

	//////////////////////////////////////////////////////////////////////////////////
	// Texture2D
	//////////////////////////////////////////////////////////////////////////////////

	VulkanTexture2D::VulkanTexture2D(const TextureSpecification& specification, const std::filesystem::path& filepath)
		: m_Path(filepath), m_Specification(specification)
	{ 
		Utils::ValidateSpecification(specification);

		m_ImageData = TextureImporter::ToBufferFromFile(filepath, m_Specification.Format, m_Specification.Width, m_Specification.Height);
		if (!m_ImageData)
		{
			// TODO(Yan): move this to asset manager
			m_ImageData = TextureImporter::ToBufferFromFile("Resources/Textures/ErrorTexture.png", m_Specification.Format, m_Specification.Width, m_Specification.Height);
		}

		ImageSpecification imageSpec;
		imageSpec.Format = m_Specification.Format;
		imageSpec.Width = m_Specification.Width;
		imageSpec.Height = m_Specification.Height;
		imageSpec.Mips = specification.GenerateMips ? GetMipLevelCount() : 1;
		imageSpec.DebugName = specification.DebugName;
		imageSpec.CreateSampler = false;
		m_Image = Image2D::Create(imageSpec);

		HZ_CORE_ASSERT(m_Specification.Format != ImageFormat::None);

		Ref<VulkanTexture2D> instance = this;
		Renderer::Submit([instance]() mutable
		{
			instance->Invalidate();
		});
	}

	VulkanTexture2D::VulkanTexture2D(const TextureSpecification& specification, Buffer data)
		: m_Specification(specification)
	{
		if (m_Specification.Height == 0)
		{
			m_ImageData = TextureImporter::ToBufferFromMemory(Buffer(data.Data, m_Specification.Width), m_Specification.Format, m_Specification.Width, m_Specification.Height);
			if (!m_ImageData)
			{
				// TODO(Yan): move this to asset manager
				m_ImageData = TextureImporter::ToBufferFromFile("Resources/Textures/ErrorTexture.png", m_Specification.Format, m_Specification.Width, m_Specification.Height);
			}

			Utils::ValidateSpecification(m_Specification);
		}
		else if (data)
		{
			Utils::ValidateSpecification(m_Specification);
			auto size = (uint32_t)Utils::GetMemorySize(m_Specification.Format, m_Specification.Width, m_Specification.Height);
			m_ImageData = Buffer::Copy(data.Data, size);
		}
		else
		{
			Utils::ValidateSpecification(m_Specification);
			auto size = (uint32_t)Utils::GetMemorySize(m_Specification.Format, m_Specification.Width, m_Specification.Height);
			m_ImageData.Allocate(size);
			m_ImageData.ZeroInitialize();
		}

		ImageSpecification imageSpec;
		imageSpec.Format = m_Specification.Format;
		imageSpec.Width = m_Specification.Width;
		imageSpec.Height = m_Specification.Height;
		imageSpec.Mips = specification.GenerateMips ? VulkanTexture2D::GetMipLevelCount() : 1;
		imageSpec.DebugName = specification.DebugName;
		imageSpec.CreateSampler = false;
		if (specification.Storage)
			imageSpec.Usage = ImageUsage::Storage;
		m_Image = Image2D::Create(imageSpec);

		Ref<VulkanTexture2D> instance = this;
		Renderer::Submit([instance]() mutable
		{
			instance->Invalidate();
		});
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		if (m_Image)
			m_Image->Release();

		m_ImageData.Release();
	}

	void VulkanTexture2D::Resize(const glm::uvec2& size)
	{
		Resize(size.x, size.y);
	}

	void VulkanTexture2D::Resize(const uint32_t width, const uint32_t height)
	{
		m_Specification.Width = width;
		m_Specification.Height = height;

		//Invalidate();

		Ref<VulkanTexture2D> instance = this;
		Renderer::Submit([instance]() mutable
		{
			instance->Invalidate();
		});
	}

	void VulkanTexture2D::Invalidate()
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		m_Image->Release();

		uint32_t mipCount = m_Specification.GenerateMips ? GetMipLevelCount() : 1;

		ImageSpecification& imageSpec = m_Image->GetSpecification();
		imageSpec.Format = m_Specification.Format;
		imageSpec.Width = m_Specification.Width;
		imageSpec.Height = m_Specification.Height;
		imageSpec.Mips = mipCount;
		imageSpec.CreateSampler = false;
		if (!m_ImageData) // TODO(Yan): better management for this, probably from texture spec
			imageSpec.Usage = ImageUsage::Storage;

		Ref<VulkanImage2D> image = m_Image.As<VulkanImage2D>();
		image->RT_Invalidate();

		auto& info = image->GetImageInfo();

		if (m_ImageData)
		{
			VkDeviceSize size = m_ImageData.Size;

			VkMemoryAllocateInfo memAllocInfo {};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

			VulkanAllocator allocator("Texture2D");

			// Create staging buffer
			VkBufferCreateInfo bufferCreateInfo {};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = size;
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VkBuffer stagingBuffer;
			VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

			// Copy data to staging buffer
			uint8_t* destData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
			HZ_CORE_ASSERT(m_ImageData.Data);
			memcpy(destData, m_ImageData.Data, size);
			allocator.UnmapMemory(stagingBufferAllocation);

			VkCommandBuffer copyCmd = device->GetCommandBuffer(true);

			// Image memory barriers for the texture image

			// The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
			VkImageSubresourceRange subresourceRange = {};
			// Image only contains color data
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			// Start at first mip level
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;

			// Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
			VkImageMemoryBarrier imageMemoryBarrier {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.image = info.Image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
			// Source pipeline stage is host write/read exection (VK_PIPELINE_STAGE_HOST_BIT)
			// Destination pipeline stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_HOST_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = m_Specification.Width;
			bufferCopyRegion.imageExtent.height = m_Specification.Height;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = 0;

			// Copy mip levels from staging buffer
			vkCmdCopyBufferToImage(
				copyCmd,
				stagingBuffer,
				info.Image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&bufferCopyRegion);

			#if 0
			// Once the data has been uploaded we transfer to the texture image to the shader read layout, so it can be sampled from
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
			// Source pipeline stage stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
			// Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			#endif

			if (mipCount > 1) // Mips to generate
			{
				Utils::InsertImageMemoryBarrier(copyCmd, info.Image,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					subresourceRange);
			}
			else
			{
				Utils::InsertImageMemoryBarrier(copyCmd, info.Image,
					VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image->GetDescriptorInfoVulkan().imageLayout,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					subresourceRange);
			}
			

			device->FlushCommandBuffer(copyCmd);

			// Clean up staging resources
			allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
		}
		else
		{
			VkCommandBuffer transitionCommandBuffer = device->GetCommandBuffer(true);
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.layerCount = 1;
			subresourceRange.levelCount = GetMipLevelCount();
			Utils::SetImageLayout(transitionCommandBuffer, info.Image, VK_IMAGE_LAYOUT_UNDEFINED, image->GetDescriptorInfoVulkan().imageLayout, subresourceRange);
			device->FlushCommandBuffer(transitionCommandBuffer);
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// CREATE TEXTURE SAMPLER (owned by Image)
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Create a texture sampler
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.magFilter = Utils::VulkanSamplerFilter(m_Specification.SamplerFilter);
		samplerInfo.minFilter = Utils::VulkanSamplerFilter(m_Specification.SamplerFilter);
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = Utils::VulkanSamplerWrap(m_Specification.SamplerWrap);
		samplerInfo.addressModeV = Utils::VulkanSamplerWrap(m_Specification.SamplerWrap);
		samplerInfo.addressModeW = Utils::VulkanSamplerWrap(m_Specification.SamplerWrap);
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = (float)mipCount;
		// Enable anisotropic filtering
		// This feature is optional, so we must check if it's supported on the device

		// TODO:
		/*if (vulkanDevice->features.samplerAnisotropy) {
				// Use max. level of anisotropy for this example
				sampler.maxAnisotropy = 1.0f;// vulkanDevice->properties.limits.maxSamplerAnisotropy;
				sampler.anisotropyEnable = VK_TRUE;
		}
		else {
				// The device does not support anisotropic filtering
				sampler.maxAnisotropy = 1.0;
				sampler.anisotropyEnable = VK_FALSE;
		}*/
		samplerInfo.maxAnisotropy = 1.0;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		info.Sampler = Vulkan::CreateSampler(samplerInfo);
		image->UpdateDescriptor();

		if (!m_Specification.Storage)
		{
			VkImageViewCreateInfo view{};
			view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view.format = Utils::VulkanImageFormat(m_Specification.Format);
			view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			// The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
			// It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
			view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			view.subresourceRange.baseMipLevel = 0;
			view.subresourceRange.baseArrayLayer = 0;
			view.subresourceRange.layerCount = 1;
			view.subresourceRange.levelCount = mipCount;
			view.image = info.Image;
			VK_CHECK_RESULT(vkCreateImageView(vulkanDevice, &view, nullptr, &info.ImageView));

			VKUtils::SetDebugUtilsObjectName(vulkanDevice, VK_OBJECT_TYPE_IMAGE_VIEW, fmt::format("Texture view: {}", m_Specification.DebugName), info.ImageView);

			image->UpdateDescriptor();
		}

		if (m_ImageData && m_Specification.GenerateMips && mipCount > 1)
			GenerateMips();

		// TODO(Yan): option for local storage
		m_ImageData.Release();
		m_ImageData = Buffer();
	}

	void VulkanTexture2D::Bind(uint32_t slot) const
	{
	}

	void VulkanTexture2D::Lock()
	{
	}

	void VulkanTexture2D::Unlock()
	{
	}

	Buffer VulkanTexture2D::GetWriteableBuffer()
	{
		return m_ImageData;
	}

	const std::filesystem::path& VulkanTexture2D::GetPath() const
	{
		return m_Path;
	}

	uint32_t VulkanTexture2D::GetMipLevelCount() const
	{
		return Utils::CalculateMipCount(m_Specification.Width, m_Specification.Height);
	}

	std::pair<uint32_t, uint32_t> VulkanTexture2D::GetMipSize(uint32_t mip) const
	{
		uint32_t width = m_Specification.Width;
		uint32_t height = m_Specification.Height;
		while (mip != 0)
		{
			width /= 2;
			height /= 2;
			mip--;
		}

		return { width, height };
	}

	void VulkanTexture2D::GenerateMips()
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		Ref<VulkanImage2D> image = m_Image.As<VulkanImage2D>();
		const auto& info = image->GetImageInfo();

		const VkCommandBuffer blitCmd = VulkanContext::GetCurrentDevice()->GetCommandBuffer(true);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = info.Image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		const auto mipLevels = GetMipLevelCount();
		for (uint32_t i = 1; i < mipLevels; i++)
		{
			VkImageBlit imageBlit {};

			// Source
			imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.srcSubresource.layerCount = 1;
			imageBlit.srcSubresource.mipLevel = i - 1;
			imageBlit.srcOffsets[1].x = int32_t(m_Specification.Width >> (i - 1));
			imageBlit.srcOffsets[1].y = int32_t(m_Specification.Height >> (i - 1));
			imageBlit.srcOffsets[1].z = 1;

			// Destination
			imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.dstSubresource.layerCount = 1;
			imageBlit.dstSubresource.mipLevel = i;
			imageBlit.dstOffsets[1].x = int32_t(m_Specification.Width >> i);
			imageBlit.dstOffsets[1].y = int32_t(m_Specification.Height >> i);
			imageBlit.dstOffsets[1].z = 1;

			VkImageSubresourceRange mipSubRange = {};
			mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			mipSubRange.baseMipLevel = i;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = 1;

			// Prepare current mip level as image blit destination
			Utils::InsertImageMemoryBarrier(blitCmd, info.Image,
											0, VK_ACCESS_TRANSFER_WRITE_BIT,
											VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
											mipSubRange);

			// Blit from previous level
			vkCmdBlitImage(
				blitCmd,
				info.Image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				info.Image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&imageBlit,
				Utils::VulkanSamplerFilter(m_Specification.SamplerFilter));

			// Prepare current mip level as image blit source for next level
			Utils::InsertImageMemoryBarrier(blitCmd, info.Image,
											VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
											VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
											VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
											mipSubRange);
		}

		// After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.layerCount = 1;
		subresourceRange.levelCount = mipLevels;

		Utils::InsertImageMemoryBarrier(blitCmd, info.Image,
										VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
										VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
										VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
										subresourceRange);

		VulkanContext::GetCurrentDevice()->FlushCommandBuffer(blitCmd);

		#if 0
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = m_Image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;
		barrier.subresourceRange = subresourceRange;

		int32_t mipWidth = m_Specification.Width;
		int32_t mipHeight = m_Specification.Height;

		VkCommandBuffer commandBuffer = VulkanContext::GetCurrentDevice()->GetCommandBuffer(true);

		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
							 0, nullptr,
							 0, nullptr,
							 1, &barrier);

		auto mipLevels = GetMipLevelCount();
		for (uint32_t i = 1; i < mipLevels; i++)
		{
			VkImageBlit blit = {};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;

			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth / 2, mipHeight / 2, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
						   m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						   m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						   1, &blit,
						   VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.subresourceRange.baseMipLevel = i;

			vkCmdPipelineBarrier(commandBuffer,
								 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
								 0, nullptr,
								 0, nullptr,
								 1, &barrier);

			if (mipWidth > 1)
				mipWidth /= 2;
			if (mipHeight > 1)
				mipHeight /= 2;
		}

		// Transition all mips from transfer to shader read
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
							 0, nullptr,
							 0, nullptr,
							 1, &barrier);

		VulkanContext::GetCurrentDevice()->FlushCommandBuffer(commandBuffer);
		#endif
	}

	void VulkanTexture2D::CopyToHostBuffer(Buffer& buffer)
	{
		if (m_Image)
			m_Image.As<VulkanImage2D>()->CopyToHostBuffer(buffer);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// TextureCube
	//////////////////////////////////////////////////////////////////////////////////

	VulkanTextureCube::VulkanTextureCube(const TextureSpecification& specification, Buffer data)
		: m_Specification(specification)
	{
		if (data)
		{
			uint32_t size = m_Specification.Width * m_Specification.Height * 4 * 6; // six layers
			m_LocalStorage = Buffer::Copy(data.Data, size);
		}
		
		Invalidate();
		//Ref<VulkanTextureCube> instance = this;
		//Renderer::Submit([instance]() mutable
		//{
		//	instance->Invalidate();
		//});
	}

	void VulkanTextureCube::Release()
	{
		if (m_Image == nullptr)
			return;

		Renderer::SubmitResourceFree([image = m_Image, allocation = m_MemoryAlloc, texInfo = m_DescriptorImageInfo]()
		{
			HZ_CORE_TRACE_TAG("Renderer", "Destroying VulkanTextureCube");
			auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			vkDestroyImageView(vulkanDevice, texInfo.imageView, nullptr);
			Vulkan::DestroySampler(texInfo.sampler);

			VulkanAllocator allocator("TextureCube");
			allocator.DestroyImage(image, allocation);
		});
		m_Image = nullptr;
		m_MemoryAlloc = nullptr;
		m_DescriptorImageInfo.imageView = nullptr;
		m_DescriptorImageInfo.sampler = nullptr;
	}

	VulkanTextureCube::~VulkanTextureCube()
	{
		Release();
	}

	void VulkanTextureCube::Invalidate()
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		Release();

		VkFormat format = Utils::VulkanImageFormat(m_Specification.Format);
		uint32_t mipCount = GetMipLevelCount();

		VkMemoryAllocateInfo memAllocInfo {};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		VulkanAllocator allocator("TextureCube");

		// Create optimal tiled target image on the device
		VkImageCreateInfo imageCreateInfo {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = mipCount;
		imageCreateInfo.arrayLayers = 6;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.extent = { m_Specification.Width, m_Specification.Height, 1 };
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		m_MemoryAlloc = allocator.AllocateImage(imageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, m_Image, &m_GPUAllocationSize);

		m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		// Copy data if present
		if (m_LocalStorage)
		{
			// Create staging buffer
			VkBufferCreateInfo bufferCreateInfo {};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = m_LocalStorage.Size;
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VkBuffer stagingBuffer;
			VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

			// Copy data to staging buffer
			uint8_t* destData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
			memcpy(destData, m_LocalStorage.Data, m_LocalStorage.Size);
			allocator.UnmapMemory(stagingBufferAllocation);

			VkCommandBuffer copyCmd = device->GetCommandBuffer(true);

			// Image memory barriers for the texture image

			// The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
			VkImageSubresourceRange subresourceRange = {};
			// Image only contains color data
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			// Start at first mip level
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 6;

			// Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
			VkImageMemoryBarrier imageMemoryBarrier {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.image = m_Image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
			// Source pipeline stage is host write/read exection (VK_PIPELINE_STAGE_HOST_BIT)
			// Destination pipeline stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_HOST_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 6;
			bufferCopyRegion.imageExtent.width = m_Specification.Width;
			bufferCopyRegion.imageExtent.height = m_Specification.Height;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = 0;

			// Copy mip levels from staging buffer
			vkCmdCopyBufferToImage(
				copyCmd,
				stagingBuffer,
				m_Image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&bufferCopyRegion);

			Utils::InsertImageMemoryBarrier(copyCmd, m_Image,
											VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
											VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
											VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
											subresourceRange);

			device->FlushCommandBuffer(copyCmd);

			allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
		}

		VkCommandBuffer layoutCmd = device->GetCommandBuffer(true);

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipCount;
		subresourceRange.layerCount = 6;

		Utils::SetImageLayout(
			layoutCmd, m_Image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			m_DescriptorImageInfo.imageLayout,
			subresourceRange);

		device->FlushCommandBuffer(layoutCmd);

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// CREATE TEXTURE SAMPLER
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Create a texture sampler
		VkSamplerCreateInfo sampler {};
		sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler.maxAnisotropy = 1.0f;
		sampler.magFilter = Utils::VulkanSamplerFilter(m_Specification.SamplerFilter);
		sampler.minFilter = Utils::VulkanSamplerFilter(m_Specification.SamplerFilter);
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = Utils::VulkanSamplerWrap(m_Specification.SamplerWrap);
		sampler.addressModeV = Utils::VulkanSamplerWrap(m_Specification.SamplerWrap);
		sampler.addressModeW = Utils::VulkanSamplerWrap(m_Specification.SamplerWrap);
		sampler.mipLodBias = 0.0f;
		sampler.compareOp = VK_COMPARE_OP_NEVER;
		sampler.minLod = 0.0f;
		// Set max level-of-detail to mip level count of the texture
		sampler.maxLod = (float)mipCount;
		// Enable anisotropic filtering
		// This feature is optional, so we must check if it's supported on the device

		sampler.maxAnisotropy = 1.0;
		sampler.anisotropyEnable = VK_FALSE;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		m_DescriptorImageInfo.sampler = Vulkan::CreateSampler(sampler);

		// Create image view
		// Textures are not directly accessed by the shaders and
		// are abstracted by image views containing additional
		// information and sub resource ranges
		VkImageViewCreateInfo view {};
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		// The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
		// It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view.subresourceRange.baseMipLevel = 0;
		view.subresourceRange.baseArrayLayer = 0;
		view.subresourceRange.layerCount = 6;
		view.subresourceRange.levelCount = mipCount;
		view.image = m_Image;
		VK_CHECK_RESULT(vkCreateImageView(vulkanDevice, &view, nullptr, &m_DescriptorImageInfo.imageView));

		VKUtils::SetDebugUtilsObjectName(vulkanDevice, VK_OBJECT_TYPE_IMAGE_VIEW, fmt::format("Texture cube view: {}", m_Specification.DebugName), m_DescriptorImageInfo.imageView);
	}

	uint32_t VulkanTextureCube::GetMipLevelCount() const
	{
		return Utils::CalculateMipCount(m_Specification.Width, m_Specification.Height);
	}

	std::pair<uint32_t, uint32_t> VulkanTextureCube::GetMipSize(uint32_t mip) const
	{
		uint32_t width = m_Specification.Width;
		uint32_t height = m_Specification.Height;
		while (mip != 0)
		{
			width /= 2;
			height /= 2;
			mip--;
		}

		return { width, height };
	}

	VkImageView VulkanTextureCube::CreateImageViewSingleMip(uint32_t mip)
	{
		// TODO: assert to check mip count

		auto device = VulkanContext::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		VkFormat format = Utils::VulkanImageFormat(m_Specification.Format);

		VkImageViewCreateInfo view {};
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view.subresourceRange.baseMipLevel = mip;
		view.subresourceRange.baseArrayLayer = 0;
		view.subresourceRange.layerCount = 6;
		view.subresourceRange.levelCount = 1;
		view.image = m_Image;

		VkImageView result;
		VK_CHECK_RESULT(vkCreateImageView(vulkanDevice, &view, nullptr, &result));
		VKUtils::SetDebugUtilsObjectName(vulkanDevice, VK_OBJECT_TYPE_IMAGE_VIEW, fmt::format("Texture cube mip: {}", mip), result);

		return result;
	}

	void VulkanTextureCube::GenerateMips(bool readonly)
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		VkCommandBuffer blitCmd = VulkanContext::GetCurrentDevice()->GetCommandBuffer(true);

		uint32_t mipLevels = GetMipLevelCount();
		for (uint32_t face = 0; face < 6; face++)
		{
			VkImageSubresourceRange mipSubRange = {};
			mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			mipSubRange.baseMipLevel = 0;
			mipSubRange.baseArrayLayer = face;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = 1;

			// Prepare current mip level as image blit destination
			Utils::InsertImageMemoryBarrier(blitCmd, m_Image,
											0, VK_ACCESS_TRANSFER_WRITE_BIT,
											VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
											VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
											mipSubRange);
		}

		for (uint32_t i = 1; i < mipLevels; i++)
		{
			for (uint32_t face = 0; face < 6; face++)
			{
				VkImageBlit imageBlit {};

				// Source
				imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.srcSubresource.layerCount = 1;
				imageBlit.srcSubresource.mipLevel = i - 1;
				imageBlit.srcSubresource.baseArrayLayer = face;
				imageBlit.srcOffsets[1].x = int32_t(m_Specification.Width >> (i - 1));
				imageBlit.srcOffsets[1].y = int32_t(m_Specification.Height >> (i - 1));
				imageBlit.srcOffsets[1].z = 1;

				// Destination
				imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.dstSubresource.layerCount = 1;
				imageBlit.dstSubresource.mipLevel = i;
				imageBlit.dstSubresource.baseArrayLayer = face;
				imageBlit.dstOffsets[1].x = int32_t(m_Specification.Width >> i);
				imageBlit.dstOffsets[1].y = int32_t(m_Specification.Height >> i);
				imageBlit.dstOffsets[1].z = 1;

				VkImageSubresourceRange mipSubRange = {};
				mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				mipSubRange.baseMipLevel = i;
				mipSubRange.baseArrayLayer = face;
				mipSubRange.levelCount = 1;
				mipSubRange.layerCount = 1;

				// Prepare current mip level as image blit destination
				Utils::InsertImageMemoryBarrier(blitCmd, m_Image,
												0, VK_ACCESS_TRANSFER_WRITE_BIT,
												VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
												VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
												mipSubRange);

				// Blit from previous level
				vkCmdBlitImage(
					blitCmd,
					m_Image,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					m_Image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&imageBlit,
					VK_FILTER_LINEAR);

				// Prepare current mip level as image blit source for next level
				Utils::InsertImageMemoryBarrier(blitCmd, m_Image,
												VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
												VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
												VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
												mipSubRange);
			}
		}

		// After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.layerCount = 6;
		subresourceRange.levelCount = mipLevels;

		Utils::InsertImageMemoryBarrier(blitCmd, m_Image,
										VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
										VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readonly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL,
										VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
										subresourceRange);

		VulkanContext::GetCurrentDevice()->FlushCommandBuffer(blitCmd);

		m_MipsGenerated = true;

		m_DescriptorImageInfo.imageLayout = readonly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;

	}

#if 0
	void VulkanTextureCube::CopyToHostBuffer(Buffer& buffer)
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();
		VulkanAllocator allocator("TextureCube");

		// Create staging buffer
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = m_GPUAllocationSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		
		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_GPU_TO_CPU, stagingBuffer);

		VkCommandBuffer copyCmd = device->GetCommandBufferAssetThread(true);

		// Image memory barriers for the texture image
		// The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
		VkImageSubresourceRange subresourceRange = {};
		// Image only contains color data
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// Start at first mip level
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = GetMipLevelCount();
		subresourceRange.layerCount = 6;

		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = m_Image;
		imageMemoryBarrier.subresourceRange = subresourceRange;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.oldLayout = m_DescriptorImageInfo.imageLayout;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
		// Source pipeline stage is host write/read exection (VK_PIPELINE_STAGE_HOST_BIT)
		// Destination pipeline stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
		vkCmdPipelineBarrier(
			copyCmd,
			VK_PIPELINE_STAGE_HOST_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		bufferCopyRegion.imageSubresource.layerCount = 6;
		bufferCopyRegion.imageExtent.width = m_Specification.Width;
		bufferCopyRegion.imageExtent.height = m_Specification.Height;
		bufferCopyRegion.imageExtent.depth = 1;
		bufferCopyRegion.bufferOffset = 0;

		// Copy mip levels from staging buffer
		vkCmdCopyImageToBuffer(
			copyCmd,
			m_Image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			stagingBuffer,
			1,
			&bufferCopyRegion);

		Utils::InsertImageMemoryBarrier(copyCmd, m_Image,
										VK_ACCESS_TRANSFER_WRITE_BIT, 0,
										VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_DescriptorImageInfo.imageLayout,
										VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
										subresourceRange);

		device->FlushCommandBufferAssetThread(copyCmd);

		// Copy data from staging buffer
		uint8_t* srcData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
		buffer.Allocate(m_GPUAllocationSize);
		memcpy(buffer.Data, srcData, m_GPUAllocationSize);
		allocator.UnmapMemory(stagingBufferAllocation);

		allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
	}
#endif

	void VulkanTextureCube::CopyToHostBuffer(Buffer& buffer)
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();
		VulkanAllocator allocator("TextureCube");

		uint32_t mipCount = GetMipLevelCount();

		uint32_t faces = 6;
		uint32_t bpp = sizeof(float) * 4;
		uint64_t bufferSize = 0;
		uint32_t w = m_Specification.Width, h = m_Specification.Height;

		for (int i = 0; i < mipCount; i++)
		{
			bufferSize += w * h * bpp * faces;
			w /= 2;
			h /= 2;
		}

		// Create staging buffer
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = bufferSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_GPU_TO_CPU, stagingBuffer);

		uint32_t mipWidth = m_Specification.Width, mipHeight = m_Specification.Height;

		VkCommandBuffer copyCmd = device->GetCommandBuffer(true);
		
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipCount;
		subresourceRange.layerCount = 6;

		Utils::InsertImageMemoryBarrier(copyCmd, m_Image,
										VK_ACCESS_TRANSFER_READ_BIT, 0,
										m_DescriptorImageInfo.imageLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
										VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
										subresourceRange);

		uint64_t mipDataOffset = 0;
		for (uint32_t mip = 0; mip < mipCount; mip++)
		{
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = mip;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 6;
			bufferCopyRegion.imageExtent.width = mipWidth;
			bufferCopyRegion.imageExtent.height = mipHeight;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = mipDataOffset;

			vkCmdCopyImageToBuffer(
				copyCmd,
				m_Image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				stagingBuffer,
				1,
				&bufferCopyRegion);

			uint64_t mipDataSize = mipWidth * mipHeight * sizeof(float) * 4 * 6;
			mipDataOffset += mipDataSize;
			mipWidth /= 2;
			mipHeight /= 2;
		}

		Utils::InsertImageMemoryBarrier(copyCmd, m_Image,
										VK_ACCESS_TRANSFER_READ_BIT, 0,
										VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_DescriptorImageInfo.imageLayout,
										VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
										subresourceRange);

		device->FlushCommandBuffer(copyCmd);

		// Copy data from staging buffer
		uint8_t* srcData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
		buffer.Allocate(bufferSize);
		memcpy(buffer.Data, srcData, bufferSize);
		allocator.UnmapMemory(stagingBufferAllocation);

		allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
	}

	void VulkanTextureCube::CopyFromBuffer(const Buffer& buffer, uint32_t mips)
	{
		// HZ_CORE_VERIFY(buffer.Size == m_GPUAllocationSize);

		auto device = VulkanContext::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();
		VulkanAllocator allocator("TextureCube");

		// Create staging buffer
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = buffer.Size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

		// Copy data from staging buffer
		uint8_t* dstData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
		memcpy(dstData, buffer.Data, buffer.Size);
		allocator.UnmapMemory(stagingBufferAllocation);

		uint32_t mipWidth = m_Specification.Width, mipHeight = m_Specification.Height;

		VkCommandBuffer copyCmd = device->GetCommandBuffer(true);
		uint64_t mipDataOffset = 0;
		for (uint32_t mip = 0; mip < mips; mip++)
		{
			// Image memory barriers for the texture image
			// The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
			VkImageSubresourceRange subresourceRange = {};
			// Image only contains color data
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			// Start at first mip level
			subresourceRange.baseMipLevel = mip;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 6;

			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.image = m_Image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.oldLayout = m_DescriptorImageInfo.imageLayout;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
			// Source pipeline stage is host write/read exection (VK_PIPELINE_STAGE_HOST_BIT)
			// Destination pipeline stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_HOST_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = mip;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 6;
			bufferCopyRegion.imageExtent.width = mipWidth;
			bufferCopyRegion.imageExtent.height = mipHeight;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = mipDataOffset;

			vkCmdCopyBufferToImage(
				copyCmd,
				stagingBuffer,
				m_Image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&bufferCopyRegion);

			uint64_t mipDataSize = mipWidth * mipHeight * sizeof(float) * 4 * 6;
			mipDataOffset += mipDataSize;

			mipWidth /= 2;
			mipHeight /= 2;

			Utils::InsertImageMemoryBarrier(copyCmd, m_Image,
											VK_ACCESS_TRANSFER_WRITE_BIT, 0,
											VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_DescriptorImageInfo.imageLayout,
											VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
											subresourceRange);
		}
		device->FlushCommandBuffer(copyCmd);

		allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
	}

}

#pragma once


#include "Hazel/Renderer/StorageBuffer.h"
#include "VulkanAllocator.h"

namespace Hazel {

	class VulkanStorageBuffer : public StorageBuffer
	{
	public:
		VulkanStorageBuffer(uint32_t size, const StorageBufferSpecification& specification);
		virtual ~VulkanStorageBuffer() override;

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
		virtual void RT_SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
		virtual void Resize(uint32_t newSize) override;

		VkBuffer GetVulkanBuffer() const { return m_Buffer; }
		const VkDescriptorBufferInfo& GetDescriptorBufferInfo() const { return m_DescriptorInfo; }
	private:
		void Release();
		void RT_Invalidate();
	private:
		StorageBufferSpecification m_Specification;
		VmaAllocation m_MemoryAlloc = nullptr;
		VkBuffer m_Buffer {};
		VkDescriptorBufferInfo m_DescriptorInfo{};
		uint32_t m_Size = 0;
		std::string m_Name;
		VkShaderStageFlagBits m_ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		VmaAllocation m_StagingAlloc = nullptr;
		VkBuffer m_StagingBuffer = nullptr;

		uint8_t* m_LocalStorage = nullptr;
	};
}

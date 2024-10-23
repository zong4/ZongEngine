#include "pch.h"
#include "VulkanStorageBuffer.h"

#include "VulkanContext.h"

namespace Hazel {

#define NO_STAGING 1

	VulkanStorageBuffer::VulkanStorageBuffer(uint32_t size, const StorageBufferSpecification& specification)
		: m_Specification(specification), m_Size(size)
	{
		Ref<VulkanStorageBuffer> instance = this;
		Renderer::Submit([instance]() mutable
		{
			instance->RT_Invalidate();
		});
	}

	VulkanStorageBuffer::~VulkanStorageBuffer()
	{
		Release();
	}

	void VulkanStorageBuffer::Release()
	{
		if (!m_MemoryAlloc)
			return;

		Renderer::SubmitResourceFree([buffer = m_Buffer, memoryAlloc = m_MemoryAlloc, stagingAlloc = m_StagingAlloc, stagingBuffer = m_StagingBuffer]()
		{
			VulkanAllocator allocator("StorageBuffer");
			allocator.DestroyBuffer(buffer, memoryAlloc);
			if (stagingBuffer)
				allocator.DestroyBuffer(stagingBuffer, stagingAlloc);
		});

		m_Buffer = nullptr;
		m_MemoryAlloc = nullptr;
	}

	void VulkanStorageBuffer::RT_Invalidate()
	{
		Release();

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		bufferInfo.size = m_Size;

		VulkanAllocator allocator("StorageBuffer");
		m_MemoryAlloc = allocator.AllocateBuffer(bufferInfo, m_Specification.GPUOnly ? VMA_MEMORY_USAGE_GPU_ONLY : VMA_MEMORY_USAGE_CPU_TO_GPU, m_Buffer);

		m_DescriptorInfo.buffer = m_Buffer;
		m_DescriptorInfo.offset = 0;
		m_DescriptorInfo.range = m_Size;

#if 0
		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingBufferInfo.size = m_Size;

		m_StagingAlloc = allocator.AllocateBuffer(stagingBufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, m_StagingBuffer);
#endif
	}

	void VulkanStorageBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		memcpy(m_LocalStorage, data, size);
		Ref<VulkanStorageBuffer> instance = this;
		Renderer::Submit([instance, size, offset]() mutable
		{
			instance->RT_SetData(instance->m_LocalStorage, size, offset);
		});
	}

	void VulkanStorageBuffer::RT_SetData(const void* data, uint32_t size, uint32_t offset)
	{
		ZONG_PROFILE_FUNC();
		ZONG_SCOPE_PERF("VulkanStorageBuffer::RT_SetData");

		// Cannot call SetData if GPU only
		ZONG_CORE_VERIFY(!m_Specification.GPUOnly);

#if NO_STAGING
		VulkanAllocator allocator("StorageBuffer");
		uint8_t* pData = allocator.MapMemory<uint8_t>(m_MemoryAlloc);
		memcpy(pData, data, size);
		allocator.UnmapMemory(m_MemoryAlloc);
#else
		VulkanAllocator allocator("Staging");

		{
			ZONG_SCOPE_PERF("VulkanStorageBuffer::RT_SetData - MemoryMap");
			uint8_t* pData = allocator.MapMemory<uint8_t>(m_StagingAlloc);
			memcpy(pData, data, size);
			allocator.UnmapMemory(m_StagingAlloc);
		}

		{
			ZONG_SCOPE_PERF("VulkanStorageBuffer::RT_SetData - CommandBuffer");
			VkCommandBuffer commandBuffer = VulkanContext::GetCurrentDevice()->GetCommandBuffer(true);

			VkBufferCopy copyRegion = {
				0,
				offset,
				size
			};
			vkCmdCopyBuffer(commandBuffer, m_StagingBuffer, m_Buffer, 1, &copyRegion);

			VulkanContext::GetCurrentDevice()->FlushCommandBuffer(commandBuffer);
		}
#endif
	}

	void VulkanStorageBuffer::Resize(uint32_t newSize)
	{
		m_Size = newSize;
		Ref<VulkanStorageBuffer> instance = this;
		Renderer::Submit([instance]() mutable
		{
			instance->RT_Invalidate();
		});
	}
}

#include "hzpch.h"
#include "VulkanVertexBuffer.h"

#include "VulkanContext.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Debug/Profiler.h"

namespace Hazel {

	VulkanVertexBuffer::VulkanVertexBuffer(uint64_t size, VertexBufferUsage usage)
		: m_Size(size)
	{
		m_LocalData.Allocate(size);

		Ref<VulkanVertexBuffer> instance = this;
		Renderer::Submit([instance]() mutable
		{
			auto device = VulkanContext::GetCurrentDevice();
			VulkanAllocator allocator("VertexBuffer");

			VkBufferCreateInfo vertexBufferCreateInfo = {};
			vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			vertexBufferCreateInfo.size = instance->m_Size;
			vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

			instance->m_MemoryAllocation = allocator.AllocateBuffer(vertexBufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, instance->m_VulkanBuffer);
		});
	}

	VulkanVertexBuffer::VulkanVertexBuffer(void* data, uint64_t size, VertexBufferUsage usage)
		: m_Size(size)
	{
		m_LocalData = Buffer::Copy(data, size);

		Ref<VulkanVertexBuffer> instance = this;
		Renderer::Submit([instance]() mutable
		{
			auto device = VulkanContext::GetCurrentDevice();
			VulkanAllocator allocator("VertexBuffer");

#define USE_STAGING 1
#if USE_STAGING
			VkBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = instance->m_Size;
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VkBuffer stagingBuffer;
			VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

			// Copy data to staging buffer
			uint8_t* destData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
			memcpy(destData, instance->m_LocalData.Data, instance->m_LocalData.Size);
			allocator.UnmapMemory(stagingBufferAllocation);

			VkBufferCreateInfo vertexBufferCreateInfo = {};
			vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			vertexBufferCreateInfo.size = instance->m_Size;
			vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			instance->m_MemoryAllocation = allocator.AllocateBuffer(vertexBufferCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, instance->m_VulkanBuffer);

			VkCommandBuffer copyCmd = device->GetCommandBuffer(true);

			VkBufferCopy copyRegion = {};
			copyRegion.size = instance->m_LocalData.Size;
			vkCmdCopyBuffer(
				copyCmd,
				stagingBuffer,
				instance->m_VulkanBuffer,
				1,
				&copyRegion);

			device->FlushCommandBuffer(copyCmd);

			allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
#else
			VkBufferCreateInfo vertexBufferCreateInfo = {};
			vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			vertexBufferCreateInfo.size = instance->m_Size;
			vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

			auto bufferAlloc = allocator.AllocateBuffer(vertexBufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, instance->m_VulkanBuffer);

			void* dstBuffer = allocator.MapMemory<void>(bufferAlloc);
			memcpy(dstBuffer, instance->m_LocalData.Data, instance->m_Size);
			allocator.UnmapMemory(bufferAlloc);
			
#endif
		});
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		VkBuffer buffer = m_VulkanBuffer;
		VmaAllocation allocation = m_MemoryAllocation;
		Renderer::SubmitResourceFree([buffer, allocation]()
		{
			VulkanAllocator allocator("VertexBuffer");
			allocator.DestroyBuffer(buffer, allocation);
		});

		m_LocalData.Release();
	}


	void VulkanVertexBuffer::SetData(void* buffer, uint64_t size, uint64_t offset)
	{
		HZ_PROFILE_FUNC();

		HZ_CORE_ASSERT(size <= m_LocalData.Size);
		memcpy(m_LocalData.Data, (uint8_t*)buffer + offset, size);;
		Ref<VulkanVertexBuffer> instance = this;
		Renderer::Submit([instance, size, offset]() mutable
		{
			instance->RT_SetData(instance->m_LocalData.Data, size, offset);
		});
	}

	void VulkanVertexBuffer::RT_SetData(void* buffer, uint64_t size, uint64_t offset)
	{
		HZ_PROFILE_FUNC();

		VulkanAllocator allocator("VulkanVertexBuffer");
		uint8_t* pData = allocator.MapMemory<uint8_t>(m_MemoryAllocation);
		memcpy(pData, (uint8_t*)buffer + offset, size);
		allocator.UnmapMemory(m_MemoryAllocation);
	}

}


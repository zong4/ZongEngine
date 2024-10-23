#include "hzpch.h"
#include "VulkanUniformBuffer.h"

#include "VulkanContext.h"

namespace Hazel {

	VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size)
		: m_Size(size)
	{
		m_LocalStorage = hnew uint8_t[size];

		//Ref<VulkanUniformBuffer> instance = this;
		//Renderer::Submit([instance]() mutable
		//{
			RT_Invalidate();
		//});
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		Release();
	}

	void VulkanUniformBuffer::Release()
	{
		if (!m_MemoryAlloc)
			return;

		Renderer::SubmitResourceFree([buffer = m_Buffer, memoryAlloc = m_MemoryAlloc]()
		{
			VulkanAllocator allocator("UniformBuffer");
			allocator.DestroyBuffer(buffer, memoryAlloc);
		});

		m_Buffer = nullptr;
		m_MemoryAlloc = nullptr;

		delete[] m_LocalStorage;
		m_LocalStorage = nullptr;
	}

	void VulkanUniformBuffer::RT_Invalidate()
	{
		Release();

		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.allocationSize = 0;
		allocInfo.memoryTypeIndex = 0;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.size = m_Size;

		VulkanAllocator allocator("UniformBuffer");
		m_MemoryAlloc = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, m_Buffer);

		m_DescriptorInfo.buffer = m_Buffer;
		m_DescriptorInfo.offset = 0;
		m_DescriptorInfo.range = m_Size;
	}
	
	void VulkanUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		// TODO: local storage should be potentially replaced with render thread storage
		memcpy(m_LocalStorage, data, size);
		Ref<VulkanUniformBuffer> instance = this;
		Renderer::Submit([instance, size, offset]() mutable
		{
			instance->RT_SetData(instance->m_LocalStorage, size, offset);
		});
	}

	void VulkanUniformBuffer::RT_SetData(const void* data, uint32_t size, uint32_t offset)
	{
		VulkanAllocator allocator("VulkanUniformBuffer");
		uint8_t* pData = allocator.MapMemory<uint8_t>(m_MemoryAlloc);
		memcpy(pData, (const uint8_t*)data + offset, size);
		allocator.UnmapMemory(m_MemoryAlloc);
	}


}

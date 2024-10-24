#pragma once

#include "Engine/Renderer/IndexBuffer.h"

#include "Engine/Core/Buffer.h"

#include "VulkanAllocator.h"

namespace Engine {

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(uint64_t size);
		VulkanIndexBuffer(void* data, uint64_t size = 0);
		virtual ~VulkanIndexBuffer();

		virtual void SetData(void* buffer, uint64_t size, uint64_t offset = 0) override;
		virtual void Bind() const override;

		virtual uint32_t GetCount() const override { return m_Size / sizeof(uint32_t); }

		virtual uint64_t GetSize() const override { return m_Size; }
		virtual RendererID GetRendererID() const override;

		VkBuffer GetVulkanBuffer() { return m_VulkanBuffer; }
	private:
		uint64_t m_Size = 0;
		Buffer m_LocalData;

		VkBuffer m_VulkanBuffer = nullptr;
		VmaAllocation m_MemoryAllocation;
		
	};

}

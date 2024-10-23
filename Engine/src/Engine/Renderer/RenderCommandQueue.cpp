#include "pch.h"
#include "RenderCommandQueue.h"

#define ZONG_RENDER_TRACE(...) ZONG_CORE_TRACE(__VA_ARGS__)

namespace Hazel {

	RenderCommandQueue::RenderCommandQueue()
	{
		m_CommandBuffer = hnew uint8_t[10 * 1024 * 1024]; // 10mb buffer
		m_CommandBufferPtr = m_CommandBuffer;
		memset(m_CommandBuffer, 0, 10 * 1024 * 1024);
	}

	RenderCommandQueue::~RenderCommandQueue()
	{
		delete[] m_CommandBuffer;
	}

	void* RenderCommandQueue::Allocate(RenderCommandFn fn, uint32_t size)
	{
		// TODO: alignment
		*(RenderCommandFn*)m_CommandBufferPtr = fn;
		m_CommandBufferPtr += sizeof(RenderCommandFn);

		*(uint32_t*)m_CommandBufferPtr = size;
		m_CommandBufferPtr += sizeof(uint32_t);

		void* memory = m_CommandBufferPtr;
		m_CommandBufferPtr += size;

		m_CommandCount++;
		return memory;
	}

	void RenderCommandQueue::Execute()
	{
		//ZONG_RENDER_TRACE("RenderCommandQueue::Execute -- {0} commands, {1} bytes", m_CommandCount, (m_CommandBufferPtr - m_CommandBuffer));

		byte* buffer = m_CommandBuffer;

		for (uint32_t i = 0; i < m_CommandCount; i++)
		{
			RenderCommandFn function = *(RenderCommandFn*)buffer;
			buffer += sizeof(RenderCommandFn);

			uint32_t size = *(uint32_t*)buffer;
			buffer += sizeof(uint32_t);
			function(buffer);
			buffer += size;
		}

		m_CommandBufferPtr = m_CommandBuffer;
		m_CommandCount = 0;
	}

}

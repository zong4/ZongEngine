#include "pch.h"
#include "VertexBuffer.h"

#include "Renderer.h"

#include "Engine/Platform/Vulkan/VulkanVertexBuffer.h"

#include "Engine/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<VertexBuffer> VertexBuffer::Create(void* data, uint64_t size, VertexBufferUsage usage)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanVertexBuffer>::Create(data, size, usage);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(uint64_t size, VertexBufferUsage usage)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanVertexBuffer>::Create(size, usage);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}

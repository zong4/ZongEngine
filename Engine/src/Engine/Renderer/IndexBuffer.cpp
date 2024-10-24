#include "pch.h"
#include "IndexBuffer.h"

#include "Renderer.h"

#include "Engine/Platform/Vulkan/VulkanIndexBuffer.h"

#include "Engine/Renderer/RendererAPI.h"

namespace Engine {

	Ref<IndexBuffer> IndexBuffer::Create(uint64_t size)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanIndexBuffer>::Create(size);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(void* data, uint64_t size)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanIndexBuffer>::Create(data, size);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}

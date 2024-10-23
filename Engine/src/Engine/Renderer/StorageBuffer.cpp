#include "hzpch.h"
#include "StorageBuffer.h"

#include "Engine/Platform/Vulkan/VulkanStorageBuffer.h"
#include "Engine/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<StorageBuffer> StorageBuffer::Create(uint32_t size, const StorageBufferSpecification& specification)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:     return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanStorageBuffer>::Create(size, specification);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}

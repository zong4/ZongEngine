#include "pch.h"

#include "UniformBufferSet.h"

#include "Engine/Renderer/Renderer.h"

#include "StorageBufferSet.h"

#include "Engine/Platform/Vulkan/VulkanStorageBufferSet.h"
#include "Engine/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<StorageBufferSet> StorageBufferSet::Create(const StorageBufferSpecification& specification, uint32_t size, uint32_t framesInFlight)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:   return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanStorageBufferSet>::Create(specification, size, framesInFlight);
		}

		ZONG_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
#include "pch.h"
#include "Image.h"

#include "Engine/Platform/Vulkan/VulkanImage.h"

#include "Engine/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<Image2D> Image2D::Create(const ImageSpecification& specification, Buffer buffer)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanImage2D>::Create(specification);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Image2D> Image2D::Create(const ImageSpecification& specification, const void* data)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanImage2D>::Create(specification);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<ImageView> ImageView::Create(const ImageViewSpecification& specification)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanImageView>::Create(specification);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}

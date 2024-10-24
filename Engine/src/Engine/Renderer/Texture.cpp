#include "pch.h"
#include "Texture.h"

#include "Engine/Renderer/RendererAPI.h"
#include "Engine/Platform/Vulkan/VulkanTexture.h"

#include "Engine/Renderer/RendererAPI.h"

namespace Engine {

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanTexture2D>::Create(specification);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification, const std::filesystem::path& filepath)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanTexture2D>::Create(specification, filepath);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification, Buffer imageData)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanTexture2D>::Create(specification, imageData);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<TextureCube> TextureCube::Create(const TextureSpecification& specification, Buffer imageData)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanTextureCube>::Create(specification, imageData);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
		

}

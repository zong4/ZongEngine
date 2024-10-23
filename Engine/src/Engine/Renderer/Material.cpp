#include "pch.h"
#include "Material.h"

#include "Engine/Platform/Vulkan/VulkanMaterial.h"

#include "Engine/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<Material> Material::Create(const Ref<Shader>& shader, const std::string& name)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanMaterial>::Create(shader, name);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
	
	Ref<Material> Material::Copy(const Ref<Material>& other, const std::string& name)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanMaterial>::Create(other, name);
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
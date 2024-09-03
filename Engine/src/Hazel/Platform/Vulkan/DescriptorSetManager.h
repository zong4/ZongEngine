#pragma once

#include "Hazel/Renderer/UniformBufferSet.h"
#include "Hazel/Renderer/StorageBufferSet.h"
#include "Hazel/Renderer/Image.h"
#include "Hazel/Renderer/Texture.h"

#include "Hazel/Platform/Vulkan/VulkanShader.h"

#include "vulkan/vulkan.h"

namespace Hazel {

	// TODO(Yan):
	// - Move these enums/structs to a non-API specific place
	// - Maybe rename from RenderPassXXX to DescriptorXXX or something more
	//   generic, because these are also used for compute & materials

	enum class RenderPassResourceType : uint16_t
	{
		None = 0,
		UniformBuffer,
		UniformBufferSet,
		StorageBuffer,
		StorageBufferSet,
		Texture2D,
		TextureCube,
		Image2D
	};

	enum class RenderPassInputType : uint16_t
	{
		None = 0,
		UniformBuffer,
		StorageBuffer,
		ImageSampler1D,
		ImageSampler2D,
		ImageSampler3D, // NOTE(Yan): 3D vs Cube?
		StorageImage1D,
		StorageImage2D,
		StorageImage3D
	};

	struct RenderPassInput
	{
		RenderPassResourceType Type = RenderPassResourceType::None;
		std::vector<Ref<RefCounted>> Input;

		RenderPassInput() = default;

		RenderPassInput(Ref<UniformBuffer> uniformBuffer)
			: Type(RenderPassResourceType::UniformBuffer), Input(std::vector<Ref<RefCounted>>(1, uniformBuffer))
		{
		}

		RenderPassInput(Ref<UniformBufferSet> uniformBufferSet)
			: Type(RenderPassResourceType::UniformBufferSet), Input(std::vector<Ref<RefCounted>>(1, uniformBufferSet))
		{
		}

		RenderPassInput(Ref<StorageBuffer> storageBuffer)
			: Type(RenderPassResourceType::StorageBuffer), Input(std::vector<Ref<RefCounted>>(1, storageBuffer))
		{
		}

		RenderPassInput(Ref<StorageBufferSet> storageBufferSet)
			: Type(RenderPassResourceType::StorageBufferSet), Input(std::vector<Ref<RefCounted>>(1, storageBufferSet))
		{
		}

		RenderPassInput(Ref<Texture2D> texture)
			: Type(RenderPassResourceType::Texture2D), Input(std::vector<Ref<RefCounted>>(1, texture))
		{
		}

		RenderPassInput(Ref<TextureCube> texture)
			: Type(RenderPassResourceType::TextureCube), Input(std::vector<Ref<RefCounted>>(1, texture))
		{
		}

		RenderPassInput(Ref<Image2D> image)
			: Type(RenderPassResourceType::Image2D), Input(std::vector<Ref<RefCounted>>(1, image))
		{
		}

		void Set(Ref<UniformBuffer> uniformBuffer, uint32_t index = 0)
		{
			Type = RenderPassResourceType::UniformBuffer;
			Input[index] = uniformBuffer;
		}

		void Set(Ref<UniformBufferSet> uniformBufferSet, uint32_t index = 0)
		{
			Type = RenderPassResourceType::UniformBufferSet;
			Input[index] = uniformBufferSet;
		}

		void Set(Ref<StorageBuffer> storageBuffer, uint32_t index = 0)
		{
			Type = RenderPassResourceType::StorageBuffer;
			Input[index] = storageBuffer;
		}

		void Set(Ref<StorageBufferSet> storageBufferSet, uint32_t index = 0)
		{
			Type = RenderPassResourceType::StorageBufferSet;
			Input[index] = storageBufferSet;
		}

		void Set(Ref<Texture2D> texture, uint32_t index = 0)
		{
			Type = RenderPassResourceType::Texture2D;
			Input[index] = texture;
		}

		void Set(Ref<TextureCube> texture, uint32_t index = 0)
		{
			Type = RenderPassResourceType::TextureCube;
			Input[index] = texture;
		}

		void Set(Ref<Image2D> image, uint32_t index = 0)
		{
			Type = RenderPassResourceType::Image2D;
			Input[index] = image;
		}

		void Set(Ref<ImageView> image, uint32_t index = 0)
		{
			Type = RenderPassResourceType::Image2D;
			Input[index] = image;
		}

	};

	inline bool IsCompatibleInput(RenderPassResourceType input, VkDescriptorType descriptorType)
	{
		switch (descriptorType)
		{
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			{
				return input == RenderPassResourceType::Texture2D || input == RenderPassResourceType::TextureCube || input == RenderPassResourceType::Image2D;
			}
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			{
				return input == RenderPassResourceType::Image2D;
			}
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			{
				return input == RenderPassResourceType::UniformBuffer || input == RenderPassResourceType::UniformBufferSet;
			}
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			{
				return input == RenderPassResourceType::StorageBuffer || input == RenderPassResourceType::StorageBufferSet;
			}
		}
		return false;
	}

	inline RenderPassInputType RenderPassInputTypeFromVulkanDescriptorType(VkDescriptorType descriptorType)
	{
		switch (descriptorType)
		{
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				return RenderPassInputType::ImageSampler2D;
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				return RenderPassInputType::StorageImage2D;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				return RenderPassInputType::UniformBuffer;
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				return RenderPassInputType::StorageBuffer;
		}

		HZ_CORE_ASSERT(false);
		return RenderPassInputType::None;
	}

	struct RenderPassInputDeclaration
	{
		RenderPassInputType Type = RenderPassInputType::None;
		uint32_t Set = 0;
		uint32_t Binding = 0;
		uint32_t Count = 0;
		std::string Name;
	};

	struct DescriptorSetManagerSpecification
	{
		Ref<VulkanShader> Shader;
		std::string DebugName;

		// Which descriptor sets should be managed
		uint32_t StartSet = 0, EndSet = 3;

		bool DefaultResources = false;
	};

	struct DescriptorSetManager
	{
		//
		// Input Resources (map of set->binding->resource)
		// 
		// Invalidated input resources will attempt to be assigned on Renderer::BeginRenderPass
		// This is useful for resources that may not exist at RenderPass creation but will be
		// present during actual rendering
		std::map<uint32_t, std::map<uint32_t, RenderPassInput>> InputResources;
		std::map<uint32_t, std::map<uint32_t, RenderPassInput>> InvalidatedInputResources;
		std::map<std::string, RenderPassInputDeclaration> InputDeclarations;

		// Per-frame in flight
		std::vector<std::vector<VkDescriptorSet>> m_DescriptorSets;

		struct WriteDescriptor
		{
			VkWriteDescriptorSet WriteDescriptorSet{};
			std::vector<void*> ResourceHandles;
		};
		std::vector<std::map<uint32_t, std::map<uint32_t, WriteDescriptor>>> WriteDescriptorMap;

		DescriptorSetManager() = default;
		DescriptorSetManager(const DescriptorSetManager& other);
		DescriptorSetManager(const DescriptorSetManagerSpecification& specification);
		static DescriptorSetManager Copy(const DescriptorSetManager& other);

		void SetInput(std::string_view name, Ref<UniformBufferSet> uniformBufferSet);
		void SetInput(std::string_view name, Ref<UniformBuffer> uniformBuffer);
		void SetInput(std::string_view name, Ref<StorageBufferSet> storageBufferSet);
		void SetInput(std::string_view name, Ref<StorageBuffer> storageBuffer);
		void SetInput(std::string_view name, Ref<Texture2D> texture, uint32_t index = 0);
		void SetInput(std::string_view name, Ref<TextureCube> textureCube);
		void SetInput(std::string_view name, Ref<Image2D> image);
		void SetInput(std::string_view name, Ref<ImageView> image);

		template<typename T>
		Ref<T> GetInput(std::string_view name)
		{
			const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
			if (decl)
			{
				auto setIt = InputResources.find(decl->Set);
				if (setIt != InputResources.end())
				{
					auto resourceIt = setIt->second.find(decl->Binding);
					if (resourceIt != setIt->second.end())
						return resourceIt->second.Input[0].As<T>();
				}
			}
			return nullptr;
		}

		bool IsInvalidated(uint32_t set, uint32_t binding) const;
		bool Validate();
		void Bake();

		std::set<uint32_t> HasBufferSets() const;
		void InvalidateAndUpdate();

		VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }
		bool HasDescriptorSets() const;
		uint32_t GetFirstSetIndex() const;
		const std::vector<VkDescriptorSet>& GetDescriptorSets(uint32_t frameIndex) const;
		bool IsInputValid(std::string_view name) const;
		const RenderPassInputDeclaration* GetInputDeclaration(std::string_view name) const;
	private:
		void Init();
	private:
		DescriptorSetManagerSpecification m_Specification;
		VkDescriptorPool m_DescriptorPool = nullptr;
	};

}

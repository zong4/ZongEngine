#include "hzpch.h"
#include "DescriptorSetManager.h"

#include "Engine/Renderer/Renderer.h"

#include "VulkanAPI.h"
#include "VulkanStorageBuffer.h"
#include "VulkanStorageBufferSet.h"
#include "VulkanUniformBuffer.h"
#include "VulkanUniformBufferSet.h"
#include "VulkanTexture.h"

namespace Hazel {
	
	namespace Utils {

		inline RenderPassResourceType GetDefaultResourceType(VkDescriptorType descriptorType)
		{
			switch (descriptorType)
			{
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
					return RenderPassResourceType::Texture2D;
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					return RenderPassResourceType::Image2D;
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
					return RenderPassResourceType::UniformBuffer;
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
					return RenderPassResourceType::StorageBuffer;
			}

			HZ_CORE_ASSERT(false);
			return RenderPassResourceType::None;
		}

	}

	DescriptorSetManager::DescriptorSetManager(const DescriptorSetManagerSpecification& specification)
		: m_Specification(specification)
	{
		Init();
	}

	DescriptorSetManager::DescriptorSetManager(const DescriptorSetManager& other)
		: m_Specification(other.m_Specification)
	{
		Init();
		InputResources = other.InputResources;
		Bake();
	}

	DescriptorSetManager DescriptorSetManager::Copy(const DescriptorSetManager& other)
	{
		DescriptorSetManager result(other);
		return result;
	}

	void DescriptorSetManager::Init()
	{
		const auto& shaderDescriptorSets = m_Specification.Shader->GetShaderDescriptorSets();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		WriteDescriptorMap.resize(framesInFlight);

		for (uint32_t set = m_Specification.StartSet; set <= m_Specification.EndSet; set++)
		{
			if (set >= shaderDescriptorSets.size())
				break;

			const auto& shaderDescriptor = shaderDescriptorSets[set];
			for (auto&& [bname, wd] : shaderDescriptor.WriteDescriptorSets)
			{
				// NOTE(Emily): This is a hack to fix a bad input decl name
				//				Coming from somewhere.
				const char* broken = strrchr(bname.c_str(), '.');
				std::string name = broken ? broken + 1 : bname;

				uint32_t binding = wd.dstBinding;
				RenderPassInputDeclaration& inputDecl = InputDeclarations[name];
				inputDecl.Type = RenderPassInputTypeFromVulkanDescriptorType(wd.descriptorType);
				inputDecl.Set = set;
				inputDecl.Binding = binding;
				inputDecl.Name = name;
				inputDecl.Count = wd.descriptorCount;

				// Insert default resources (useful for materials)
				if (m_Specification.DefaultResources || true)
				{
					// Create RenderPassInput
					RenderPassInput& input = InputResources[set][binding];
					input.Input.resize(wd.descriptorCount);
					input.Type = Utils::GetDefaultResourceType(wd.descriptorType);

					// Set default textures
					if (inputDecl.Type == RenderPassInputType::ImageSampler2D)
					{
						for (size_t i = 0; i < input.Input.size(); i++)
						{
							input.Input[i] = Renderer::GetWhiteTexture();
						}
					}
					else if (inputDecl.Type == RenderPassInputType::ImageSampler3D)
					{
						for (size_t i = 0; i < input.Input.size(); i++)
							input.Input[i] = Renderer::GetBlackCubeTexture();
					}
				}

				for (uint32_t frameIndex = 0; frameIndex < framesInFlight; frameIndex++)
					WriteDescriptorMap[frameIndex][set][binding] = { wd, std::vector<void*>(wd.descriptorCount) };

				if (shaderDescriptor.ImageSamplers.find(binding) != shaderDescriptor.ImageSamplers.end())
				{
					auto& imageSampler = shaderDescriptor.ImageSamplers.at(binding);
					uint32_t dimension = imageSampler.Dimension;
					if (wd.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || wd.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					{
						switch (dimension)
						{
							case 1:
								inputDecl.Type = RenderPassInputType::ImageSampler1D;
								break;
							case 2:
								inputDecl.Type = RenderPassInputType::ImageSampler2D;
								break;
							case 3:
								inputDecl.Type = RenderPassInputType::ImageSampler3D;
								break;
						}
					}
					else if (wd.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
					{
						switch (dimension)
						{
							case 1:
								inputDecl.Type = RenderPassInputType::StorageImage1D;
								break;
							case 2:
								inputDecl.Type = RenderPassInputType::StorageImage2D;
								break;
							case 3:
								inputDecl.Type = RenderPassInputType::StorageImage3D;
								break;
						}

					}
				}
			}
		}
	}

	void DescriptorSetManager::SetInput(std::string_view name, Ref<UniformBufferSet> uniformBufferSet)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		if (decl)
			InputResources.at(decl->Set).at(decl->Binding).Set(uniformBufferSet);
		else
			HZ_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.DebugName, name);
	}

	void DescriptorSetManager::SetInput(std::string_view name, Ref<UniformBuffer> uniformBuffer)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		if (decl)
			InputResources.at(decl->Set).at(decl->Binding).Set(uniformBuffer);
		else
			HZ_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.DebugName, name);
	}

	void DescriptorSetManager::SetInput(std::string_view name, Ref<StorageBufferSet> storageBufferSet)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		if (decl)
			InputResources.at(decl->Set).at(decl->Binding).Set(storageBufferSet);
		else
			HZ_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.DebugName, name);
	}

	void DescriptorSetManager::SetInput(std::string_view name, Ref<StorageBuffer> storageBuffer)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		if (decl)
			InputResources.at(decl->Set).at(decl->Binding).Set(storageBuffer);
		else
			HZ_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.DebugName, name);
	}

	void DescriptorSetManager::SetInput(std::string_view name, Ref<Texture2D> texture, uint32_t index)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		HZ_CORE_VERIFY(index < decl->Count);
		if (decl)
			InputResources.at(decl->Set).at(decl->Binding).Set(texture, index);
		else
			HZ_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.DebugName, name);
	}

	void DescriptorSetManager::SetInput(std::string_view name, Ref<TextureCube> textureCube)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		if (decl)
			InputResources.at(decl->Set).at(decl->Binding).Set(textureCube);
		else
			HZ_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.DebugName, name);
	}

	void DescriptorSetManager::SetInput(std::string_view name, Ref<Image2D> image)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		if (decl)
			InputResources.at(decl->Set).at(decl->Binding).Set(image);
		else
			HZ_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.DebugName, name);
	}

	void DescriptorSetManager::SetInput(std::string_view name, Ref<ImageView> image)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		if (decl)
			InputResources.at(decl->Set).at(decl->Binding).Set(image);
		else
			HZ_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.DebugName, name);
	}

	bool DescriptorSetManager::IsInvalidated(uint32_t set, uint32_t binding) const
	{
		if (InvalidatedInputResources.find(set) != InvalidatedInputResources.end())
		{
			const auto& resources = InvalidatedInputResources.at(set);
			return resources.find(binding) != resources.end();
		}

		return false;
	}

	

	std::set<uint32_t> DescriptorSetManager::HasBufferSets() const
	{
		// Find all descriptor sets that have either UniformBufferSet or StorageBufferSet descriptors
		std::set<uint32_t> sets;

		for (const auto& [set, resources] : InputResources)
		{
			for (const auto& [binding, input] : resources)
			{
				if (input.Type == RenderPassResourceType::UniformBufferSet || input.Type == RenderPassResourceType::StorageBufferSet)
				{
					sets.insert(set);
					break;
				}
			}
		}
		return sets;
	}


	bool DescriptorSetManager::Validate()
	{
		// Go through pipeline requirements to make sure we have all required resource
		const auto& shaderDescriptorSets = m_Specification.Shader->GetShaderDescriptorSets();

		// Nothing to validate, pipeline only contains material inputs
		//if (shaderDescriptorSets.size() < 2)
		//	return true;

		for (uint32_t set = m_Specification.StartSet; set <= m_Specification.EndSet; set++)
		{
			if (set >= shaderDescriptorSets.size())
				break;

			// No descriptors in this set
			if (!shaderDescriptorSets[set])
				continue;

			if (InputResources.find(set) == InputResources.end())
			{
				HZ_CORE_ERROR_TAG("Renderer", "[RenderPass ({})] No input resources for Set {}", m_Specification.DebugName, set);
				return false;
			}

			const auto& setInputResources = InputResources.at(set);

			const auto& shaderDescriptor = shaderDescriptorSets[set];
			for (auto&& [name, wd] : shaderDescriptor.WriteDescriptorSets)
			{
				uint32_t binding = wd.dstBinding;
				if (setInputResources.find(binding) == setInputResources.end())
				{
					HZ_CORE_ERROR_TAG("Renderer", "[RenderPass ({})] No input resource for {}.{}", m_Specification.DebugName, set, binding);
					HZ_CORE_ERROR_TAG("Renderer", "[RenderPass ({})] Required resource is {} ({})", m_Specification.DebugName, name, wd.descriptorType);
					return false;
				}

				const auto& resource = setInputResources.at(binding);
				if (!IsCompatibleInput(resource.Type, wd.descriptorType))
				{
					HZ_CORE_ERROR_TAG("Renderer", "[RenderPass ({})] Required resource is wrong type! {} but needs {}", m_Specification.DebugName, resource.Type, wd.descriptorType);
					return false;
				}

				if (resource.Type != RenderPassResourceType::Image2D && resource.Input[0] == nullptr)
				{
					HZ_CORE_ERROR_TAG("Renderer", "[RenderPass ({})] Resource is null! {} ({}.{})", m_Specification.DebugName, name, set, binding);
					return false;
				}
			}
		}

		// All resources present
		return true;
	}

	void DescriptorSetManager::Bake()
	{
		// Make sure all resources are present and we can properly bake
		if (!Validate())
		{
			HZ_CORE_ERROR_TAG("Renderer", "[RenderPass] Bake - Validate failed! {}", m_Specification.DebugName);
			return;
		}
		
		// If valid, we can create descriptor sets

		// Create Descriptor Pool
		VkDescriptorPoolSize poolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 10 * 3; // frames in flight should partially determine this
		poolInfo.poolSizeCount = 10;
		poolInfo.pPoolSizes = poolSizes;

		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool));

		auto bufferSets = HasBufferSets();
		bool perFrameInFlight = !bufferSets.empty();
		perFrameInFlight = true; // always
		uint32_t descriptorSetCount = Renderer::GetConfig().FramesInFlight;
		if (!perFrameInFlight)
			descriptorSetCount = 1;

		if (m_DescriptorSets.size() < 1)
		{
			for (uint32_t i = 0; i < descriptorSetCount; i++)
				m_DescriptorSets.emplace_back();
		}

		for (auto& descriptorSet : m_DescriptorSets)
			descriptorSet.clear();

		for (const auto& [set, setData] : InputResources)
		{
			uint32_t descriptorCountInSet = bufferSets.find(set) != bufferSets.end() ? descriptorSetCount : 1;
			for (uint32_t frameIndex = 0; frameIndex < descriptorSetCount; frameIndex++)
			{
				VkDescriptorSetLayout dsl = m_Specification.Shader->GetDescriptorSetLayout(set);
				VkDescriptorSetAllocateInfo descriptorSetAllocInfo = Vulkan::DescriptorSetAllocInfo(&dsl);
				descriptorSetAllocInfo.descriptorPool = m_DescriptorPool;
				VkDescriptorSet descriptorSet = nullptr;
				VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSet));

				m_DescriptorSets[frameIndex].emplace_back(descriptorSet);

				auto& writeDescriptorMap = WriteDescriptorMap[frameIndex].at(set);
				std::vector<std::vector<VkDescriptorImageInfo>> imageInfoStorage;
				uint32_t imageInfoStorageIndex = 0;

				for (const auto& [binding, input] : setData)
				{
					auto& storedWriteDescriptor = writeDescriptorMap.at(binding);

					VkWriteDescriptorSet& writeDescriptor = storedWriteDescriptor.WriteDescriptorSet;
					writeDescriptor.dstSet = descriptorSet;

					switch (input.Type)
					{
						case RenderPassResourceType::UniformBuffer:
						{
							Ref<VulkanUniformBuffer> buffer = input.Input[0].As<VulkanUniformBuffer>();
							writeDescriptor.pBufferInfo = &buffer->GetDescriptorBufferInfo();
							storedWriteDescriptor.ResourceHandles[0] = writeDescriptor.pBufferInfo->buffer;

							// Defer if resource doesn't exist
							if (writeDescriptor.pBufferInfo->buffer == nullptr)
								InvalidatedInputResources[set][binding] = input;

							break;
						}
						case RenderPassResourceType::UniformBufferSet:
						{
							Ref<UniformBufferSet> buffer = input.Input[0].As<UniformBufferSet>();
							// TODO: replace 0 with current frame in flight (i.e. create bindings for all frames)
							writeDescriptor.pBufferInfo = &buffer->Get(frameIndex).As<VulkanUniformBuffer>()->GetDescriptorBufferInfo();
							storedWriteDescriptor.ResourceHandles[0] = writeDescriptor.pBufferInfo->buffer;

							// Defer if resource doesn't exist
							if (writeDescriptor.pBufferInfo->buffer == nullptr)
								InvalidatedInputResources[set][binding] = input;

							break;
						}
						case RenderPassResourceType::StorageBuffer:
						{
							Ref<VulkanStorageBuffer> buffer = input.Input[0].As<VulkanStorageBuffer>();
							writeDescriptor.pBufferInfo = &buffer->GetDescriptorBufferInfo();
							storedWriteDescriptor.ResourceHandles[0] = writeDescriptor.pBufferInfo->buffer;

							// Defer if resource doesn't exist
							if (writeDescriptor.pBufferInfo->buffer == nullptr)
								InvalidatedInputResources[set][binding] = input;

							break;
						}
						case RenderPassResourceType::StorageBufferSet:
						{
							Ref<StorageBufferSet> buffer = input.Input[0].As<StorageBufferSet>();
							// TODO: replace 0 with current frame in flight (i.e. create bindings for all frames)
							writeDescriptor.pBufferInfo = &buffer->Get(frameIndex).As<VulkanStorageBuffer>()->GetDescriptorBufferInfo();
							storedWriteDescriptor.ResourceHandles[0] = writeDescriptor.pBufferInfo->buffer;

							// Defer if resource doesn't exist
							if (writeDescriptor.pBufferInfo->buffer == nullptr)
								InvalidatedInputResources[set][binding] = input;

							break;
						}
						case RenderPassResourceType::Texture2D:
						{
							if (input.Input.size() > 1)
							{
								imageInfoStorage.emplace_back(input.Input.size());
								for (size_t i = 0; i < input.Input.size(); i++)
								{
									Ref<VulkanTexture2D> texture = input.Input[i].As<VulkanTexture2D>();
									imageInfoStorage[imageInfoStorageIndex][i] = texture->GetDescriptorInfoVulkan();

								}
								writeDescriptor.pImageInfo = imageInfoStorage[imageInfoStorageIndex].data();
								imageInfoStorageIndex++;
							}
							else
							{
								Ref<VulkanTexture2D> texture = input.Input[0].As<VulkanTexture2D>();
								writeDescriptor.pImageInfo = &texture->GetDescriptorInfoVulkan();
							}
							storedWriteDescriptor.ResourceHandles[0] = writeDescriptor.pImageInfo->imageView;

							// Defer if resource doesn't exist
							if (writeDescriptor.pImageInfo->imageView == nullptr)
								InvalidatedInputResources[set][binding] = input;

							break;
						}
						case RenderPassResourceType::TextureCube:
						{
							Ref<VulkanTextureCube> texture = input.Input[0].As<VulkanTextureCube>();
							writeDescriptor.pImageInfo = &texture->GetDescriptorInfoVulkan();
							storedWriteDescriptor.ResourceHandles[0] = writeDescriptor.pImageInfo->imageView;

							// Defer if resource doesn't exist
							if (writeDescriptor.pImageInfo->imageView == nullptr)
								InvalidatedInputResources[set][binding] = input;

							break;
						}
						case RenderPassResourceType::Image2D:
						{
							Ref<RendererResource> image = input.Input[0].As<RendererResource>();
							// Defer if resource doesn't exist
							if (image == nullptr)
							{
								InvalidatedInputResources[set][binding] = input;
								break;
							}

							writeDescriptor.pImageInfo = (VkDescriptorImageInfo*)image->GetDescriptorInfo();
							storedWriteDescriptor.ResourceHandles[0] = writeDescriptor.pImageInfo->imageView;

							// Defer if resource doesn't exist
							if (writeDescriptor.pImageInfo->imageView == nullptr)
								InvalidatedInputResources[set][binding] = input;

							break;
						}
					}
				}

				std::vector<VkWriteDescriptorSet> writeDescriptors;
				for (auto&& [binding, writeDescriptor] : writeDescriptorMap)
				{
					// Include if valid, otherwise defer (these will be resolved if possible at Prepare stage)
					if (!IsInvalidated(set, binding))
						writeDescriptors.emplace_back(writeDescriptor.WriteDescriptorSet);
				}

				if (!writeDescriptors.empty())
				{
					HZ_CORE_INFO_TAG("Renderer", "Render pass update {} descriptors in set {}", writeDescriptors.size(), set);
					vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
				}
			}
		}

	}

	void DescriptorSetManager::InvalidateAndUpdate()
	{
		HZ_PROFILE_FUNC();
		HZ_SCOPE_PERF("DescriptorSetManager::InvalidateAndUpdate");

		uint32_t currentFrameIndex = Renderer::RT_GetCurrentFrameIndex();

		// Check for invalidated resources
		for (const auto& [set, inputs] : InputResources)
		{
			for (const auto& [binding, input] : inputs)
			{
				switch (input.Type)
				{
					case RenderPassResourceType::UniformBuffer:
					{
						//for (uint32_t frameIndex = 0; frameIndex < (uint32_t)WriteDescriptorMap.size(); frameIndex++)
						{
							const VkDescriptorBufferInfo& bufferInfo = input.Input[0].As<VulkanUniformBuffer>()->GetDescriptorBufferInfo();
							if (bufferInfo.buffer != WriteDescriptorMap[currentFrameIndex].at(set).at(binding).ResourceHandles[0])
							{
								InvalidatedInputResources[set][binding] = input;
								break;
							}
						}
						break;
					}
					case RenderPassResourceType::UniformBufferSet:
					{
						//for (uint32_t frameIndex = 0; frameIndex < (uint32_t)WriteDescriptorMap.size(); frameIndex++)
						{
							const VkDescriptorBufferInfo& bufferInfo = input.Input[0].As<VulkanUniformBufferSet>()->Get(currentFrameIndex).As<VulkanUniformBuffer>()->GetDescriptorBufferInfo();
							if (bufferInfo.buffer != WriteDescriptorMap[currentFrameIndex].at(set).at(binding).ResourceHandles[0])
							{
								InvalidatedInputResources[set][binding] = input;
								break;
							}
						}
						break;
					}
					case RenderPassResourceType::StorageBuffer:
					{

						//for (uint32_t frameIndex = 0; frameIndex < (uint32_t)WriteDescriptorMap.size(); frameIndex++)
						{
							const VkDescriptorBufferInfo& bufferInfo = input.Input[0].As<VulkanStorageBuffer>()->GetDescriptorBufferInfo();
							if (bufferInfo.buffer != WriteDescriptorMap[currentFrameIndex].at(set).at(binding).ResourceHandles[0])
							{
								InvalidatedInputResources[set][binding] = input;
								break;
							}
						}
						break;
					}
					case RenderPassResourceType::StorageBufferSet:
					{
						//for (uint32_t frameIndex = 0; frameIndex < (uint32_t)WriteDescriptorMap.size(); frameIndex++)
						{
							const VkDescriptorBufferInfo& bufferInfo = input.Input[0].As<VulkanStorageBufferSet>()->Get(currentFrameIndex).As<VulkanStorageBuffer>()->GetDescriptorBufferInfo();
							if (bufferInfo.buffer != WriteDescriptorMap[currentFrameIndex].at(set).at(binding).ResourceHandles[0])
							{
								InvalidatedInputResources[set][binding] = input;
								break;
							}
						}
						break;
					}
					case RenderPassResourceType::Texture2D:
					{
						for (size_t i = 0; i < input.Input.size(); i++)
						{
							const VkDescriptorImageInfo& imageInfo = input.Input[i].As<VulkanTexture2D>()->GetDescriptorInfoVulkan();
							if (imageInfo.imageView != WriteDescriptorMap[currentFrameIndex].at(set).at(binding).ResourceHandles[i])
							{
								InvalidatedInputResources[set][binding] = input;
								break;
							}
						}
						break;
					}
					case RenderPassResourceType::TextureCube:
					{
						//for (uint32_t frameIndex = 0; frameIndex < (uint32_t)WriteDescriptorMap.size(); frameIndex++)
						{
							const VkDescriptorImageInfo& imageInfo = input.Input[0].As<VulkanTextureCube>()->GetDescriptorInfoVulkan();
							if (imageInfo.imageView != WriteDescriptorMap[currentFrameIndex].at(set).at(binding).ResourceHandles[0])
							{
								InvalidatedInputResources[set][binding] = input;
								break;
							}
						}
						break;
					}
					case RenderPassResourceType::Image2D:
					{
						//for (uint32_t frameIndex = 0; frameIndex < (uint32_t)WriteDescriptorMap.size(); frameIndex++)
						{
							const VkDescriptorImageInfo& imageInfo = *(VkDescriptorImageInfo*)input.Input[0].As<RendererResource>()->GetDescriptorInfo();
							if (imageInfo.imageView != WriteDescriptorMap[currentFrameIndex].at(set).at(binding).ResourceHandles[0])
							{
								InvalidatedInputResources[set][binding] = input;
								break;
							}
						}
						break;
					}
				}
			}
		}

		// Nothing to do
		if (InvalidatedInputResources.empty())
			return;

		auto bufferSets = HasBufferSets();
		bool perFrameInFlight = !bufferSets.empty();
		perFrameInFlight = true; // always
		uint32_t descriptorSetCount = Renderer::GetConfig().FramesInFlight;
		if (!perFrameInFlight)
			descriptorSetCount = 1;


		// TODO(Yan): handle these if they fail (although Vulkan will probably give us a validation error if they do anyway)
		for (const auto& [set, setData] : InvalidatedInputResources)
		{
			uint32_t descriptorCountInSet = bufferSets.find(set) != bufferSets.end() ? descriptorSetCount : 1;
			//for (uint32_t frameIndex = currentFrameIndex; frameIndex < descriptorSetCount; frameIndex++)
			uint32_t frameIndex = perFrameInFlight ? currentFrameIndex : 0;
			{
				// Go through every resource here and call vkUpdateDescriptorSets with write descriptors
				// If we don't have valid buffers/images to bind to here, that's an error and needs to be
				// probably handled by putting in some error resources, otherwise we'll crash
				std::vector<VkWriteDescriptorSet> writeDescriptorsToUpdate;
				writeDescriptorsToUpdate.reserve(setData.size());
				std::vector<std::vector<VkDescriptorImageInfo>> imageInfoStorage;
				uint32_t imageInfoStorageIndex = 0;
				for (const auto& [binding, input] : setData)
				{
					// Update stored write descriptor
					auto& wd = WriteDescriptorMap[frameIndex].at(set).at(binding);
					VkWriteDescriptorSet& writeDescriptor = wd.WriteDescriptorSet;
					switch (input.Type)
					{
						case RenderPassResourceType::UniformBuffer:
						{
							Ref<VulkanUniformBuffer> buffer = input.Input[0].As<VulkanUniformBuffer>();
							writeDescriptor.pBufferInfo = &buffer->GetDescriptorBufferInfo();
							wd.ResourceHandles[0] = writeDescriptor.pBufferInfo->buffer;
							break;
						}
						case RenderPassResourceType::UniformBufferSet:
						{
							Ref<UniformBufferSet> buffer = input.Input[0].As<UniformBufferSet>();
							writeDescriptor.pBufferInfo = &buffer->Get(frameIndex).As<VulkanUniformBuffer>()->GetDescriptorBufferInfo();
							wd.ResourceHandles[0] = writeDescriptor.pBufferInfo->buffer;
							break;
						}
						case RenderPassResourceType::StorageBuffer:
						{
							Ref<VulkanStorageBuffer> buffer = input.Input[0].As<VulkanStorageBuffer>();
							writeDescriptor.pBufferInfo = &buffer->GetDescriptorBufferInfo();
							wd.ResourceHandles[0] = writeDescriptor.pBufferInfo->buffer;
							break;
						}
						case RenderPassResourceType::StorageBufferSet:
						{
							Ref<StorageBufferSet> buffer = input.Input[0].As<StorageBufferSet>();
							writeDescriptor.pBufferInfo = &buffer->Get(frameIndex).As<VulkanStorageBuffer>()->GetDescriptorBufferInfo();
							wd.ResourceHandles[0] = writeDescriptor.pBufferInfo->buffer;
							break;
						}
						case RenderPassResourceType::Texture2D:
						{

							if (input.Input.size() > 1)
							{
								imageInfoStorage.emplace_back(input.Input.size());
								for (size_t i = 0; i < input.Input.size(); i++)
								{
									Ref<VulkanTexture2D> texture = input.Input[i].As<VulkanTexture2D>();
									imageInfoStorage[imageInfoStorageIndex][i] = texture->GetDescriptorInfoVulkan();
									wd.ResourceHandles[i] = imageInfoStorage[imageInfoStorageIndex][i].imageView;
								}
								writeDescriptor.pImageInfo = imageInfoStorage[imageInfoStorageIndex].data();
								imageInfoStorageIndex++;
							}
							else
							{
								Ref<VulkanTexture2D> texture = input.Input[0].As<VulkanTexture2D>();
								writeDescriptor.pImageInfo = &texture->GetDescriptorInfoVulkan();
								wd.ResourceHandles[0] = writeDescriptor.pImageInfo->imageView;
							}

							break;
						}
						case RenderPassResourceType::TextureCube:
						{
							Ref<VulkanTextureCube> texture = input.Input[0].As<VulkanTextureCube>();
							writeDescriptor.pImageInfo = &texture->GetDescriptorInfoVulkan();
							wd.ResourceHandles[0] = writeDescriptor.pImageInfo->imageView;
							break;
						}
						case RenderPassResourceType::Image2D:
						{
							Ref<RendererResource> image = input.Input[0].As<RendererResource>();
							writeDescriptor.pImageInfo = (VkDescriptorImageInfo*)image->GetDescriptorInfo();
							HZ_CORE_VERIFY(writeDescriptor.pImageInfo->imageView);
							wd.ResourceHandles[0] = writeDescriptor.pImageInfo->imageView;
							break;
						}
					}
					writeDescriptorsToUpdate.emplace_back(writeDescriptor);
				}
				// HZ_CORE_INFO_TAG("Renderer", "RenderPass::Prepare ({}) - updating {} descriptors in set {} (frameIndex={})", m_Specification.DebugName, writeDescriptorsToUpdate.size(), set, frameIndex);
				HZ_CORE_INFO_TAG("Renderer", "DescriptorSetManager::InvalidateAndUpdate ({}) - updating {} descriptors in set {} (frameIndex={})", m_Specification.DebugName, writeDescriptorsToUpdate.size(), set, frameIndex);
				VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
				vkUpdateDescriptorSets(device, (uint32_t)writeDescriptorsToUpdate.size(), writeDescriptorsToUpdate.data(), 0, nullptr);
			}
		}

		InvalidatedInputResources.clear();
	}

	bool DescriptorSetManager::HasDescriptorSets() const
	{
		return !m_DescriptorSets.empty() && !m_DescriptorSets[0].empty();
	}

	uint32_t DescriptorSetManager::GetFirstSetIndex() const
	{
		if (InputResources.empty())
			return UINT32_MAX;

		// Return first key (key == descriptor set index)
		return InputResources.begin()->first;
	}

	const std::vector<VkDescriptorSet>& DescriptorSetManager::GetDescriptorSets(uint32_t frameIndex) const
	{
		HZ_CORE_ASSERT(!m_DescriptorSets.empty());

		if (frameIndex > 0 && m_DescriptorSets.size() == 1)
			return m_DescriptorSets[0]; // Frame index is irrelevant for this type of render pass

		return m_DescriptorSets[frameIndex];
	}

	bool DescriptorSetManager::IsInputValid(std::string_view name) const
	{
		std::string nameStr(name);
		return InputDeclarations.find(nameStr) != InputDeclarations.end();
	}

	const RenderPassInputDeclaration* DescriptorSetManager::GetInputDeclaration(std::string_view name) const
	{
		std::string nameStr(name);
		if (InputDeclarations.find(nameStr) == InputDeclarations.end())
			return nullptr;

		const RenderPassInputDeclaration& decl = InputDeclarations.at(nameStr);
		return &decl;
	}



}

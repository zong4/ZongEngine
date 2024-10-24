#pragma once

#include "vulkan/vulkan.h"
#include "VulkanAllocator.h"

#include "Engine/Serialization/StreamReader.h"
#include "Engine/Serialization/StreamWriter.h"

#include <string>

namespace Engine {

	namespace ShaderResource {

		struct UniformBuffer
		{
			VkDescriptorBufferInfo Descriptor;
			uint32_t Size = 0;
			uint32_t BindingPoint = 0;
			std::string Name;
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

			static void Serialize(StreamWriter* serializer, const UniformBuffer& instance)
			{
				serializer->WriteRaw(instance.Descriptor);
				serializer->WriteRaw(instance.Size);
				serializer->WriteRaw(instance.BindingPoint);
				serializer->WriteString(instance.Name);
				serializer->WriteRaw(instance.ShaderStage);
			}

			static void Deserialize(StreamReader* deserializer, UniformBuffer& instance)
			{
				deserializer->ReadRaw(instance.Descriptor);
				deserializer->ReadRaw(instance.Size);
				deserializer->ReadRaw(instance.BindingPoint);
				deserializer->ReadString(instance.Name);
				deserializer->ReadRaw(instance.ShaderStage);
			}
		};

		struct StorageBuffer
		{
			VmaAllocation MemoryAlloc = nullptr;
			VkDescriptorBufferInfo Descriptor;
			uint32_t Size = 0;
			uint32_t BindingPoint = 0;
			std::string Name;
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

			static void Serialize(StreamWriter* serializer, const StorageBuffer& instance)
			{
				serializer->WriteRaw(instance.Descriptor);
				serializer->WriteRaw(instance.Size);
				serializer->WriteRaw(instance.BindingPoint);
				serializer->WriteString(instance.Name);
				serializer->WriteRaw(instance.ShaderStage);
			}

			static void Deserialize(StreamReader* deserializer, StorageBuffer& instance)
			{
				deserializer->ReadRaw(instance.Descriptor);
				deserializer->ReadRaw(instance.Size);
				deserializer->ReadRaw(instance.BindingPoint);
				deserializer->ReadString(instance.Name);
				deserializer->ReadRaw(instance.ShaderStage);
			}
		};

		struct ImageSampler
		{
			uint32_t BindingPoint = 0;
			uint32_t DescriptorSet = 0;
			uint32_t Dimension = 0;
			uint32_t ArraySize = 0;
			std::string Name;
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

			static void Serialize(StreamWriter* serializer, const ImageSampler& instance)
			{
				serializer->WriteRaw(instance.BindingPoint);
				serializer->WriteRaw(instance.DescriptorSet);
				serializer->WriteRaw(instance.Dimension);
				serializer->WriteRaw(instance.ArraySize);
				serializer->WriteString(instance.Name);
				serializer->WriteRaw(instance.ShaderStage);
			}

			static void Deserialize(StreamReader* deserializer, ImageSampler& instance)
			{
				deserializer->ReadRaw(instance.BindingPoint);
				deserializer->ReadRaw(instance.DescriptorSet);
				deserializer->ReadRaw(instance.Dimension);
				deserializer->ReadRaw(instance.ArraySize);
				deserializer->ReadString(instance.Name);
				deserializer->ReadRaw(instance.ShaderStage);
			}
		};

		struct PushConstantRange
		{
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			uint32_t Offset = 0;
			uint32_t Size = 0;

			static void Serialize(StreamWriter* writer, const PushConstantRange& range) { writer->WriteRaw(range); }
			static void Deserialize(StreamReader* reader, PushConstantRange& range) { reader->ReadRaw(range); }
		};

		struct ShaderDescriptorSet
		{
			std::unordered_map<uint32_t, UniformBuffer> UniformBuffers;
			std::unordered_map<uint32_t, StorageBuffer> StorageBuffers;
			std::unordered_map<uint32_t, ImageSampler> ImageSamplers;
			std::unordered_map<uint32_t, ImageSampler> StorageImages;
			std::unordered_map<uint32_t, ImageSampler> SeparateTextures; // Not really an image sampler.
			std::unordered_map<uint32_t, ImageSampler> SeparateSamplers;

			std::unordered_map<std::string, VkWriteDescriptorSet> WriteDescriptorSets;

			operator bool() const { return !(StorageBuffers.empty() && UniformBuffers.empty() && ImageSamplers.empty() && StorageImages.empty() && SeparateTextures.empty() && SeparateSamplers.empty()); }
		};

	}
}

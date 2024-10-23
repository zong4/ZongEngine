#include "hzpch.h"
#include "ShaderPack.h"

#include "Engine/Core/Hash.h"

#include "Engine/Serialization/FileStream.h"

#include "Engine/Platform/Vulkan/VulkanShader.h"

namespace Hazel {

	namespace Utils {

		enum class ShaderStage : uint8_t
		{
			None = 0, Vertex = 1, Fragment = 2, Compute = 3
		};

		VkShaderStageFlagBits ShaderStageToVkShaderStage(ShaderStage stage)
		{
			switch (stage)
			{
				case ShaderStage::Vertex:   return VK_SHADER_STAGE_VERTEX_BIT;
				case ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
				case ShaderStage::Compute:  return VK_SHADER_STAGE_COMPUTE_BIT;
			}

			HZ_CORE_VERIFY(false);
			return (VkShaderStageFlagBits)0;
		}

		ShaderStage ShaderStageFromVkShaderStage(VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
				case VK_SHADER_STAGE_VERTEX_BIT:   return ShaderStage::Vertex;
				case VK_SHADER_STAGE_FRAGMENT_BIT: return ShaderStage::Fragment;
				case VK_SHADER_STAGE_COMPUTE_BIT:  return ShaderStage::Compute;
			}

			HZ_CORE_VERIFY(false);
			return (ShaderStage)0;
		}

	}

	ShaderPack::ShaderPack(const std::filesystem::path& path)
		: m_Path(path)
	{
		// Read index
		FileStreamReader serializer(path);
		if (!serializer)
			return;

		serializer.ReadRaw(m_File.Header);
		if (memcmp(m_File.Header.HEADER, "HZSP", 4) != 0)
			return;

		m_Loaded = true;
		for (uint32_t i = 0; i < m_File.Header.ShaderProgramCount; i++)
		{
			uint32_t key;
			serializer.ReadRaw(key);
			auto& shaderProgramInfo = m_File.Index.ShaderPrograms[key];
			serializer.ReadRaw(shaderProgramInfo.ReflectionDataOffset);
			serializer.ReadArray(shaderProgramInfo.ModuleIndices);
		}

		auto sp = serializer.GetStreamPosition();
		serializer.ReadArray(m_File.Index.ShaderModules, m_File.Header.ShaderModuleCount);
	}

	bool ShaderPack::Contains(std::string_view name) const
	{
		return m_File.Index.ShaderPrograms.find(Hash::GenerateFNVHash(name)) != m_File.Index.ShaderPrograms.end();
	}

	Ref<Shader> ShaderPack::LoadShader(std::string_view name)
	{
		uint32_t nameHash = Hash::GenerateFNVHash(name);
		HZ_CORE_VERIFY(Contains(name));

		const auto& shaderProgramInfo = m_File.Index.ShaderPrograms.at(nameHash);

		FileStreamReader serializer(m_Path);

		serializer.SetStreamPosition(shaderProgramInfo.ReflectionDataOffset);
		
		// Debug only
		std::string shaderName;
		{
			std::string path(name);
			size_t found = path.find_last_of("/\\");
			shaderName = found != std::string::npos ? path.substr(found + 1) : path;
			found = shaderName.find_last_of('.');
			shaderName = found != std::string::npos ? shaderName.substr(0, found) : name;
		}

		Ref<VulkanShader> vulkanShader = Ref<VulkanShader>::Create();
		vulkanShader->m_Name = shaderName;
		vulkanShader->m_AssetPath = name;
		//vulkanShader->TryReadReflectionData(&serializer);
		// vulkanShader->m_DisableOptimization =

		std::map<VkShaderStageFlagBits, std::vector<uint32_t>> shaderModules;
		for (uint32_t index : shaderProgramInfo.ModuleIndices)
		{
			const auto& info = m_File.Index.ShaderModules[index];
			auto& moduleData = shaderModules[Utils::ShaderStageToVkShaderStage((Utils::ShaderStage)info.Stage)];

			serializer.SetStreamPosition(info.PackedOffset);
			serializer.ReadArray(moduleData, (uint32_t)info.PackedSize);
		}

		serializer.SetStreamPosition(shaderProgramInfo.ReflectionDataOffset);
		vulkanShader->TryReadReflectionData(&serializer);

		vulkanShader->LoadAndCreateShaders(shaderModules);
		vulkanShader->CreateDescriptors();

		//Renderer::AcknowledgeParsedGlobalMacros(compiler->GetAcknowledgedMacros(), vulkanShader);
		//Renderer::OnShaderReloaded(vulkanShader->GetHash());
		return vulkanShader;
	}

	Ref<ShaderPack> ShaderPack::CreateFromLibrary(Ref<ShaderLibrary> shaderLibrary, const std::filesystem::path& path)
	{
		Ref<ShaderPack> shaderPack = Ref<ShaderPack>::Create();

		const auto& shaderMap = shaderLibrary->GetShaders();
		auto& shaderPackFile = shaderPack->m_File;
		
		shaderPackFile.Header.Version = 1;
		shaderPackFile.Header.ShaderProgramCount = (uint32_t)shaderMap.size();
		shaderPackFile.Header.ShaderModuleCount = 0;

		// Determine number of modules (per shader)
		// NOTE(Yan): this currently doesn't care about duplicated modules, but it should (eventually, not that important atm)
		uint32_t shaderModuleIndex = 0;
		uint32_t shaderModuleIndexArraySize = 0;
		for (const auto& [name, shader] : shaderMap)
		{
			Ref<VulkanShader> vulkanShader = shader.As<VulkanShader>();
			const auto& shaderData = vulkanShader->m_ShaderData;

			shaderPackFile.Header.ShaderModuleCount += (uint32_t)shaderData.size();
			auto& shaderProgramInfo = shaderPackFile.Index.ShaderPrograms[(uint32_t)vulkanShader->GetHash()];

			for (int i = 0; i < (int)shaderData.size(); i++)
				shaderProgramInfo.ModuleIndices.emplace_back(shaderModuleIndex++);

			shaderModuleIndexArraySize += sizeof(uint32_t); // size
			shaderModuleIndexArraySize += (uint32_t)shaderData.size() * sizeof(uint32_t); // indices
		}

		uint32_t shaderProgramIndexSize = shaderPackFile.Header.ShaderProgramCount *
			(sizeof(std::map<uint32_t, ShaderPackFile::ShaderProgramInfo>::key_type) + sizeof(ShaderPackFile::ShaderProgramInfo::ReflectionDataOffset))
			+ shaderModuleIndexArraySize;

		FileStreamWriter serializer(path);
		
		// Write header
		serializer.WriteRaw<ShaderPackFile::FileHeader>(shaderPackFile.Header);

		// ===============
		// Write index
		// ===============
		// Write dummy data for shader programs
		uint64_t shaderProgramIndexPos = serializer.GetStreamPosition();
		serializer.WriteZero(shaderProgramIndexSize);

		// Write dummy data for shader modules
		uint64_t shaderModuleIndexPos = serializer.GetStreamPosition();
		serializer.WriteZero(shaderPackFile.Header.ShaderModuleCount * sizeof(ShaderPackFile::ShaderModuleInfo));
		for (const auto& [name, shader] : shaderMap)
		{
			Ref<VulkanShader> vulkanShader = shader.As<VulkanShader>();

			// Serialize reflection data
			shaderPackFile.Index.ShaderPrograms[(uint32_t)vulkanShader->GetHash()].ReflectionDataOffset = serializer.GetStreamPosition();
			vulkanShader->SerializeReflectionData(&serializer);

			// Serialize SPIR-V data
			const auto& shaderData = vulkanShader->m_ShaderData;
			for (const auto& [stage, data] : shaderData)
			{
				auto& indexShaderModule = shaderPackFile.Index.ShaderModules.emplace_back();
				indexShaderModule.PackedOffset = serializer.GetStreamPosition();
				indexShaderModule.PackedSize = data.size();
				indexShaderModule.Stage = (uint8_t)Utils::ShaderStageFromVkShaderStage(stage);

				serializer.WriteArray(data, false);
			}
		}

		// Write program index
		serializer.SetStreamPosition(shaderProgramIndexPos);
		uint64_t begin = shaderProgramIndexPos;
		for (const auto& [name, programInfo] : shaderPackFile.Index.ShaderPrograms)
		{
			serializer.WriteRaw(name);
			serializer.WriteRaw(programInfo.ReflectionDataOffset);
			serializer.WriteArray(programInfo.ModuleIndices);
		}
		uint64_t end = serializer.GetStreamPosition();
		uint64_t s = end - begin;

		// Write module index
		serializer.SetStreamPosition(shaderModuleIndexPos);
		serializer.WriteArray(shaderPackFile.Index.ShaderModules, false);

		return shaderPack;
	}

#if 0
	void ShaderPack::CompileAndStoreShader(std::string_view shaderSourcePath)
	{
		uint32_t hash = Hash::GenerateFNVHash(shaderSourcePath);
		m_File.Index.ShaderPrograms[hash];

		BinarySerializer serializer(m_File);
		serializer.SerializeToFile(m_Path);
	}

	void ShaderPack::AddShader(Ref<Shader> shader)
	{
	}
#endif

}

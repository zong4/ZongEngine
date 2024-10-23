#include "hzpch.h"
#include "VulkanShaderCache.h"
#include "Engine/Core/Hash.h"
#include "Engine/Platform/Vulkan/VulkanShaderUtils.h"

#include "yaml-cpp/yaml.h"
#include "Engine/Utilities/SerializationMacros.h"

#include "ShaderPreprocessing/ShaderPreprocessor.h"

namespace Hazel {

	static const char* s_ShaderRegistryPath = "Resources/Cache/Shader/ShaderRegistry.cache";


	VkShaderStageFlagBits VulkanShaderCache::HasChanged(Ref<VulkanShaderCompiler> shader)
	{
		std::map<std::string, std::map<VkShaderStageFlagBits, StageData>> shaderCache;

		Deserialize(shaderCache);

		VkShaderStageFlagBits changedStages = {};
		const bool shaderNotCached = shaderCache.find(shader->m_ShaderSourcePath.string()) == shaderCache.end();

		for (const auto& [stage, stageSource] : shader->m_ShaderSource)
		{
			// Keep in mind that we're using the [] operator.
			// Which means that we add the stage if it's not already there.
			if (shaderNotCached || shader->m_StagesMetadata.at(stage) != shaderCache[shader->m_ShaderSourcePath.string()][stage])
			{
				shaderCache[shader->m_ShaderSourcePath.string()][stage] = shader->m_StagesMetadata.at(stage);
				*(int*)&changedStages |= stage;
			}
		}

		// Update cache in case we added a stage but didn't remove the deleted(in file) stages
		shaderCache.at(shader->m_ShaderSourcePath.string()) = shader->m_StagesMetadata;

		if (changedStages)
		{
			Serialize(shaderCache);
		}

		return changedStages;
	}


	void VulkanShaderCache::Serialize(const std::map<std::string, std::map<VkShaderStageFlagBits, StageData>>& shaderCache)
	{
		YAML::Emitter out;

		out << YAML::BeginMap << YAML::Key << "ShaderRegistry" << YAML::BeginSeq;// ShaderRegistry_

		for (auto& [filepath, shader] : shaderCache)
		{
			out << YAML::BeginMap; // Shader_

			out << YAML::Key << "ShaderPath" << YAML::Value << filepath;

			out << YAML::Key << "Stages" << YAML::BeginSeq; // Stages_

			for (auto& [stage, stageData] : shader)
			{
				out << YAML::BeginMap; // Stage_

				out << YAML::Key << "Stage" << YAML::Value << ShaderUtils::ShaderStageToString(stage);
				out << YAML::Key << "StageHash" << YAML::Value << stageData.HashValue;

				out << YAML::Key << "Headers" << YAML::BeginSeq; // Headers_
				for (auto& header : stageData.Headers)
				{

					out << YAML::BeginMap;

					HZ_SERIALIZE_PROPERTY(HeaderPath, header.IncludedFilePath.string(), out);
					HZ_SERIALIZE_PROPERTY(IncludeDepth, header.IncludeDepth, out);
					HZ_SERIALIZE_PROPERTY(IsRelative, header.IsRelative, out);
					HZ_SERIALIZE_PROPERTY(IsGaurded, header.IsGuarded, out);
					HZ_SERIALIZE_PROPERTY(HashValue, header.HashValue, out);

					out << YAML::EndMap;
				}
				out << YAML::EndSeq; // Headers_

				out << YAML::EndMap; // Stage_
			}
			out << YAML::EndSeq; // Stages_
			out << YAML::EndMap; // Shader_

		}
		out << YAML::EndSeq; // ShaderRegistry_
		out << YAML::EndMap; // File_

		std::ofstream fout(s_ShaderRegistryPath);
		fout << out.c_str();
	}

	void VulkanShaderCache::Deserialize(std::map<std::string, std::map<VkShaderStageFlagBits, StageData>>& shaderCache)
	{
		// Read registry
		std::ifstream stream(s_ShaderRegistryPath);
		if (!stream.good())
			return;

		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		auto handles = data["ShaderRegistry"];
		if (handles.IsNull())
		{
			HZ_CORE_ERROR("[ShaderCache] Shader Registry is invalid.");
			return;
		}

		// Old format
		if (handles.IsMap())
		{
			HZ_CORE_ERROR("[ShaderCache] Old Shader Registry format.");
			return;
		}

		for (auto shader : handles)
		{
			std::string path;
			HZ_DESERIALIZE_PROPERTY(ShaderPath, path, shader, std::string());
			for (auto stage : shader["Stages"]) //Stages
			{
				std::string stageType;
				uint32_t stageHash;
				HZ_DESERIALIZE_PROPERTY(Stage, stageType, stage, std::string());
				HZ_DESERIALIZE_PROPERTY(StageHash, stageHash, stage, 0u);

				auto& stageCache = shaderCache[path][ShaderUtils::ShaderTypeFromString(stageType)];
				stageCache.HashValue = stageHash;

				for (auto header : stage["Headers"])
				{
					std::string headerPath;
					uint32_t includeDepth;
					bool isRelative;
					bool isGuarded;
					uint32_t hashValue;
					HZ_DESERIALIZE_PROPERTY(HeaderPath, headerPath, header, std::string());
					HZ_DESERIALIZE_PROPERTY(IncludeDepth, includeDepth, header, 0u);
					HZ_DESERIALIZE_PROPERTY(IsRelative, isRelative, header, false);
					HZ_DESERIALIZE_PROPERTY(IsGaurded, isGuarded, header, false);
					HZ_DESERIALIZE_PROPERTY(HashValue, hashValue, header, 0u);

					stageCache.Headers.emplace(IncludeData{ headerPath, includeDepth, isRelative, isGuarded, hashValue });

				}

			}

		}

	}

}

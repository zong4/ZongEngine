#pragma once

#include "vulkan/vulkan.h"

#include "Engine/Platform/Vulkan/VulkanShaderResource.h"
#include "Engine/Platform/Vulkan/VulkanShaderUtils.h"
#include "Engine/Platform/Vulkan/VulkanShader.h"

#include "ShaderPreprocessing/ShaderPreprocessor.h"

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>

struct IDxcCompiler3;
struct IDxcUtils;

namespace Engine {

	struct DxcInstances
	{
		inline static IDxcCompiler3* Compiler = nullptr;
		inline static IDxcUtils* Utils = nullptr;
	};
	struct StageData
	{
		std::unordered_set<IncludeData> Headers;
		uint32_t HashValue = 0;
		bool operator== (const StageData& other) const noexcept { return this->Headers == other.Headers && this->HashValue == other.HashValue; }
		bool operator!= (const StageData& other) const noexcept { return !(*this == other); }
	};

	class VulkanShader;

	class VulkanShaderCompiler : public RefCounted
	{
	public:
		VulkanShaderCompiler(const std::filesystem::path& shaderSourcePath, bool disableOptimization = false);

		bool Reload(bool forceCompile = false);

		const std::map<VkShaderStageFlagBits, std::vector<uint32_t>>& GetSPIRVData() const { return m_SPIRVData; }
		const std::unordered_set<std::string>& GetAcknowledgedMacros() const { return m_AcknowledgedMacros; }

		static void ClearUniformBuffers();

		static Ref<VulkanShader> Compile(const std::filesystem::path& shaderSourcePath, bool forceCompile = false, bool disableOptimization = false);
		static bool TryRecompile(Ref<VulkanShader> shader);
	private:
		std::map<VkShaderStageFlagBits, std::string> PreProcess(const std::string& source);
		std::map<VkShaderStageFlagBits, std::string> PreProcessGLSL(const std::string& source);
		std::map<VkShaderStageFlagBits, std::string> PreProcessHLSL(const std::string& source);

		struct CompilationOptions
		{
			bool GenerateDebugInfo = false;
			bool Optimize = true;
		};

		std::string Compile(std::vector<uint32_t>& outputBinary, const VkShaderStageFlagBits stage, CompilationOptions options) const;
		bool CompileOrGetVulkanBinaries(std::map<VkShaderStageFlagBits, std::vector<uint32_t>>& outputDebugBinary, std::map<VkShaderStageFlagBits, std::vector<uint32_t>>& outputBinary, const VkShaderStageFlagBits changedStages, const bool forceCompile);
		bool CompileOrGetVulkanBinary(VkShaderStageFlagBits stage, std::vector<uint32_t>& outputBinary, bool debug, VkShaderStageFlagBits changedStages, bool forceCompile);

		void ClearReflectionData();

		void TryGetVulkanCachedBinary(const std::filesystem::path& cacheDirectory, const std::string& extension, std::vector<uint32_t>& outputBinary) const;
		bool TryReadCachedReflectionData();
		void SerializeReflectionData();
		void SerializeReflectionData(StreamWriter* serializer);

		void ReflectAllShaderStages(const std::map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData);
		void Reflect(VkShaderStageFlagBits shaderStage, const std::vector<uint32_t>& shaderData);
	private:
		std::filesystem::path m_ShaderSourcePath;
		bool m_DisableOptimization = false;

		std::map<VkShaderStageFlagBits, std::string> m_ShaderSource;
		std::map<VkShaderStageFlagBits, std::vector<uint32_t>> m_SPIRVDebugData, m_SPIRVData;

		// Reflection info
		VulkanShader::ReflectionData m_ReflectionData;

		// Names of macros that are parsed from shader.
		// These are used to reliably get informattion about what shaders need what macros
		std::unordered_set<std::string> m_AcknowledgedMacros;
		ShaderUtils::SourceLang m_Language;

		std::map<VkShaderStageFlagBits, StageData> m_StagesMetadata;
		
		friend class VulkanShader;
		friend class VulkanShaderCache;
		friend class ShaderPack;
	};

}

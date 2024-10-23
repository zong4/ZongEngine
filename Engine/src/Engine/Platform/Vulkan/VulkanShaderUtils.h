#pragma once
#include <shaderc/shaderc.h>

#include <vulkan/vulkan_core.h>
#include "Engine/Renderer/Shader.h"


namespace Hazel {
	namespace ShaderUtils {

		inline static std::string_view VKStageToShaderMacro(const VkShaderStageFlagBits stage)
		{
			if (stage == VK_SHADER_STAGE_VERTEX_BIT)   return "__VERTEX_STAGE__";
			if (stage == VK_SHADER_STAGE_FRAGMENT_BIT) return "__FRAGMENT_STAGE__";
			if (stage == VK_SHADER_STAGE_COMPUTE_BIT)  return "__COMPUTE_STAGE__";
			HZ_CORE_VERIFY(false, "Unknown shader stage.");
			return "";
		}

		inline static std::string_view StageToShaderMacro(const std::string_view stage)
		{
			if (stage == "vert") return "__VERTEX_STAGE__";
			if (stage == "frag") return "__FRAGMENT_STAGE__";
			if (stage == "comp") return "__COMPUTE_STAGE__";
			HZ_CORE_VERIFY(false, "Unknown shader stage.");
			return "";
		}

		inline static VkShaderStageFlagBits StageToVKShaderStage(const std::string_view stage)
		{
			if (stage == "vert") return VK_SHADER_STAGE_VERTEX_BIT;
			if (stage == "frag") return VK_SHADER_STAGE_FRAGMENT_BIT;
			if (stage == "comp") return VK_SHADER_STAGE_COMPUTE_BIT;
			HZ_CORE_VERIFY(false, "Unknown shader stage.");
			return VK_SHADER_STAGE_ALL;
		}

		inline static const char* ShaderStageToString(const VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
				case VK_SHADER_STAGE_VERTEX_BIT:    return "vert";
				case VK_SHADER_STAGE_FRAGMENT_BIT:  return "frag";
				case VK_SHADER_STAGE_COMPUTE_BIT:   return "comp";
			}
			HZ_CORE_ASSERT(false);
			return "UNKNOWN";
		}

		inline static VkShaderStageFlagBits ShaderTypeFromString(const std::string_view type)
		{
			if (type == "vert")	return VK_SHADER_STAGE_VERTEX_BIT;
			if (type == "frag")	return VK_SHADER_STAGE_FRAGMENT_BIT;
			if (type == "comp")	return VK_SHADER_STAGE_COMPUTE_BIT;

			return VK_SHADER_STAGE_ALL;
		}

		inline static SourceLang ShaderLangFromExtension(const std::string_view type)
		{
			if (type == ".glsl")	return SourceLang::GLSL;
			if (type == ".hlsl")	return SourceLang::HLSL;

			HZ_CORE_ASSERT(false);

			return SourceLang::NONE;
		}

		inline static shaderc_shader_kind ShaderStageToShaderC(const VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
				case VK_SHADER_STAGE_VERTEX_BIT:    return shaderc_vertex_shader;
				case VK_SHADER_STAGE_FRAGMENT_BIT:  return shaderc_fragment_shader;
				case VK_SHADER_STAGE_COMPUTE_BIT:   return shaderc_compute_shader;
			}
			HZ_CORE_ASSERT(false);
			return {};
		}

		inline static const char* ShaderStageCachedFileExtension(const VkShaderStageFlagBits stage, bool debug)
		{
			if (debug)
			{
				switch (stage)
				{
					case VK_SHADER_STAGE_VERTEX_BIT:    return ".cached_vulkan_debug.vert";
					case VK_SHADER_STAGE_FRAGMENT_BIT:  return ".cached_vulkan_debug.frag";
					case VK_SHADER_STAGE_COMPUTE_BIT:   return ".cached_vulkan_debug.comp";
				}
			}
			else
			{
				switch (stage)
				{
					case VK_SHADER_STAGE_VERTEX_BIT:    return ".cached_vulkan.vert";
					case VK_SHADER_STAGE_FRAGMENT_BIT:  return ".cached_vulkan.frag";
					case VK_SHADER_STAGE_COMPUTE_BIT:   return ".cached_vulkan.comp";
				}
			}
			HZ_CORE_ASSERT(false);
			return "";
		}

#ifdef HZ_PLATFORM_WINDOWS
		inline static const wchar_t* HLSLShaderProfile(const VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
				case VK_SHADER_STAGE_VERTEX_BIT:    return L"vs_6_0";
				case VK_SHADER_STAGE_FRAGMENT_BIT:  return L"ps_6_0";
				case VK_SHADER_STAGE_COMPUTE_BIT:   return L"cs_6_0";
			}
			HZ_CORE_ASSERT(false);
			return L"";
		}
#else
		inline static const char* HLSLShaderProfile(const VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
				case VK_SHADER_STAGE_VERTEX_BIT:    return "vs_6_0";
				case VK_SHADER_STAGE_FRAGMENT_BIT:  return "ps_6_0";
				case VK_SHADER_STAGE_COMPUTE_BIT:   return "cs_6_0";
			}
			HZ_CORE_ASSERT(false);
			return "";
		}
#endif
	}


}

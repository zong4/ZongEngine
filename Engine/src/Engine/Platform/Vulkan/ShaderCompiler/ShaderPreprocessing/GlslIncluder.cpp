#include "hzpch.h"
#include "GlslIncluder.h"

#include "Engine/Utilities/StringUtils.h"
#include <libshaderc_util/io_shaderc.h>
#include "Engine/Core/Hash.h"
#include "Engine/Platform/Vulkan/VulkanShader.h"


namespace Hazel {

	GlslIncluder::GlslIncluder(const shaderc_util::FileFinder* file_finder)
		: m_FileFinder(*file_finder)
	{
	}

	GlslIncluder::~GlslIncluder() = default;

	shaderc_include_result* GlslIncluder::GetInclude(const char* requestedPath, const shaderc_include_type type, const char* requestingPath, const size_t includeDepth)
	{
		const std::filesystem::path requestedFullPath = (type == shaderc_include_type_relative)
			? m_FileFinder.FindRelativeReadableFilepath(requestingPath, requestedPath)
			: m_FileFinder.FindReadableFilepath(requestedPath);

		auto& [source, sourceHash, stages, isGuarded] = m_HeaderCache[requestedFullPath.string()];

		if (source.empty())
		{
			source = Utils::ReadFileAndSkipBOM(requestedFullPath);
			if (source.empty())
				HZ_CORE_ERROR("Failed to load included file: {} in {}.", requestedFullPath, requestingPath);
			sourceHash = Hash::GenerateFNVHash(source.c_str());

			// Can clear "source" in case it has already been included in this stage and is guarded.
			stages = ShaderPreprocessor::PreprocessHeader<ShaderUtils::SourceLang::GLSL>(source, isGuarded, m_ParsedSpecialMacros, m_includeData, requestedFullPath);
		}
		else if (isGuarded)
		{
			source.clear();
		}

		// Does not emplace if it finds the same include path and same header hash value.
		m_includeData.emplace(IncludeData{ requestedFullPath, includeDepth, type == shaderc_include_type_relative, isGuarded, sourceHash, stages });

		auto* const container = new std::array<std::string, 2>;
		(*container)[0] = requestedPath;
		(*container)[1] = source;
		auto* const data = new shaderc_include_result;

		data->user_data = container;

		data->source_name = (*container)[0].data();
		data->source_name_length = (*container)[0].size();

		data->content = (*container)[1].data();
		data->content_length = (*container)[1].size();

		return data;
	}

	void GlslIncluder::ReleaseInclude(shaderc_include_result* data)
	{
		delete static_cast<std::array<std::string, 2>*>(data->user_data);
		delete data;
	}

}

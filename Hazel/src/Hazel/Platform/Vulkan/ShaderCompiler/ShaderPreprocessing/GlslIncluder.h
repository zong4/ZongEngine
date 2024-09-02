#pragma once

#include <unordered_set>
#include <shaderc/shaderc.hpp>
#include <libshaderc_util/file_finder.h>

#include "ShaderPreprocessor.h"

namespace Hazel {
	struct IncludeData;

	class GlslIncluder : public shaderc::CompileOptions::IncluderInterface
	{
	public:
		explicit GlslIncluder(const shaderc_util::FileFinder* file_finder);

		~GlslIncluder() override;

		shaderc_include_result* GetInclude(const char* requestedPath, shaderc_include_type type, const char* requestingPath, size_t includeDepth) override;

		void ReleaseInclude(shaderc_include_result* data) override;

		std::unordered_set<IncludeData>&& GetIncludeData() { return std::move(m_includeData); }
		std::unordered_set<std::string>&& GetParsedSpecialMacros() { return std::move(m_ParsedSpecialMacros); }

	private:
		// Used by GetInclude() to get the full filepath.
		const shaderc_util::FileFinder& m_FileFinder;
		std::unordered_set<IncludeData> m_includeData;
		std::unordered_set<std::string> m_ParsedSpecialMacros;
		std::unordered_map <std::string, HeaderCache> m_HeaderCache;
	};
}

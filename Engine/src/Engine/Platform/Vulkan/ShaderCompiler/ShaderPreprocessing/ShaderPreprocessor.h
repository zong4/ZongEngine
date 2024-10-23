#pragma once

#include <filesystem>
#include <unordered_set>
#include <sstream>

#include "Engine/Utilities/StringUtils.h"
#include "Engine/Renderer/Shader.h"
#include "Engine/Platform/Vulkan/VulkanShaderUtils.h"

enum VkShaderStageFlagBits;

namespace Hazel {
	namespace PreprocessUtils {
		template<bool RemoveHeaderGuard = false>
		bool ContainsHeaderGuard(std::string& header)
		{
			size_t pos = header.find('#');
			while (pos != std::string::npos)
			{
				const size_t endOfLine = header.find_first_of("\r\n", pos) + 1;
				auto tokens = Utils::SplitStringAndKeepDelims(header.substr(pos, endOfLine - pos));
				auto it = tokens.begin();

				if (*(++it) == "pragma")
				{
					if (*(++it) == "once")
					{
						if constexpr (RemoveHeaderGuard)
							header.erase(pos, endOfLine - pos);
						return true;
					}
				}
				pos = header.find('#', pos + 1);
			}
			return false;
		}

		// From https://wandbox.org/permlink/iXC7DWaU8Tk8jrf3 and is modified.
		enum class State : char { SlashOC, StarIC, SingleLineComment, MultiLineComment, NotAComment };

		template <typename InputIt, typename OutputIt>
		void CopyWithoutComments(InputIt first, InputIt last, OutputIt out)
		{
			State state = State::NotAComment;

			while (first != last)
			{
				switch (state)
				{
					case State::SlashOC:
						if (*first == '/') state = State::SingleLineComment;
						else if (*first == '*') state = State::MultiLineComment;
						else
						{
							state = State::NotAComment;
							*out++ = '/';
							*out++ = *first;
						}
						break;
					case State::StarIC:
						if (*first == '/') state = State::NotAComment;
						else state = State::MultiLineComment;
						break;
					case State::NotAComment:
						if (*first == '/') state = State::SlashOC;
						else *out++ = *first;
						break;
					case State::SingleLineComment:
						if (*first == '\n')
						{
							state = State::NotAComment;
							*out++ = '\n';
						}
						break;
					case State::MultiLineComment:
						if (*first == '*') state = State::StarIC;
						else if (*first == '\n') *out++ = '\n';
						break;
				}
				++first;
			}
		}
	}

	struct IncludeData
	{
		std::filesystem::path IncludedFilePath {};
		size_t IncludeDepth {};
		bool IsRelative { false };
		bool IsGuarded { false };
		uint32_t HashValue {};

		VkShaderStageFlagBits IncludedStage{};

		inline bool operator==(const IncludeData& other) const noexcept
		{
			return this->IncludedFilePath == other.IncludedFilePath && this->HashValue == other.HashValue;
		}
	};

	struct HeaderCache
	{
		std::string Source;
		uint32_t SourceHash;
		VkShaderStageFlagBits Stages;
		bool IsGuarded;
	};
}

namespace std {
	template<>
	struct hash<Hazel::IncludeData>
	{
		size_t operator()(const Hazel::IncludeData& data) const noexcept
		{
			return std::filesystem::hash_value(data.IncludedFilePath) ^ data.HashValue;
		}
	};
}

namespace Hazel {
	class ShaderPreprocessor
	{
	public:
		template<ShaderUtils::SourceLang Lang>
		static VkShaderStageFlagBits PreprocessHeader(std::string& contents, bool& isGuarded, std::unordered_set<std::string>& specialMacros, const std::unordered_set<IncludeData>& includeData, const std::filesystem::path& fullPath);
		template<ShaderUtils::SourceLang Lang>
		static std::map<VkShaderStageFlagBits, std::string> PreprocessShader(const std::string& source, std::unordered_set<std::string>& specialMacros);
	};

	template<ShaderUtils::SourceLang Lang>
	VkShaderStageFlagBits ShaderPreprocessor::PreprocessHeader(std::string& contents, bool& isGuarded, std::unordered_set<std::string>& specialMacros, const std::unordered_set<IncludeData>& includeData, const std::filesystem::path& fullPath)
	{
		std::stringstream sourceStream;
		PreprocessUtils::CopyWithoutComments(contents.begin(), contents.end(), std::ostream_iterator<char>(sourceStream));
		contents = sourceStream.str();

		VkShaderStageFlagBits stagesInHeader = {};

		//Removes header guard in GLSL only.
		isGuarded = PreprocessUtils::ContainsHeaderGuard<Lang == ShaderUtils::SourceLang::GLSL>(contents);

		uint32_t stageCount = 0;
		size_t startOfShaderStage = contents.find('#', 0);

		while (startOfShaderStage != std::string::npos)
		{
			const size_t endOfLine = contents.find_first_of("\r\n", startOfShaderStage) + 1;
			// Parse stage. example: #pragma stage:vert
			auto tokens = Utils::SplitStringAndKeepDelims(contents.substr(startOfShaderStage, endOfLine - startOfShaderStage));

			uint32_t index = 0;
			// Pre-processor directives
			if (tokens[index] == "#")
			{
				++index;
				// Pragmas
				if (tokens[index] == "pragma")
				{
					++index;
					// Stages
					if (tokens[index] == "stage")
					{
						ZONG_CORE_VERIFY(tokens[++index] == ":", "Stage pragma is invalid");

						// Skipped ':'
						const std::string_view stage(tokens[++index]);
						ZONG_CORE_VERIFY(stage == "vert" || stage == "frag" || stage == "comp", "Invalid shader type specified");
						VkShaderStageFlagBits foundStage = ShaderUtils::StageToVKShaderStage(stage);


						const bool alreadyIncluded = std::find_if(includeData.begin(), includeData.end(), [fullPath, foundStage](const IncludeData& data)
						{
							return data.IncludedFilePath == fullPath.string() && !bool(foundStage & data.IncludedStage);
						}) != includeData.end();

						if (isGuarded && alreadyIncluded)
							contents.clear();
						else if (!isGuarded && alreadyIncluded)
							ZONG_CORE_WARN("\"{}\" Header does not contain a header guard (#pragma once).", fullPath);

						// Add #endif
						if (stageCount == 0)
							contents.replace(startOfShaderStage, endOfLine - startOfShaderStage, fmt::format("#ifdef {}\r\n", ShaderUtils::StageToShaderMacro(stage)));
						else // Add stage macro instead of stage pragma, both #endif and #ifdef must be in the same line, hence no '\n'
							contents.replace(startOfShaderStage, endOfLine - startOfShaderStage, fmt::format("#endif\r\n#ifdef {}", ShaderUtils::StageToShaderMacro(stage)));

						*(int*)&stagesInHeader |= (int)foundStage;
						stageCount++;
					}
				}
				else if (tokens[index] == "ifdef")
				{
					++index;
					if (tokens[index].rfind("__ZONG_", 0) == 0) // Hazel special macros start with "__ZONG_"
					{
						specialMacros.emplace(tokens[index]);
					}
				}
				else if (tokens[index] == "if" || tokens[index] == "define") // Consider "#if defined()" too?
				{
					++index;
					for (size_t i = index; i < tokens.size(); ++i)
					{
						if (tokens[i].rfind("__ZONG_", 0) == 0) // Hazel special macros start with "__ZONG_"
						{
							specialMacros.emplace(tokens[i]);
						}
					}
				}
			}

			startOfShaderStage = contents.find('#', startOfShaderStage + 1);
		}
		if (stageCount)
			contents.append("\n#endif");
		else
		{
			const bool alreadyIncluded = std::find_if(includeData.begin(), includeData.end(), [fullPath](const IncludeData& data)
			{
				return data.IncludedFilePath == fullPath;
			}) != includeData.end();
			if (isGuarded && alreadyIncluded)
				contents.clear();
			else if (!isGuarded && alreadyIncluded)
				ZONG_CORE_WARN("\"{}\" Header does not contain a header guard (#pragma once)", fullPath);
		}


		return stagesInHeader;
	}

	template <ShaderUtils::SourceLang Lang>
	std::map<VkShaderStageFlagBits, std::string> ShaderPreprocessor::PreprocessShader(const std::string& source, std::unordered_set<std::string>& specialMacros)
	{
		std::stringstream sourceStream;
		PreprocessUtils::CopyWithoutComments(source.begin(), source.end(), std::ostream_iterator<char>(sourceStream));
		std::string newSource = sourceStream.str();

		std::map<VkShaderStageFlagBits, std::string> shaderSources;
		std::vector<std::pair<VkShaderStageFlagBits, size_t>> stagePositions;
		ZONG_CORE_ASSERT(newSource.size(), "Shader is empty!");

		size_t startOfStage = 0;
		size_t pos = newSource.find('#');

		//Check first #version
		if constexpr (Lang == ShaderUtils::SourceLang::GLSL)
		{
			const size_t endOfLine = newSource.find_first_of("\r\n", pos) + 1;
			const std::vector<std::string> tokens = Utils::SplitStringAndKeepDelims(newSource.substr(pos, endOfLine - pos));
			ZONG_CORE_VERIFY(tokens.size() >= 3 && tokens[1] == "version", "Invalid #version encountered or #version is NOT encounted first.");
			pos = newSource.find('#', pos + 1);
		}

		while (pos != std::string::npos)
		{

			const size_t endOfLine = newSource.find_first_of("\r\n", pos) + 1;
			std::vector<std::string> tokens = Utils::SplitStringAndKeepDelims(newSource.substr(pos, endOfLine - pos));

			size_t index = 1; // Skip #


			if (tokens[index] == "pragma") // Parse stage. example: #pragma stage : vert
			{
				++index;
				if (tokens[index] == "stage")
				{
					++index;
					// Jump over ':'
					ZONG_CORE_VERIFY(tokens[index] == ":", "Stage pragma is invalid");
					++index;

					const std::string_view stage = tokens[index];
					ZONG_CORE_VERIFY(stage == "vert" || stage == "frag" || stage == "comp", "Invalid shader type specified");
					auto shaderStage = ShaderUtils::ShaderTypeFromString(stage);

					stagePositions.emplace_back(shaderStage, startOfStage);
				}
			}
			else if (tokens[index] == "ifdef")
			{
				++index;
				if (tokens[index].rfind("__ZONG_", 0) == 0) // Hazel special macros start with "__ZONG_"
				{
					specialMacros.emplace(tokens[index]);
				}
			}
			else if (tokens[index] == "if" || tokens[index] == "define")
			{
				++index;
				for (size_t i = index; i < tokens.size(); ++i)
				{
					if (tokens[i].rfind("__ZONG_", 0) == 0) // Hazel special macros start with "__ZONG_"
					{
						specialMacros.emplace(tokens[i]);
					}
				}
			}
			else if constexpr (Lang == ShaderUtils::SourceLang::GLSL)
			{
				if (tokens[index] == "version")
				{
					++index;
					startOfStage = pos;
				}
			}

			pos = newSource.find('#', pos + 1);
		}

		ZONG_CORE_VERIFY(stagePositions.size(), "Could not pre-process shader! There are no known stages defined in file.");
		auto& [firstStage, firstStagePos] = stagePositions[0];
		if (stagePositions.size() > 1)
		{
			//Get first stage
			const std::string firstStageStr = newSource.substr(0, stagePositions[1].second);
			size_t lineCount = std::count(firstStageStr.begin(), firstStageStr.end(), '\n') + 1;
			shaderSources[firstStage] = firstStageStr;


			//Get stages in the middle
			for (size_t i = 1; i < stagePositions.size() - 1; ++i)
			{
				auto& [stage, stagePos] = stagePositions[i];
				std::string stageStr = newSource.substr(stagePos, stagePositions[i + 1].second - stagePos);
				const size_t secondLinePos = stageStr.find_first_of('\n', 1) + 1;
				stageStr.insert(secondLinePos, fmt::format("#line {}\n", lineCount));
				shaderSources[stage] = stageStr;
				lineCount += std::count(stageStr.begin(), stageStr.end(), '\n') + 1;
			}

			//Get last stage
			auto& [stage, stagePos] = stagePositions[stagePositions.size() - 1];
			std::string lastStageStr = newSource.substr(stagePos);
			const size_t secondLinePos = lastStageStr.find_first_of('\n', 1) + 1;
			lastStageStr.insert(secondLinePos, fmt::format("#line {}\n", lineCount + 1));
			shaderSources[stage] = lastStageStr;
		}
		else
		{
			shaderSources[firstStage] = newSource;
		}

		return shaderSources;
	}
}




#include "pch.h"
#include "ImGuiFonts.h"

namespace Hazel::UI {

	static std::unordered_map<std::string, ImFont*> s_Fonts;

	void Fonts::Add(const FontConfiguration& config, bool isDefault)
	{
		if (s_Fonts.find(config.FontName) != s_Fonts.end())
		{
			ZONG_CORE_WARN_TAG("EditorUI", "Tried to add font with name '{0}' but that name is already taken!", config.FontName);
			return;
		}

		ImFontConfig imguiFontConfig;
		imguiFontConfig.MergeMode = config.MergeWithLast;
		auto& io = ImGui::GetIO();
		ImFont* font = io.Fonts->AddFontFromFileTTF(config.FilePath.data(), config.Size, &imguiFontConfig, config.GlyphRanges == nullptr ? io.Fonts->GetGlyphRangesDefault() : config.GlyphRanges);
		ZONG_CORE_VERIFY(font, "Failed to load font file!");
		s_Fonts[config.FontName] = font;

		if (isDefault)
			io.FontDefault = font;
	}

	ImFont* Fonts::Get(const std::string& fontName)
	{
		ZONG_CORE_VERIFY(s_Fonts.find(fontName) != s_Fonts.end(), "Failed to find font with that name!");
		return s_Fonts.at(fontName);
	}

	void Fonts::PushFont(const std::string& fontName)
	{
		const auto& io = ImGui::GetIO();

		if (s_Fonts.find(fontName) == s_Fonts.end())
		{
			ImGui::PushFont(io.FontDefault);
			return;
		}

		ImGui::PushFont(s_Fonts.at(fontName));
	}

	void Fonts::PopFont()
	{
		ImGui::PopFont();
	}

}


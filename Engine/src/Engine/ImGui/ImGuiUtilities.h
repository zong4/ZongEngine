#pragma once

#include "Engine/ImGui/Colors.h"
#include "Engine/Renderer/Texture.h"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <vector>
#include <string>

namespace Hazel::UI {
	//=========================================================================================
	/// Utilities
	ImTextureID GetTextureID(Ref<Texture2D> texture);

	class ScopedStyle
	{
	public:
		ScopedStyle(const ScopedStyle&) = delete;
		ScopedStyle& operator=(const ScopedStyle&) = delete;
		template<typename T>
		ScopedStyle(ImGuiStyleVar styleVar, T value) { ImGui::PushStyleVar(styleVar, value); }
		~ScopedStyle() { ImGui::PopStyleVar(); }
	};

	class ScopedColour
	{
	public:
		ScopedColour(const ScopedColour&) = delete;
		ScopedColour& operator=(const ScopedColour&) = delete;
		template<typename T>
		ScopedColour(ImGuiCol colourId, T colour) { ImGui::PushStyleColor(colourId, ImColor(colour).Value); }
		~ScopedColour() { ImGui::PopStyleColor(); }
	};

	class ScopedFont
	{
	public:
		ScopedFont(const ScopedFont&) = delete;
		ScopedFont& operator=(const ScopedFont&) = delete;
		ScopedFont(ImFont* font) { ImGui::PushFont(font); }
		~ScopedFont() { ImGui::PopFont(); }
	};

	class ScopedID
	{
	public:
		ScopedID(const ScopedID&) = delete;
		ScopedID& operator=(const ScopedID&) = delete;
		template<typename T>
		ScopedID(T id) { ImGui::PushID(id); }
		~ScopedID() { ImGui::PopID(); }
	};

	class ScopedColourStack
	{
	public:
		ScopedColourStack(const ScopedColourStack&) = delete;
		ScopedColourStack& operator=(const ScopedColourStack&) = delete;

		template <typename ColourType, typename... OtherColours>
		ScopedColourStack(ImGuiCol firstColourID, ColourType firstColour, OtherColours&& ... otherColourPairs)
			: m_Count((sizeof... (otherColourPairs) / 2) + 1)
		{
			static_assert ((sizeof... (otherColourPairs) & 1u) == 0,
				"ScopedColourStack constructor expects a list of pairs of colour IDs and colours as its arguments");

			PushColour(firstColourID, firstColour, std::forward<OtherColours>(otherColourPairs)...);
		}

		~ScopedColourStack() { ImGui::PopStyleColor(m_Count); }

	private:
		int m_Count;

		template <typename ColourType, typename... OtherColours>
		void PushColour(ImGuiCol colourID, ColourType colour, OtherColours&& ... otherColourPairs)
		{
			if constexpr (sizeof... (otherColourPairs) == 0)
			{
				ImGui::PushStyleColor(colourID, ImColor(colour).Value);
			}
			else
			{
				ImGui::PushStyleColor(colourID, ImColor(colour).Value);
				PushColour(std::forward<OtherColours>(otherColourPairs)...);
			}
		}
	};

	class ScopedStyleStack
	{
	public:
		ScopedStyleStack(const ScopedStyleStack&) = delete;
		ScopedStyleStack& operator=(const ScopedStyleStack&) = delete;

		template <typename ValueType, typename... OtherStylePairs>
		ScopedStyleStack(ImGuiStyleVar firstStyleVar, ValueType firstValue, OtherStylePairs&& ... otherStylePairs)
			: m_Count((sizeof... (otherStylePairs) / 2) + 1)
		{
			static_assert ((sizeof... (otherStylePairs) & 1u) == 0,
				"ScopedStyleStack constructor expects a list of pairs of colour IDs and colours as its arguments");

			PushStyle(firstStyleVar, firstValue, std::forward<OtherStylePairs>(otherStylePairs)...);
		}

		~ScopedStyleStack() { ImGui::PopStyleVar(m_Count); }

	private:
		int m_Count;

		template <typename ValueType, typename... OtherStylePairs>
		void PushStyle(ImGuiStyleVar styleVar, ValueType value, OtherStylePairs&& ... otherStylePairs)
		{
			if constexpr (sizeof... (otherStylePairs) == 0)
			{
				ImGui::PushStyleVar(styleVar, value);
			}
			else
			{
				ImGui::PushStyleVar(styleVar, value);
				PushStyle(std::forward<OtherStylePairs>(otherStylePairs)...);
			}
		}
	};


	class ScopedItemFlags
	{
	public:
		ScopedItemFlags(const ScopedItemFlags&) = delete;
		ScopedItemFlags& operator=(const ScopedItemFlags&) = delete;
		ScopedItemFlags(const ImGuiItemFlags flags, const bool enable = true)
		{
			ZONG_CORE_VERIFY(!(flags & ImGuiItemFlags_Disabled), "We shouldn't use ImGuiItemFlags_Disabled! Use UI::BeginDisabled / UI::EndDisabled instead. It will handle visuals for you.");
			ImGui::PushItemFlag(flags, enable);
		}
		~ScopedItemFlags() { ImGui::PopItemFlag(); }
	};

	class ScopedDisable
	{
	public:
		ScopedDisable(const ScopedDisable&) = delete;
		ScopedDisable& operator=(const ScopedDisable&) = delete;
		ScopedDisable(bool disabled = true);
		~ScopedDisable();
	};

	// The delay won't work on texts, because the timer isn't tracked for them.
	inline bool IsItemHovered(float delayInSeconds = 0.1f, ImGuiHoveredFlags flags = 0)
	{
		return ImGui::IsItemHovered() && GImGui->HoveredIdTimer > delayInSeconds; /*HoveredIdNotActiveTimer*/
	}

	inline void SetTooltip(std::string_view text, float delayInSeconds = 0.1f, bool allowWhenDisabled = true, ImVec2 padding = ImVec2(5, 5))
	{
		if (IsItemHovered(delayInSeconds, allowWhenDisabled ? ImGuiHoveredFlags_AllowWhenDisabled : 0))
		{
			UI::ScopedStyle tooltipPadding(ImGuiStyleVar_WindowPadding, padding);
			UI::ScopedColour textCol(ImGuiCol_Text, Colors::Theme::textBrighter);
			ImGui::SetTooltip(text.data());
		}
	}

	// Check if navigated to current item, e.g. with arrow keys
	inline bool NavigatedTo()
	{
		ImGuiContext& g = *GImGui;
		return g.NavJustMovedToId == g.LastItemData.ID;
	}


	//=========================================================================================
	/// Colours

	inline ImColor ColourWithValue(const ImColor& color, float value)
	{
		const ImVec4& colRaw = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRaw.x, colRaw.y, colRaw.z, hue, sat, val);
		return ImColor::HSV(hue, sat, std::min(value, 1.0f));
	}

	inline ImColor ColourWithSaturation(const ImColor& color, float saturation)
	{
		const ImVec4& colRaw = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRaw.x, colRaw.y, colRaw.z, hue, sat, val);
		return ImColor::HSV(hue, std::min(saturation, 1.0f), val);
	}

	inline ImColor ColourWithHue(const ImColor& color, float hue)
	{
		const ImVec4& colRaw = color.Value;
		float h, s, v;
		ImGui::ColorConvertRGBtoHSV(colRaw.x, colRaw.y, colRaw.z, h, s, v);
		return ImColor::HSV(std::min(hue, 1.0f), s, v);
	}

	inline ImColor ColourWithAlpha(const ImColor& color, float multiplier)
	{
		ImVec4 colRaw = color.Value;
		colRaw.w = multiplier;
		return colRaw;
	}

	inline ImColor ColourWithMultipliedValue(const ImColor& color, float multiplier)
	{
		const ImVec4& colRaw = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRaw.x, colRaw.y, colRaw.z, hue, sat, val);
		return ImColor::HSV(hue, sat, std::min(val * multiplier, 1.0f));
	}

	inline ImColor ColourWithMultipliedSaturation(const ImColor& color, float multiplier)
	{
		const ImVec4& colRaw = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRaw.x, colRaw.y, colRaw.z, hue, sat, val);
		return ImColor::HSV(hue, std::min(sat * multiplier, 1.0f), val);
	}

	inline ImColor ColourWithMultipliedHue(const ImColor& color, float multiplier)
	{
		const ImVec4& colRaw = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRaw.x, colRaw.y, colRaw.z, hue, sat, val);
		return ImColor::HSV(std::min(hue * multiplier, 1.0f), sat, val);
	}

	inline ImColor ColourWithMultipliedAlpha(const ImColor& color, float multiplier)
	{
		ImVec4 colRaw = color.Value;
		colRaw.w *= multiplier;
		return colRaw;
	}


	// TODO: move most of the functions in this header into the Draw namespace
	namespace Draw {
		//=========================================================================================
		/// Lines
		inline void Underline(bool fullWidth = false, float offsetX = 0.0f, float offsetY = -1.0f)
		{
			if (fullWidth)
			{
				if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr)
					ImGui::PushColumnsBackground();
				else if (ImGui::GetCurrentTable() != nullptr)
					ImGui::TablePushBackgroundChannel();
			}

			const float width = fullWidth ? ImGui::GetWindowWidth() : ImGui::GetContentRegionAvail().x;
			const ImVec2 cursor = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddLine(ImVec2(cursor.x + offsetX, cursor.y + offsetY),
				ImVec2(cursor.x + width, cursor.y + offsetY),
				Colors::Theme::backgroundDark, 1.0f);

			if (fullWidth)
			{
				if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr)
					ImGui::PopColumnsBackground();
				else if (ImGui::GetCurrentTable() != nullptr)
					ImGui::TablePopBackgroundChannel();
			}
		}
	}

	//=========================================================================================
	/// Rectangle

	inline ImRect GetItemRect()
	{
		return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	}

	inline ImRect RectExpanded(const ImRect& rect, float x, float y)
	{
		ImRect result = rect;
		result.Min.x -= x;
		result.Min.y -= y;
		result.Max.x += x;
		result.Max.y += y;
		return result;
	}

	inline ImRect RectOffset(const ImRect& rect, float x, float y)
	{
		ImRect result = rect;
		result.Min.x += x;
		result.Min.y += y;
		result.Max.x += x;
		result.Max.y += y;
		return result;
	}

	inline ImRect RectOffset(const ImRect& rect, ImVec2 xy)
	{
		return RectOffset(rect, xy.x, xy.y);
	}

	//=========================================================================================
	/// Window

	bool BeginPopup(const char* str_id, ImGuiWindowFlags flags = 0);
	void EndPopup();

	// MenuBar which allows you to specify its rectangle
	bool BeginMenuBar(const ImRect& barRectangle);
	void EndMenuBar();

	// Exposed to be used for window with disabled decorations
	// This border is going to be drawn even if window border size is set to 0.0f
	void RenderWindowOuterBorders(ImGuiWindow* window);

	// Exposed resize behavior for native OS windows
	bool UpdateWindowManualResize(ImGuiWindow* window, ImVec2& newSize, ImVec2& newPosition);

	// AKA ImGui::CollapsingHeader
	bool ContextMenuHeader(const char* label, ImGuiTreeNodeFlags flags = 0);

	//=========================================================================================
	/// Shadows

	void DrawShadow(const Ref<Texture2D>& shadowImage, int radius, ImVec2 rectMin, ImVec2 rectMax, float alphMultiplier = 1.0f, float lengthStretch = 10.0f,
		 bool drawLeft = true, bool drawRight = true, bool drawTop = true, bool drawBottom = true);

	void DrawShadow(const Ref<Texture2D>& shadowImage, int radius, ImRect rectangle, float alphMultiplier = 1.0f, float lengthStretch = 10.0f,
		 bool drawLeft = true, bool drawRight = true, bool drawTop = true, bool drawBottom = true);


	void DrawShadow(const Ref<Texture2D>& shadowImage, int radius, float alphMultiplier = 1.0f, float lengthStretch = 10.0f,
		 bool drawLeft = true, bool drawRight = true, bool drawTop = true, bool drawBottom = true);

	void DrawShadowInner(const Ref<Texture2D>& shadowImage, int radius, ImVec2 rectMin, ImVec2 rectMax, float alpha = 1.0f, float lengthStretch = 10.0f,
		 bool drawLeft = true, bool drawRight = true, bool drawTop = true, bool drawBottom = true);

	void DrawShadowInner(const Ref<Texture2D>& shadowImage, int radius, ImRect rectangle, float alpha = 1.0f, float lengthStretch = 10.0f,
		 bool drawLeft = true, bool drawRight = true, bool drawTop = true, bool drawBottom = true);


	void DrawShadowInner(const Ref<Texture2D>& shadowImage, int radius, float alpha = 1.0f, float lengthStretch = 10.0f,
		 bool drawLeft = true, bool drawRight = true, bool drawTop = true, bool drawBottom = true);


	//=========================================================================================
	// Outline

	inline void BeginDisabled(bool disabled = true)
	{
		if (disabled)
			ImGui::BeginDisabled(true);
	}

	inline bool IsItemDisabled()
	{
		return ImGui::GetItemFlags() & ImGuiItemFlags_Disabled;
	}

	inline void EndDisabled()
	{
		// NOTE(Peter): Cheeky hack to prevent ImGui from asserting (required due to the nature of UI::BeginDisabled)
		if (GImGui->DisabledStackSize > 0)
			ImGui::EndDisabled();
	}

	typedef int OutlineFlags;
	enum OutlineFlags_
	{
		OutlineFlags_None = 0,               // draw no activity outline
		OutlineFlags_WhenHovered = 1 << 1,          // draw an outline when item is hovered
		OutlineFlags_WhenActive = 1 << 2,          // draw an outline when item is active
		OutlineFlags_WhenInactive = 1 << 3,          // draw an outline when item is inactive
		OutlineFlags_HighlightActive = 1 << 4,           // when active, the outline is in highlight colour
		OutlineFlags_NoHighlightActive = OutlineFlags_WhenHovered | OutlineFlags_WhenActive | OutlineFlags_WhenInactive,
		OutlineFlags_NoOutlineInactive = OutlineFlags_WhenHovered | OutlineFlags_WhenActive | OutlineFlags_HighlightActive,
		OutlineFlags_All = OutlineFlags_WhenHovered | OutlineFlags_WhenActive | OutlineFlags_WhenInactive | OutlineFlags_HighlightActive,
	};

	inline void DrawItemActivityOutline(OutlineFlags flags = OutlineFlags_All, ImColor colourHighlight = Colors::Theme::accent, float rounding = GImGui->Style.FrameRounding)
	{
		if (IsItemDisabled())
			return;

		auto* drawList = ImGui::GetWindowDrawList();
		const ImRect rect = RectExpanded(GetItemRect(), 1.0f, 1.0f);
		if ((flags & OutlineFlags_WhenActive) && ImGui::IsItemActive())
		{
			if (flags & OutlineFlags_HighlightActive)
			{
				drawList->AddRect(rect.Min, rect.Max, colourHighlight, rounding, 0, 1.5f);
			}
			else
			{
				drawList->AddRect(rect.Min, rect.Max, ImColor(60, 60, 60), rounding, 0, 1.5f);
			}
		}
		else if ((flags & OutlineFlags_WhenHovered) && ImGui::IsItemHovered() && !ImGui::IsItemActive())
		{
			drawList->AddRect(rect.Min, rect.Max, ImColor(60, 60, 60), rounding, 0, 1.5f);
		}
		else if ((flags & OutlineFlags_WhenInactive) && !ImGui::IsItemHovered() && !ImGui::IsItemActive())
		{
			drawList->AddRect(rect.Min, rect.Max, ImColor(50, 50, 50), rounding, 0, 1.0f);
		}
	};


	//=========================================================================================
	/// Property Fields

	// GetColFunction function takes 'int' index of the option to display and returns ImColor
	template<typename GetColFunction>
	bool PropertyDropdown(const char* label, const std::vector<std::string>& options, int32_t optionCount, int32_t* selected,
		 GetColFunction getColourFunction)
	{
		const char* current = options[*selected].c_str();
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		bool changed = false;

		const std::string id = "##" + std::string(label);

		ImGui::PushStyleColor(ImGuiCol_Text, getColourFunction(*selected).Value);

		if (ImGui::BeginCombo(id.c_str(), current))
		{
			for (int i = 0; i < optionCount; i++)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, getColourFunction(i).Value);

				const bool is_selected = (current == options[i]);
				if (ImGui::Selectable(options[i].c_str(), is_selected))
				{
					current = options[i].c_str();
					*selected = i;
					changed = true;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();

				ImGui::PopStyleColor();
			}
			ImGui::EndCombo();

		}
		ImGui::PopStyleColor();

		UI::DrawItemActivityOutline();

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return changed;
	};

	inline void PopupMenuHeader(const std::string& text, bool indentAfter = true, bool unindendBefore = false)
	{
		if (unindendBefore) ImGui::Unindent();
		ImGui::TextColored(ImColor(170, 170, 170).Value, text.c_str());
		ImGui::Separator();
		if (indentAfter) ImGui::Indent();
	};


	//=========================================================================================
	/// Button Image

	inline void DrawButtonImage(const Ref<Texture2D>& imageNormal, const Ref<Texture2D>& imageHovered, const Ref<Texture2D>& imagePressed,
		 ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
		 ImVec2 rectMin, ImVec2 rectMax)
	{
		auto* drawList = ImGui::GetWindowDrawList();
		if (ImGui::IsItemActive())
			drawList->AddImage(GetTextureID(imagePressed), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintPressed);
		else if (ImGui::IsItemHovered())
			drawList->AddImage(GetTextureID(imageHovered), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintHovered);
		else
			drawList->AddImage(GetTextureID(imageNormal), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintNormal);
	};

	inline void DrawButtonImage(const Ref<Texture2D>& imageNormal, const Ref<Texture2D>& imageHovered, const Ref<Texture2D>& imagePressed,
		 ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
		 ImRect rectangle)
	{
		DrawButtonImage(imageNormal, imageHovered, imagePressed, tintNormal, tintHovered, tintPressed, rectangle.Min, rectangle.Max);
	};

	inline void DrawButtonImage(const Ref<Texture2D>& image,
		 ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
		 ImVec2 rectMin, ImVec2 rectMax)
	{
		DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, rectMin, rectMax);
	};

	inline void DrawButtonImage(const Ref<Texture2D>& image,
		 ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
		 ImRect rectangle)
	{
		DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, rectangle.Min, rectangle.Max);
	};


	inline void DrawButtonImage(const Ref<Texture2D>& imageNormal, const Ref<Texture2D>& imageHovered, const Ref<Texture2D>& imagePressed,
		 ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed)
	{
		DrawButtonImage(imageNormal, imageHovered, imagePressed, tintNormal, tintHovered, tintPressed, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	};

	inline void DrawButtonImage(const Ref<Texture2D>& image,
		 ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed)
	{
		DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	};


	//=========================================================================================
	/// Border

	inline void DrawBorder(ImVec2 rectMin, ImVec2 rectMax, const ImVec4& borderColour, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		auto min = rectMin;
		min.x -= thickness;
		min.y -= thickness;
		min.x += offsetX;
		min.y += offsetY;
		auto max = rectMax;
		max.x += thickness;
		max.y += thickness;
		max.x += offsetX;
		max.y += offsetY;

		auto* drawList = ImGui::GetWindowDrawList();
		drawList->AddRect(min, max, ImGui::ColorConvertFloat4ToU32(borderColour), 0.0f, 0, thickness);
	};

	inline void DrawBorder(const ImVec4& borderColour, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		DrawBorder(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), borderColour, thickness, offsetX, offsetY);
	};

	inline void DrawBorder(float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		DrawBorder(ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
	};

	inline void DrawBorder(ImVec2 rectMin, ImVec2 rectMax, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		DrawBorder(rectMin, rectMax, ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
	};

	inline void DrawBorder(ImRect rect, float thickness = 1.0f, float rounding = 0.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		auto min = rect.Min;
		min.x -= thickness;
		min.y -= thickness;
		min.x += offsetX;
		min.y += offsetY;
		auto max = rect.Max;
		max.x += thickness;
		max.y += thickness;
		max.x += offsetX;
		max.y += offsetY;

		auto* drawList = ImGui::GetWindowDrawList();
		drawList->AddRect(min, max, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Border)), rounding, 0, thickness);
	};

	inline void DrawBorderHorizontal(ImVec2 rectMin, ImVec2 rectMax, const ImVec4& borderColour, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		auto min = rectMin;
		min.y -= thickness;
		min.x += offsetX;
		min.y += offsetY;
		auto max = rectMax;
		max.y += thickness;
		max.x += offsetX;
		max.y += offsetY;

		auto* drawList = ImGui::GetWindowDrawList();
		const auto colour = ImGui::ColorConvertFloat4ToU32(borderColour);
		drawList->AddLine(min, ImVec2(max.x, min.y), colour, thickness);
		drawList->AddLine(ImVec2(min.x, max.y), max, colour, thickness);
	};

	inline void DrawBorderHorizontal(ImVec2 rectMin, ImVec2 rectMax, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		DrawBorderHorizontal(rectMin, rectMax, ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
	};

	inline void DrawBorderHorizontal(const ImVec4& borderColour, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		DrawBorderHorizontal(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), borderColour, thickness, offsetX, offsetY);
	};

	inline void DrawBorderHorizontal(float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		DrawBorderHorizontal(ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
	};

	inline void DrawBorderVertical(ImVec2 rectMin, ImVec2 rectMax, const ImVec4& borderColour, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		auto min = rectMin;
		min.x -= thickness;
		min.x += offsetX;
		min.y += offsetY;
		auto max = rectMax;
		max.x += thickness;
		max.x += offsetX;
		max.y += offsetY;

		auto* drawList = ImGui::GetWindowDrawList();
		const auto colour = ImGui::ColorConvertFloat4ToU32(borderColour);
		drawList->AddLine(min, ImVec2(min.x, max.y), colour, thickness);
		drawList->AddLine(ImVec2(max.x, min.y), max, colour, thickness);
	};

	inline void DrawBorderVertical(const ImVec4& borderColour, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		DrawBorderVertical(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), borderColour, thickness, offsetX, offsetY);
	};

	inline void DrawBorderVertical(float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		DrawBorderVertical(ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
	};


	//=========================================================================================
	/// Custom IMGui controls

	inline const char* PatchFormatStringFloatToInt(const char* fmt)
	{
		if (fmt[0] == '%' && fmt[1] == '.' && fmt[2] == '0' && fmt[3] == 'f' && fmt[4] == 0) // Fast legacy path for "%.0f" which is expected to be the most common case.
			return "%d";
		const char* fmt_start = ImParseFormatFindStart(fmt);    // Find % (if any, and ignore %%)
		const char* fmt_end = ImParseFormatFindEnd(fmt_start);  // Find end of format specifier, which itself is an exercise of confidence/recklessness (because snprintf is dependent on libc or user).
		if (fmt_end > fmt_start && fmt_end[-1] == 'f')
		{
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
			if (fmt_start == fmt && fmt_end[0] == 0)
				return "%d";
			ImGuiContext& g = *GImGui;
			ImFormatString(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), "%.*s%%d%s", (int)(fmt_start - fmt), fmt, fmt_end); // Honor leading and trailing decorations, but lose alignment/precision.
			return g.TempBuffer;
#else
			IM_ASSERT(0 && "DragInt(): Invalid format string!"); // Old versions used a default parameter of "%.0f", please replace with e.g. "%d"
#endif
		}
		return fmt;
	}

	inline int FormatString(char* buf, size_t buf_size, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
#ifdef IMGUI_USE_STB_SPRINTF
		int w = stbsp_vsnprintf(buf, (int)buf_size, fmt, args);
#else
		int w = vsnprintf(buf, buf_size, fmt, args);
#endif
		va_end(args);
		if (buf == NULL)
			return w;
		if (w == -1 || w >= (int)buf_size)
			w = (int)buf_size - 1;
		buf[w] = 0;
		return w;
	}

	inline bool DragScalar(const char* label, ImGuiDataType data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const float w = ImGui::CalcItemWidth();

		const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
		const ImRect frame_bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + w, window->DC.CursorPos.y + (label_size.y + style.FramePadding.y * 2.0f)));
		const ImRect total_bb(frame_bb.Min, ImVec2(frame_bb.Max.x + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), frame_bb.Max.y));

		const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
		ImGui::ItemSize(total_bb, style.FramePadding.y);
		if (!ImGui::ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
			return false;

		// Default format string when passing NULL
		if (format == NULL)
			format = ImGui::DataTypeGetInfo(data_type)->PrintFmt;
		else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0) // (FIXME-LEGACY: Patch old "%.0f" format string to use "%d", read function more details.)
			format = PatchFormatStringFloatToInt(format);

		// Tabbing or CTRL-clicking on Drag turns it into an InputText
		const bool hovered = ImGui::ItemHoverable(frame_bb, id);
		bool temp_input_is_active = temp_input_allowed && ImGui::TempInputIsActive(id);
		if (!temp_input_is_active)
		{
			const bool input_requested_by_tabbing = temp_input_allowed && (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
			const bool clicked = (hovered && g.IO.MouseClicked[0]);
			const bool double_clicked = (hovered && g.IO.MouseClickedCount[0] == 2);
			if (input_requested_by_tabbing || clicked || double_clicked || g.NavActivateId == id || g.NavActivateInputId == id)
			{
				ImGui::SetActiveID(id, window);
				ImGui::SetFocusID(id, window);
				ImGui::FocusWindow(window);
				g.ActiveIdUsingNavDirMask = (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
				if (temp_input_allowed)
					if (input_requested_by_tabbing || (clicked && g.IO.KeyCtrl) || double_clicked || g.NavActivateInputId == id)
						temp_input_is_active = true;
			}

			// Experimental: simple click (without moving) turns Drag into an InputText
			if (g.IO.ConfigDragClickToInputText && temp_input_allowed && !temp_input_is_active)
				if (g.ActiveId == id && hovered && g.IO.MouseReleased[0] && !ImGui::IsMouseDragPastThreshold(0, g.IO.MouseDragThreshold * 0.5f))
				{
					g.NavActivateId = g.NavActivateInputId = id;
					g.NavActivateFlags = ImGuiActivateFlags_PreferInput;
					temp_input_is_active = true;
				}
		}

		if (temp_input_is_active)
		{
			// Only clamp CTRL+Click input when ImGuiSliderFlags_AlwaysClamp is set
			const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0 && (p_min == NULL || p_max == NULL || ImGui::DataTypeCompare(data_type, p_min, p_max) < 0);
			return ImGui::TempInputScalar(frame_bb, id, label, data_type, p_data, format, is_clamp_input ? p_min : NULL, is_clamp_input ? p_max : NULL);
		}

		// Draw frame
		const ImU32 frame_col = ImGui::GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
		ImGui::RenderNavHighlight(frame_bb, id);
		ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, style.FrameRounding);

		// Drag behavior
		const bool value_changed = ImGui::DragBehavior(id, data_type, p_data, v_speed, p_min, p_max, format, flags);
		if (value_changed)
			ImGui::MarkItemEdited(id);

		const bool mixed_value = (g.CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0;

		// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
		char value_buf[64];
		const char* value_buf_end = value_buf + (mixed_value ? FormatString(value_buf, IM_ARRAYSIZE(value_buf), "---") : ImGui::DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format));
		if (g.LogEnabled)
			ImGui::LogSetNextTextDecoration("{", "}");
		ImGui::RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

		if (label_size.x > 0.0f)
			ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

		DrawItemActivityOutline();

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
		return value_changed;
	}

	inline bool DragScalarN(const char* label, ImGuiDataType data_type, void* p_data, int components, float v_speed = 1.0f, const void* p_min = NULL, const void* p_max = NULL, const char* format = NULL, ImGuiSliderFlags flags = 0)
	{
		bool changed = ImGui::DragScalarN(label, data_type, p_data, components, v_speed, p_min, p_max, format, flags);
		DrawItemActivityOutline();
		return changed;
	}

	inline bool DragFloat(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_Float, v, v_speed, &v_min, &v_max, format, flags);
	}

	inline bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
	{
		bool changed = ImGui::SliderFloat(label, v, v_min, v_max, format, flags);
		DrawItemActivityOutline();
		return changed;
	}

	inline bool DragFloat2(const char* label, float v[2], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
	{
		bool changed = ImGui::DragFloat2(label, v, v_speed, v_min, v_max, format, flags);
		DrawItemActivityOutline();
		return changed;
	}

	inline bool SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
	{
		bool changed = ImGui::SliderFloat2(label, v, v_min, v_max, format, flags);
		DrawItemActivityOutline();
		return changed;
	}

	inline bool DragFloat3(const char* label, float v[3], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
	{
		bool changed = ImGui::DragFloat3(label, v, v_speed, v_min, v_max, format, flags);
		DrawItemActivityOutline();
		return changed;
	}

	inline bool SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
		bool changed = ImGui::SliderFloat3(label, v, v_min, v_max, format, flags);
		DrawItemActivityOutline();
		return changed;
	}

	inline bool DragFloat4(const char* label, float v[4], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
	{
		bool changed = ImGui::DragFloat4(label, v, v_speed, v_min, v_max, format, flags);
		DrawItemActivityOutline();
		return changed;
	}
	
	inline bool SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
		bool changed = ImGui::SliderFloat4(label, v, v_min, v_max, format, flags);
		DrawItemActivityOutline();
		return changed;
	}

	inline bool DragDouble(const char* label, double* v, float v_speed = 1.0f, double v_min = 0.0, double v_max = 0.0, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_Double, v, v_speed, &v_min, &v_max, format, flags);
	}

	inline bool DragInt8(const char* label, int8_t* v, float v_speed = 1.0f, int8_t v_min = 0, int8_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_S8, v, v_speed, &v_min, &v_max, format, flags);
	}

	inline bool DragInt16(const char* label, int16_t* v, float v_speed = 1.0f, int16_t v_min = 0, int16_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_S16, v, v_speed, &v_min, &v_max, format, flags);
	}

	inline bool DragInt32(const char* label, int32_t* v, float v_speed = 1.0f, int32_t v_min = 0, int32_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_S32, v, v_speed, &v_min, &v_max, format, flags);
	}

	inline bool SliderInt32(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0)
	{
		bool changed = ImGui::SliderInt(label, v, v_min, v_max, format, flags);
		DrawItemActivityOutline();
		return changed;
	}

	inline bool DragInt64(const char* label, int64_t* v, float v_speed = 1.0f, int64_t v_min = 0, int64_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_S64, v, v_speed, &v_min, &v_max, format, flags);
	}

	inline bool DragUInt8(const char* label, uint8_t* v, float v_speed = 1.0f, uint8_t v_min = 0, uint8_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_U8, v, v_speed, &v_min, &v_max, format, flags);
	}

	inline bool DragUInt16(const char* label, uint16_t* v, float v_speed = 1.0f, uint16_t v_min = 0, uint16_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_U16, v, v_speed, &v_min, &v_max, format, flags);
	}

	inline bool DragUInt32(const char* label, uint32_t* v, float v_speed = 1.0f, uint32_t v_min = 0, uint32_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_U32, v, v_speed, &v_min, &v_max, format, flags);
	}

	inline bool DragUInt64(const char* label, uint64_t* v, float v_speed = 1.0f, uint64_t v_min = 0, uint64_t v_max = 0, const char* format = nullptr, ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_U64, v, v_speed, &v_min, &v_max, format, flags);
	}

	inline bool InputScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_step = NULL, const void* p_step_fast = NULL, const char* format = NULL, ImGuiInputTextFlags flags = 0)
	{
		bool changed = ImGui::InputScalar(label, data_type, p_data, p_step, p_step_fast, format, flags);
		DrawItemActivityOutline();
		return changed;
	}

	inline bool InputFloat(const char* label, float* v, float step = 0.0f, float step_fast = 0.0f, const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
	{
		return InputScalar(label, ImGuiDataType_Float, v, &step, &step_fast, format, flags);
	}

	inline bool InputDouble(const char* label, double* v, double step = 0.0, double step_fast = 0.0, const char* format = "%.6f", ImGuiInputTextFlags flags = 0)
	{
		return InputScalar(label, ImGuiDataType_Double, v, &step, &step_fast, format, flags);
	}

	inline bool InputInt8(const char* label, int8_t* v, int8_t step = 1, int8_t step_fast = 1, ImGuiInputTextFlags flags = 0)
	{
		return InputScalar(label, ImGuiDataType_S8, v, &step, &step_fast, nullptr, flags);
	}

	inline bool InputInt16(const char* label, int16_t* v, int16_t step = 1, int16_t step_fast = 10, ImGuiInputTextFlags flags = 0)
	{
		return InputScalar(label, ImGuiDataType_S16, v, &step, &step_fast, nullptr, flags);
	}

	inline bool InputInt32(const char* label, int32_t* v, int32_t step = 1, int32_t step_fast = 100, ImGuiInputTextFlags flags = 0)
	{
		return InputScalar(label, ImGuiDataType_S32, v, &step, &step_fast, nullptr, flags);
	}

	inline bool InputInt64(const char* label, int64_t* v, int64_t step = 1, int64_t step_fast = 1000, ImGuiInputTextFlags flags = 0)
	{
		return InputScalar(label, ImGuiDataType_S64, v, &step, &step_fast, nullptr, flags);
	}

	inline bool InputUInt8(const char* label, uint8_t* v, uint8_t step = 1, uint8_t step_fast = 1, ImGuiInputTextFlags flags = 0)
	{
		return InputScalar(label, ImGuiDataType_U8, v, &step, &step_fast, nullptr, flags);
	}

	inline bool InputUInt16(const char* label, uint16_t* v, uint16_t step = 1, uint16_t step_fast = 10, ImGuiInputTextFlags flags = 0)
	{
		return InputScalar(label, ImGuiDataType_U16, v, &step, &step_fast, nullptr, flags);
	}

	inline bool InputUInt32(const char* label, uint32_t* v, uint32_t step = 1, uint32_t step_fast = 100, ImGuiInputTextFlags flags = 0)
	{
		return InputScalar(label, ImGuiDataType_U32, v, &step, &step_fast, nullptr, flags);
	}

	inline bool InputUInt64(const char* label, uint64_t* value, ImGuiInputTextFlags flags = 0)
	{
		return InputScalar(label, ImGuiDataType_U64, value, nullptr, nullptr, nullptr, flags);
	}

	inline bool InputUInt64(const char* label, uint64_t* v, uint64_t step = 0, uint64_t step_fast = 0, ImGuiInputTextFlags flags = 0)
	{
		return InputScalar(label, ImGuiDataType_U64, v, step ? &step : nullptr, step_fast ? &step_fast : nullptr, nullptr, flags);
	}

	inline bool InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL)
	{
		bool changed = ImGui::InputText(label, buf, buf_size, flags, callback, user_data);
		DrawItemActivityOutline();
		return changed;
	}

	inline bool InputText(const char* label, std::string* value, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL)
	{
		bool changed = ImGui::InputText(label, value, flags, callback, user_data);
		DrawItemActivityOutline();
		return changed;
	}

	inline bool InputTextMultiline(const char* label, std::string* value, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL)
	{
		bool changed = ImGui::InputTextMultiline(label, value, size, flags, callback, user_data);
		DrawItemActivityOutline();
		return changed;
	}

	inline bool ColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags = 0)
	{
		bool changed = ImGui::ColorEdit3(label, col, flags);
		DrawItemActivityOutline();
		return changed;
	}

	inline bool ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags = 0)
	{
		bool changed = ImGui::ColorEdit4(label, col, flags);
		DrawItemActivityOutline();
		return changed;
	}

	inline bool BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0)
	{
		bool opened = ImGui::BeginCombo(label, preview_value, flags);
		DrawItemActivityOutline();
		return opened;
	}

	inline void EndCombo()
	{
		ImGui::EndCombo();
	}

	bool FakeComboBoxButton(const char* label, const char* preview_value, ImGuiComboFlags flags = 0);


	inline bool Checkbox(const char* label, bool* b)
	{
		bool changed = ImGui::Checkbox(label, b);
		UI::DrawItemActivityOutline();
		return changed;
	}


	inline bool Hyperlink(const char* label, ImU32 lineColor = Colors::Theme::accent, float lineThickness = GImGui->Style.FrameBorderSize)
	{
		ImGui::Text(label);
		const ImRect rect = RectExpanded(GetItemRect(), lineThickness, lineThickness);
		if (ImGui::IsItemHovered())
		{
			ImGui::GetWindowDrawList()->AddLine({ rect.Min.x, rect.Max.y }, rect.Max, lineColor, lineThickness);
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			return ImGui::IsMouseReleased(ImGuiMouseButton_Left);
		}
		return false;
	}

} // namespace Hazel::UI

#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#include "imgui_internal.h"

#include <vector>
#include <string>
#include <memory>

#include "Image.h"

namespace Hazel::UI
{
	//=========================================================================================
	/// Utilities
	class ScopedStyle
	{
	public:
		ScopedStyle(const ScopedStyle&) = delete;
		ScopedStyle operator=(const ScopedStyle&) = delete;
		template<typename T>
		ScopedStyle(ImGuiStyleVar styleVar, T value) { ImGui::PushStyleVar(styleVar, value); }
		~ScopedStyle() { ImGui::PopStyleVar(); }
	};

	class ScopedColour
	{
	public:
		ScopedColour(const ScopedColour&) = delete;
		ScopedColour operator=(const ScopedColour&) = delete;
		template<typename T>
		ScopedColour(ImGuiCol colourId, T colour) { ImGui::PushStyleColor(colourId, colour); }
		~ScopedColour() { ImGui::PopStyleColor(); }
	};

	class ScopedFont
	{
	public:
		ScopedFont(const ScopedFont&) = delete;
		ScopedFont operator=(const ScopedFont&) = delete;
		ScopedFont(ImFont* font) { ImGui::PushFont(font); }
		~ScopedFont() { ImGui::PopFont(); }
	};

	class ScopedID
	{
	public:
		ScopedID(const ScopedID&) = delete;
		ScopedID operator=(const ScopedID&) = delete;
		template<typename T>
		ScopedID(T id) { ImGui::PushID(id); }
		~ScopedID() { ImGui::PopID(); }
	};

	class ScopedColourStack
	{
	public:
		ScopedColourStack(const ScopedColourStack&) = delete;
		ScopedColourStack operator=(const ScopedColourStack&) = delete;

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
				ImGui::PushStyleColor(colourID, colour);
			}
			else
			{
				ImGui::PushStyleColor(colourID, colour);
				PushColour(std::forward<OtherColours>(otherColourPairs)...);
			}
		}
	};

	class ScopedStyleStack
	{
	public:
		ScopedStyleStack(const ScopedStyleStack&) = delete;
		ScopedStyleStack operator=(const ScopedStyleStack&) = delete;

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
		ScopedItemFlags operator=(const ScopedItemFlags&) = delete;
		ScopedItemFlags(const ImGuiItemFlags flags, const bool enable=true) { ImGui::PushItemFlag(flags, enable); }
		~ScopedItemFlags() { ImGui::PopItemFlag(); }
	};


	// The delay won't work on texts, because the timer isn't tracked for them.
	static bool IsItemHovered(float delayInSeconds = 0.1f, ImGuiHoveredFlags flags = 0)
	{
		return ImGui::IsItemHovered() && GImGui->HoveredIdTimer > delayInSeconds; /*HoveredIdNotActiveTimer*/
	}

	// Check if navigated to current item, e.g. with arrow keys
	static bool NavigatedTo()
	{
		ImGuiContext& g = *GImGui;
		return g.NavJustMovedToId == g.LastItemData.ID;
	}


	//=========================================================================================
	/// Colours

	static ImU32 ColourWithValue(const ImColor& color, float value)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(hue, sat, std::min(value, 1.0f));
	}

	static ImU32 ColourWithSaturation(const ImColor& color, float saturation)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(hue, std::min(saturation, 1.0f), val);
	}

	static ImU32 ColourWithHue(const ImColor& color, float hue)
	{
		const ImVec4& colRow = color.Value;
		float h, s, v;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, h, s, v);
		return ImColor::HSV(std::min(hue, 1.0f), s, v);
	}

	static ImU32 ColourWithMultipliedValue(const ImColor& color, float multiplier)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(hue, sat, std::min(val * multiplier, 1.0f));
	}

	static ImU32 ColourWithMultipliedSaturation(const ImColor& color, float multiplier)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(hue, std::min(sat * multiplier, 1.0f), val);
	}

	static ImU32 ColourWithMultipliedHue(const ImColor& color, float multiplier)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(std::min(hue * multiplier, 1.0f), sat, val);
	}


	//=========================================================================================
	/// Rectangle

	static inline ImRect GetItemRect()
	{
		return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	}

	static inline ImRect RectExpanded(const ImRect& rect, float x, float y)
	{
		ImRect result = rect;
		result.Min.x -= x;
		result.Min.y -= y;
		result.Max.x += x;
		result.Max.y += y;
		return result;
	}

	static inline ImRect RectOffset(const ImRect& rect, float x, float y)
	{
		ImRect result = rect;
		result.Min.x += x;
		result.Min.y += y;
		result.Max.x += x;
		result.Max.y += y;
		return result;
	}

	static inline ImRect RectOffset(const ImRect& rect, ImVec2 xy)
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

	//=========================================================================================
	/// Property Fields

	// GetColFunction function takes 'int' index of the option to display and returns ImColor
	template<typename GetColFunction>
	static bool PropertyDropdown(const char* label, const std::vector<std::string>& options, int32_t optionCount, int32_t* selected,
		 GetColFunction getColourFunction)
	{
		const char* current = options[*selected].c_str();
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		auto drawItemActivityOutline = [](float rounding = 0.0f, bool drawWhenInactive = false)
		{
			auto* drawList = ImGui::GetWindowDrawList();
			if (ImGui::IsItemHovered() && !ImGui::IsItemActive())
			{
				drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
					 ImColor(60, 60, 60), rounding, 0, 1.5f);
			}
			if (ImGui::IsItemActive())
			{
				drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
					 ImColor(80, 80, 80), rounding, 0, 1.0f);
			}
			else if (!ImGui::IsItemHovered() && drawWhenInactive)
			{
				drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
					 ImColor(50, 50, 50), rounding, 0, 1.0f);
			}
		};

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

		drawItemActivityOutline(2.5f);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return changed;
	};

	static void PopupMenuHeader(const std::string& text, bool indentAfter = true, bool unindendBefore = false)
	{
		if (unindendBefore) ImGui::Unindent();
		ImGui::TextColored(ImColor(170, 170, 170).Value, text.c_str());
		ImGui::Separator();
		if (indentAfter) ImGui::Indent();
	};


	//=========================================================================================
	/// Button Image

	static void DrawButtonImage(const std::shared_ptr<Walnut::Image>& imageNormal, const std::shared_ptr<Walnut::Image>& imageHovered, const std::shared_ptr<Walnut::Image>& imagePressed,
		 ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
		 ImVec2 rectMin, ImVec2 rectMax)
	{
		auto* drawList = ImGui::GetForegroundDrawList();
		if (ImGui::IsItemActive())
			drawList->AddImage(imagePressed->GetDescriptorSet(), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintPressed);
		else if (ImGui::IsItemHovered())
			drawList->AddImage(imagePressed->GetDescriptorSet(), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintHovered);
		else
			drawList->AddImage(imagePressed->GetDescriptorSet(), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintNormal);
	};

	static void DrawButtonImage(const std::shared_ptr<Walnut::Image>& imageNormal, const std::shared_ptr<Walnut::Image>& imageHovered, const std::shared_ptr<Walnut::Image>& imagePressed,
		 ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
		 ImRect rectangle)
	{
		DrawButtonImage(imageNormal, imageHovered, imagePressed, tintNormal, tintHovered, tintPressed, rectangle.Min, rectangle.Max);
	};

	static void DrawButtonImage(const std::shared_ptr<Walnut::Image>& image,
		 ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
		 ImVec2 rectMin, ImVec2 rectMax)
	{
		DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, rectMin, rectMax);
	};

	static void DrawButtonImage(const std::shared_ptr<Walnut::Image>& image,
		 ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
		 ImRect rectangle)
	{
		DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, rectangle.Min, rectangle.Max);
	};


	static void DrawButtonImage(const std::shared_ptr<Walnut::Image>& imageNormal, const std::shared_ptr<Walnut::Image>& imageHovered, const std::shared_ptr<Walnut::Image>& imagePressed,
		 ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed)
	{
		DrawButtonImage(imageNormal, imageHovered, imagePressed, tintNormal, tintHovered, tintPressed, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	};

	static void DrawButtonImage(const std::shared_ptr<Walnut::Image>& image,
		 ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed)
	{
		DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	};

	// TODO: move most of the functions in this header into the Draw namespace
	namespace Draw {
		//=========================================================================================
		/// Lines
		static void Underline(bool fullWidth = false, float offsetX = 0.0f, float offsetY = -1.0f)
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
				IM_COL32(26, 26, 26, 255), 1.0f);

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
	/// Border

	static void DrawBorder(ImVec2 rectMin, ImVec2 rectMax, const ImVec4& borderColour, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
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

	static void DrawBorder(const ImVec4& borderColour, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		DrawBorder(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), borderColour, thickness, offsetX, offsetY);
	};

	static void DrawBorder(float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		DrawBorder(ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
	};

	static void DrawBorder(ImVec2 rectMin, ImVec2 rectMax, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		DrawBorder(rectMin, rectMax, ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
	};
	static void DrawBorder(ImRect rect, float thickness = 1.0f, float rounding = 0.0f, float offsetX = 0.0f, float offsetY = 0.0f)
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

	static void DrawBorderHorizontal(ImVec2 rectMin, ImVec2 rectMax, const ImVec4& borderColour, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
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

	static void DrawBorderHorizontal(ImVec2 rectMin, ImVec2 rectMax, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		DrawBorderHorizontal(rectMin, rectMax, ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
	};

	static void DrawBorderHorizontal(const ImVec4& borderColour, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		DrawBorderHorizontal(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), borderColour, thickness, offsetX, offsetY);
	};

	static void DrawBorderHorizontal(float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		DrawBorderHorizontal(ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
	};

	static void DrawBorderVertical(ImVec2 rectMin, ImVec2 rectMax, const ImVec4& borderColour, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
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

	static void DrawBorderVertical(const ImVec4& borderColour, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		DrawBorderVertical(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), borderColour, thickness, offsetX, offsetY);
	};

	static void DrawBorderVertical(float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
	{
		DrawBorderVertical(ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
	};

	static void DrawItemActivityOutline(float rounding = 0.0f, bool drawWhenInactive = false, ImColor colourWhenActive = ImColor(80, 80, 80))
	{
		auto* drawList = ImGui::GetWindowDrawList();
		const ImRect rect = RectExpanded(GetItemRect(), 1.0f, 1.0f);
		if (ImGui::IsItemHovered() && !ImGui::IsItemActive())
		{
			drawList->AddRect(rect.Min, rect.Max,
				 ImColor(60, 60, 60), rounding, 0, 1.5f);
		}
		if (ImGui::IsItemActive())
		{
			drawList->AddRect(rect.Min, rect.Max,
				 colourWhenActive, rounding, 0, 1.0f);
		}
		else if (!ImGui::IsItemHovered() && drawWhenInactive)
		{
			drawList->AddRect(rect.Min, rect.Max,
				 ImColor(50, 50, 50), rounding, 0, 1.0f);
		}
	};


	//=========================================================================================
	/// Cursor

	static void ShiftCursorX(float distance)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + distance);
	}

	static void ShiftCursorY(float distance)
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + distance);
	}

	static void ShiftCursor(float x, float y)
	{
		const ImVec2 cursor = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(cursor.x + x, cursor.y + y));
	}

	//=========================================================================================
	/// Custom Drag*** / Input*** / Slider***

	static const char* PatchFormatStringFloatToInt(const char* fmt)
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

	static int FormatString(char* buf, size_t buf_size, const char* fmt, ...)
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

	static bool DragScalar(const char* label, ImGuiDataType data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
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

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
		return value_changed;
	}

	static bool DragFloat(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_Float, v, v_speed, &v_min, &v_max, format, flags);
	}

	static bool DragDouble(const char* label, double* v, float v_speed = 1.0f, double v_min = 0.0, double v_max = 0.0, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_Double, v, v_speed, &v_min, &v_max, format, flags);
	}

	static bool DragInt8(const char* label, int8_t* v, float v_speed = 1.0f, int8_t v_min = 0, int8_t v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_S8, v, v_speed, &v_min, &v_max, format, flags);
	}

	static bool DragInt16(const char* label, int16_t* v, float v_speed = 1.0f, int16_t v_min = 0, int16_t v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_S16, v, v_speed, &v_min, &v_max, format, flags);
	}

	static bool DragInt32(const char* label, int32_t* v, float v_speed = 1.0f, int32_t v_min = 0, int32_t v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_S32, v, v_speed, &v_min, &v_max, format, flags);
	}

	static bool DragInt64(const char* label, int64_t* v, float v_speed = 1.0f, int64_t v_min = 0, int64_t v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_S64, v, v_speed, &v_min, &v_max, format, flags);
	}

	static bool DragUInt8(const char* label, uint8_t* v, float v_speed = 1.0f, uint8_t v_min = 0, uint8_t v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_U8, v, v_speed, &v_min, &v_max, format, flags);
	}

	static bool DragUInt16(const char* label, uint16_t* v, float v_speed = 1.0f, uint16_t v_min = 0, uint16_t v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_U16, v, v_speed, &v_min, &v_max, format, flags);
	}

	static bool DragUInt32(const char* label, uint32_t* v, float v_speed = 1.0f, uint32_t v_min = 0, uint32_t v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_U32, v, v_speed, &v_min, &v_max, format, flags);
	}

	static bool DragUInt64(const char* label, uint64_t* v, float v_speed = 1.0f, uint64_t v_min = 0, uint64_t v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0)
	{
		return DragScalar(label, ImGuiDataType_U64, v, v_speed, &v_min, &v_max, format, flags);
	}

	static bool InputFloat(const char* label, float* v, float step = 0.0f, float step_fast = 0.0f, const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
	{
		return ImGui::InputScalar(label, ImGuiDataType_Float, v, &step, &step_fast, format, flags);
	}

	static bool InputDouble(const char* label, double* v, double step = 0.0, double step_fast = 0.0, const char* format = "%.6f", ImGuiInputTextFlags flags = 0)
	{
		return ImGui::InputScalar(label, ImGuiDataType_Double, v, &step, &step_fast, format, flags);
	}

	static bool InputInt8(const char* label, int8_t* v, int8_t step = 1, int8_t step_fast = 1, ImGuiInputTextFlags flags = 0)
	{
		return ImGui::InputScalar(label, ImGuiDataType_S8, v, &step, &step_fast, "%d", flags);
	}

	static bool InputInt16(const char* label, int16_t* v, int16_t step = 1, int16_t step_fast = 10, ImGuiInputTextFlags flags = 0)
	{
		return ImGui::InputScalar(label, ImGuiDataType_S16, v, &step, &step_fast, "%d", flags);
	}

	static bool InputInt32(const char* label, int32_t* v, int32_t step = 1, int32_t step_fast = 100, ImGuiInputTextFlags flags = 0)
	{
		return ImGui::InputScalar(label, ImGuiDataType_S32, v, &step, &step_fast, "%d", flags);
	}

	static bool InputInt64(const char* label, int64_t* v, int64_t step = 1, int64_t step_fast = 1000, ImGuiInputTextFlags flags = 0)
	{
		return ImGui::InputScalar(label, ImGuiDataType_S64, v, &step, &step_fast, "%d", flags);
	}

	static bool InputUInt8(const char* label, uint8_t* v, uint8_t step = 1, uint8_t step_fast = 1, ImGuiInputTextFlags flags = 0)
	{
		return ImGui::InputScalar(label, ImGuiDataType_U8, v, &step, &step_fast, "%d", flags);
	}

	static bool InputUInt16(const char* label, uint16_t* v, uint16_t step = 1, uint16_t step_fast = 10, ImGuiInputTextFlags flags = 0)
	{
		return ImGui::InputScalar(label, ImGuiDataType_U16, v, &step, &step_fast, "%d", flags);
	}

	static bool InputUInt32(const char* label, uint32_t* v, uint32_t step = 1, uint32_t step_fast = 100, ImGuiInputTextFlags flags = 0)
	{
		return ImGui::InputScalar(label, ImGuiDataType_U32, v, &step, &step_fast, "%d", flags);
	}

	static bool InputUInt64(const char* label, uint64_t* v, uint64_t step = 1, uint64_t step_fast = 1000, ImGuiInputTextFlags flags = 0)
	{
		return ImGui::InputScalar(label, ImGuiDataType_U64, v, &step, &step_fast, "%d", flags);
	}

} // namespace Hazel::UI

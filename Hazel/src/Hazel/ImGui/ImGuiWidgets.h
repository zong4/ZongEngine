#pragma once

#include "UICore.h"
#include "ImGuiUtilities.h"
#include "Hazel/Core/Input.h"
#include "Hazel/Script/ScriptEngine.h"
#include "Hazel/Editor/EditorResources.h"
#include "Hazel/Scene/Scene.h"
#include "Hazel/Utilities/StringUtils.h"

#include "choc/text/choc_StringUtilities.h"

#include <magic_enum.hpp>
using namespace magic_enum::bitwise_operators;

namespace Hazel::UI
{
	enum class VectorAxis
	{
		None = 0,
		X = BIT(0),
		Y = BIT(1),
		Z = BIT(2),
		W = BIT(3)
	};

	static bool IsMatchingSearch(const std::string& item, std::string_view searchQuery, bool caseSensitive = false, bool stripWhiteSpaces = false, bool stripUnderscores = false)
	{
		if (searchQuery.empty())
			return true;

		if (item.empty())
			return false;

		std::string itemSanitized = stripUnderscores ? choc::text::replace(item, "_", " ") : item;

		if (stripWhiteSpaces)
			itemSanitized = choc::text::replace(itemSanitized, " ", "");

		std::string searchString = stripWhiteSpaces ? choc::text::replace(searchQuery, " ", "") : std::string(searchQuery);

		if (!caseSensitive)
		{
			itemSanitized = Utils::String::ToLower(itemSanitized);
			searchString = Utils::String::ToLower(searchString);
		}

		bool result = false;
		if (choc::text::contains(searchString, " "))
		{
			std::vector<std::string> searchTerms = choc::text::splitAtWhitespace(searchString);
			for (const auto& searchTerm : searchTerms)
			{
				if (!searchTerm.empty() && choc::text::contains(itemSanitized, searchTerm))
					result = true;
				else
				{
					result = false;
					break;
				}
			}
		}
		else
		{
			result = choc::text::contains(itemSanitized, searchString);
		}

		return result;
	}

	class Widgets
	{
	public:
		template<uint32_t BuffSize = 256, typename StringType>
		static bool SearchWidget(StringType& searchString, const char* hint = "Search...", bool* grabFocus = nullptr)
		{
			PushID();

			ShiftCursorY(1.0f);

			const bool layoutSuspended = []
			{
				ImGuiWindow* window = ImGui::GetCurrentWindow();
				if (window->DC.CurrentLayout)
				{
					ImGui::SuspendLayout();
					return true;
				}
				return false;
			}();

			bool modified = false;
			bool searching = false;

			const float areaPosX = ImGui::GetCursorPosX();
			const float framePaddingY = ImGui::GetStyle().FramePadding.y;

			UI::ScopedStyle rounding(ImGuiStyleVar_FrameRounding, 3.0f);
			UI::ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(28.0f, framePaddingY));

			if constexpr (std::is_same<StringType, std::string>::value)
			{
				char searchBuffer[BuffSize + 1]{};
				strncpy(searchBuffer, searchString.c_str(), BuffSize);
				if (ImGui::InputText(GenerateID(), searchBuffer, BuffSize))
				{
					searchString = searchBuffer;
					modified = true;
				}
				else if (ImGui::IsItemDeactivatedAfterEdit())
				{
					searchString = searchBuffer;
					modified = true;
				}

				searching = searchBuffer[0] != 0;
			}
			else
			{
				static_assert(std::is_same<decltype(&searchString[0]), char*>::value,
					"searchString paramenter must be std::string& or char*");

				if (ImGui::InputText(GenerateID(), searchString, BuffSize))
				{
					modified = true;
				}
				else if (ImGui::IsItemDeactivatedAfterEdit())
				{
					modified = true;
				}

				searching = searchString[0] != 0;
			}

			if (grabFocus && *grabFocus)
			{
				if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)
					&& !ImGui::IsAnyItemActive()
					&& !ImGui::IsMouseClicked(0))
				{
					ImGui::SetKeyboardFocusHere(-1);
				}

				if (ImGui::IsItemFocused())
					*grabFocus = false;
			}

			UI::DrawItemActivityOutline();
			ImGui::SetItemAllowOverlap();

			ImGui::SameLine(areaPosX + 5.0f);

			if (layoutSuspended)
				ImGui::ResumeLayout();

			ImGui::BeginHorizontal(GenerateID(), ImGui::GetItemRectSize());
			const ImVec2 iconSize(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight());

			// Search icon
			{
				const float iconYOffset = framePaddingY - 3.0f;
				UI::ShiftCursorY(iconYOffset);
				UI::Image(EditorResources::SearchIcon, iconSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
				UI::ShiftCursorY(-iconYOffset);

				// Hint
				if (!searching)
				{
					UI::ShiftCursorY(-framePaddingY + 1.0f);
					UI::ScopedColour text(ImGuiCol_Text, Colors::Theme::textDarker);
					UI::ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(0.0f, framePaddingY));
					ImGui::TextUnformatted(hint);
					UI::ShiftCursorY(-1.0f);
				}
			}

			ImGui::Spring();

			// Clear icon
			if (searching)
			{
				const float spacingX = 4.0f;
				const float lineHeight = ImGui::GetItemRectSize().y - framePaddingY / 2.0f;

				if (ImGui::InvisibleButton(GenerateID(), ImVec2{ lineHeight, lineHeight }))
				{
					if constexpr (std::is_same<StringType, std::string>::value)
						searchString.clear();
					else
						memset(searchString, 0, BuffSize);

					modified = true;
				}

				if (ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()))
					ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

				UI::DrawButtonImage(EditorResources::ClearIcon, IM_COL32(160, 160, 160, 200),
					IM_COL32(170, 170, 170, 255),
					IM_COL32(160, 160, 160, 150),
					UI::RectExpanded(UI::GetItemRect(), -2.0f, -2.0f));

				ImGui::Spring(-1.0f, spacingX * 2.0f);
			}

			ImGui::EndHorizontal();
			UI::ShiftCursorY(-1.0f);
			UI::PopID();
			return modified;
		}

		static bool AssetSearchPopup(const char* ID, AssetHandle& selected, bool* cleared = nullptr, const char* hint = "Search Assets", ImVec2 size = ImVec2{ 250.0f, 350.0f }, std::initializer_list<AssetType> assetTypes = {});
		static bool AssetSearchPopup(const char* ID, AssetType assetType, AssetHandle& selected, bool* cleared = nullptr, const char* hint = "Search Assets", ImVec2 size = ImVec2{ 250.0f, 350.0f });
		static bool EntitySearchPopup(const char* ID, Ref<Scene> scene, UUID& selected, bool* cleared = nullptr, const char* hint = "Search Entities", ImVec2 size = ImVec2{ 250.0f, 350.0f });
		static bool ScriptSearchPopup(const char* ID, const ScriptEngine& scriptEngine, UUID& selected, bool* cleared = nullptr, const char* hint = "Search Scripts", ImVec2 size = ImVec2{ 250.0f, 350.0f });

		static bool OptionsButton()
		{
			const bool clicked = ImGui::InvisibleButton("##options", ImVec2{ ImGui::GetFrameHeight(), ImGui::GetFrameHeight() });

			const float spaceAvail = std::min(ImGui::GetItemRectSize().x, ImGui::GetItemRectSize().y);
			const float desiredIconSize = 15.0f;
			const float padding = std::max((spaceAvail - desiredIconSize) / 2.0f, 0.0f);

			constexpr auto buttonColour = Colors::Theme::text;
			const uint8_t value = uint8_t(ImColor(buttonColour).Value.x * 255);
			UI::DrawButtonImage(EditorResources::GearIcon, IM_COL32(value, value, value, 200),
											IM_COL32(value, value, value, 255),
											IM_COL32(value, value, value, 150),
											UI::RectExpanded(UI::GetItemRect(), -padding, -padding));
			return clicked;
		}

		// See also ImGuiUtilities.h UI::DragVec3() which uses Dear ImGui's internal DragVec3
		// By contrast EditVec3() custom draws the X,Y, and Z components with red, green and blue label buttons.
		static bool EditVec3(std::string_view label, ImVec2 size, float resetValue, bool& manuallyEdited, glm::vec3& value, VectorAxis renderMultiSelectAxes = VectorAxis::None, float speed = 1.0f, glm::vec3 v_min = glm::zero<glm::vec3>(), glm::vec3 v_max = glm::zero<glm::vec3>(), const char* format = "%.2f", ImGuiSliderFlags flags = 0)
		{
			ImGui::BeginVertical((std::string(label) + "fr").data());
			bool changed = false;
			{
				const float spacingX = 8.0f;
				UI::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2{ spacingX, 0.0f });
				UI::ScopedStyle padding(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 2.0f });
				const float framePadding = 2.0f;
				const float outlineSpacing = 1.0f;
				const float lineHeight = GImGui->Font->FontSize + framePadding * 2.0f;
				const ImVec2 buttonSize = { lineHeight + 2.0f, lineHeight };
				const float inputItemWidth = size.x / 3.0f - buttonSize.x;

				UI::ShiftCursorY(framePadding);

				const ImGuiIO& io = ImGui::GetIO();
				auto boldFont = io.Fonts->Fonts[0];

				auto drawControl = [&](const std::string& label, float& value, const ImVec4& colourN, const ImVec4& colourH, const ImVec4& colourP, bool renderMultiSelect, float speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
				{
					{
						UI::ScopedStyle buttonFrame(ImGuiStyleVar_FramePadding, ImVec2(framePadding, 0.0f));
						UI::ScopedStyle buttonRounding(ImGuiStyleVar_FrameRounding, 1.0f);
						UI::ScopedColourStack buttonColours(ImGuiCol_Button, colourN, ImGuiCol_ButtonHovered, colourH, ImGuiCol_ButtonActive, colourP);

						UI::ScopedFont buttonFont(boldFont);

						//ImGui::AlignTextToFramePadding();
						UI::ShiftCursorY(framePadding / 2.0f);
						if (ImGui::Button(label.c_str(), buttonSize))
						{
							value = resetValue;
							changed = true;
						}
					}

					ImGui::SameLine(0.0f, outlineSpacing);
					ImGui::SetNextItemWidth(inputItemWidth);
					UI::ShiftCursorY(-framePadding / 2.0f);
					ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, renderMultiSelect);
					bool wasTempInputActive = ImGui::TempInputIsActive(ImGui::GetID(("##" + label).c_str()));
					changed |= UI::DragFloat(("##" + label).c_str(), &value, speed, v_min, v_max, format, flags);

					// NOTE(Peter): Ugly hack to make tabbing behave the same as Enter (e.g marking it as manually modified)
					if (changed && Input::IsKeyDown(KeyCode::Tab))
						manuallyEdited = true;

					if (ImGui::TempInputIsActive(ImGui::GetID(("##" + label).c_str())))
						changed = false;

					ImGui::PopItemFlag();

					if (wasTempInputActive)
						manuallyEdited |= ImGui::IsItemDeactivatedAfterEdit();
				};

				drawControl("X", value.x, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f }, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f }, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f }, (renderMultiSelectAxes & VectorAxis::X) == VectorAxis::X, speed, v_min.x, v_max.x, format, flags);

				ImGui::SameLine(0.0f, outlineSpacing);
				drawControl("Y", value.y, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f }, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f }, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f }, (renderMultiSelectAxes & VectorAxis::Y) == VectorAxis::Y, speed, v_min.y, v_max.y, format, flags);

				ImGui::SameLine(0.0f, outlineSpacing);
				drawControl("Z", value.z, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f }, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f }, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f }, (renderMultiSelectAxes & VectorAxis::Z) == VectorAxis::Z, speed, v_min.z, v_max.z, format, flags);

//				ImGui::EndChild();
				ImGui::EndVertical();
			}
			return changed || manuallyEdited;
		}

	}; // Widgets

} // namespace Hazel::UI

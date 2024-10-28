#pragma once

#include "UICore.h"

#include "Engine/Asset/AssetMetadata.h"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <map>

#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Prefab.h"
#include "Engine/Asset/AssetManager.h"
#include "Engine/Editor/AssetEditorPanelInterface.h"
#include "Engine/ImGui/Colors.h"
#include "Engine/ImGui/ImGuiUtilities.h"
#include "Engine/ImGui/ImGuiWidgets.h"
#include "Engine/ImGui/ImGuiFonts.h"
#include "Engine/Utilities/StringUtils.h"
#include "Engine/Script/ScriptCache.h"

#include "choc/text/choc_StringUtilities.h"

namespace Engine::UI {

#define ZONG_MESSAGE_BOX_OK_BUTTON BIT(0)
#define ZONG_MESSAGE_BOX_CANCEL_BUTTON BIT(1)
#define ZONG_MESSAGE_BOX_USER_FUNC BIT(2)
#define ZONG_MESSAGE_BOX_AUTO_SIZE BIT(3)

	struct MessageBoxData
	{
		std::string Title = "";
		std::string Body = "";
		uint32_t Flags = 0;
		uint32_t Width = 0;
		uint32_t Height = 0;
		uint32_t MinWidth = 0;
		uint32_t MinHeight = 0;
		uint32_t MaxWidth = -1;
		uint32_t MaxHeight = -1;
		std::function<void()> UserRenderFunction;
		bool ShouldOpen = true;
		bool IsOpen = false;
	};
	static std::unordered_map<std::string, MessageBoxData> s_MessageBoxes;

	static ImFont* FindFont(const std::string& fontName)
	{
		return Fonts::Get(fontName);
	}

	static void PushFontBold()
	{
		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
	}

	static void PushFontLarge()
	{
		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
	}

	template<uint32_t flags = 0, typename... TArgs>
	static void ShowSimpleMessageBox(const char* title, const char* content, TArgs&&... contentArgs)
	{
		auto& messageBoxData = s_MessageBoxes[title];
		messageBoxData.Title = fmt::format("{0}##MessageBox{1}", title, s_MessageBoxes.size() + 1);
		if constexpr (sizeof...(contentArgs) > 0)
			messageBoxData.Body = fmt::format(content, std::forward<TArgs>(contentArgs)...);
		else
			messageBoxData.Body = content;
		messageBoxData.Flags = flags;
		messageBoxData.Width = 600;
		messageBoxData.Height = 0;
		messageBoxData.ShouldOpen = true;
	}

	static void ShowMessageBox(const char* title, const std::function<void()>& renderFunction, uint32_t width = 600, uint32_t height = 0, uint32_t minWidth = 0, uint32_t minHeight = 0, uint32_t maxWidth = -1, uint32_t maxHeight = -1, uint32_t flags = ZONG_MESSAGE_BOX_AUTO_SIZE)
	{
		auto& messageBoxData = s_MessageBoxes[title];
		messageBoxData.Title = fmt::format("{0}##MessageBox{1}", title, s_MessageBoxes.size() + 1);
		messageBoxData.UserRenderFunction = renderFunction;
		messageBoxData.Flags = ZONG_MESSAGE_BOX_USER_FUNC | flags;
		messageBoxData.Width = width;
		messageBoxData.Height = height;
		messageBoxData.MinWidth = minWidth;
		messageBoxData.MinHeight = minHeight;
		messageBoxData.MaxWidth = maxWidth;
		messageBoxData.MaxHeight = maxHeight;
		messageBoxData.ShouldOpen = true;
	}

	static void RenderMessageBoxes()
	{
		for (auto& [key, messageBoxData] : s_MessageBoxes)
		{
			if (messageBoxData.ShouldOpen && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
			{
				ImGui::OpenPopup(messageBoxData.Title.c_str());
				messageBoxData.ShouldOpen = false;
				messageBoxData.IsOpen = true;
			}

			if (!messageBoxData.IsOpen)
				continue;

			if (!ImGui::IsPopupOpen(messageBoxData.Title.c_str()))
			{
				messageBoxData.IsOpen = false;
				continue;
			}

			if (messageBoxData.Width != 0 || messageBoxData.Height != 0)
			{
				ImVec2 center = ImGui::GetMainViewport()->GetCenter();
				ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
				ImGui::SetNextWindowSize(ImVec2{ (float)messageBoxData.Width, (float)messageBoxData.Height }, ImGuiCond_Appearing);
			}

			ImGuiWindowFlags windowFlags = 0;
			if(messageBoxData.Flags & ZONG_MESSAGE_BOX_AUTO_SIZE)
			{
				windowFlags |= ImGuiWindowFlags_AlwaysAutoResize;
			}
			else
			{
				ImGui::SetNextWindowSizeConstraints(ImVec2{ (float)messageBoxData.MinWidth, (float)messageBoxData.MinHeight }, ImVec2{ (float)messageBoxData.MaxWidth, (float)messageBoxData.MaxHeight });
			}

			if (ImGui::BeginPopupModal(messageBoxData.Title.c_str(), &messageBoxData.IsOpen, windowFlags))
			{
				if (messageBoxData.Flags & ZONG_MESSAGE_BOX_USER_FUNC)
				{
					ZONG_CORE_VERIFY(messageBoxData.UserRenderFunction, "No render function provided for message box!");
					messageBoxData.UserRenderFunction();
				}
				else
				{
					ImGui::TextWrapped(messageBoxData.Body.c_str());

					if (messageBoxData.Flags & ZONG_MESSAGE_BOX_OK_BUTTON)
					{
						if (ImGui::Button("Ok"))
							ImGui::CloseCurrentPopup();

						if (messageBoxData.Flags & ZONG_MESSAGE_BOX_CANCEL_BUTTON)
							ImGui::SameLine();
					}

					if (messageBoxData.Flags & ZONG_MESSAGE_BOX_CANCEL_BUTTON && ImGui::Button("Cancel"))
					{
						ImGui::CloseCurrentPopup();
					}
				}

				ImGui::EndPopup();
			}
		}
	}

	static bool PropertyGridHeader(const std::string& name, bool openByDefault = true)
	{
		ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed
			| ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_AllowItemOverlap
			| ImGuiTreeNodeFlags_FramePadding;

		if (openByDefault)
			treeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

		bool open = false;
		const float framePaddingX = 6.0f;
		const float framePaddingY = 6.0f; // affects height of the header

		UI::ScopedStyle headerRounding(ImGuiStyleVar_FrameRounding, 0.0f);
		UI::ScopedStyle headerPaddingAndHeight(ImGuiStyleVar_FramePadding, ImVec2{ framePaddingX, framePaddingY });

		//UI::PushID();
		ImGui::PushID(name.c_str());
		open = ImGui::TreeNodeEx("##dummy_id", treeNodeFlags, Utils::ToUpper(name).c_str());
		//UI::PopID();
		ImGui::PopID();

		return open;
	}

	static bool TreeNodeWithIcon(const std::string& label, const Ref<Texture2D>& icon, const ImVec2& size, bool openByDefault = true)
	{
		ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed
			| ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_AllowItemOverlap
			| ImGuiTreeNodeFlags_FramePadding;

		if (openByDefault)
			treeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

		bool open = false;
		const float framePaddingX = 6.0f;
		const float framePaddingY = 6.0f; // affects height of the header

		UI::ScopedStyle headerRounding(ImGuiStyleVar_FrameRounding, 0.0f);
		UI::ScopedStyle headerPaddingAndHeight(ImGuiStyleVar_FramePadding, ImVec2{ framePaddingX, framePaddingY });

		ImGui::PushID(label.c_str());
		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
		open = ImGui::TreeNodeEx("##dummy_id", treeNodeFlags, "");

		float lineHeight = ImGui::GetItemRectMax().y - ImGui::GetItemRectMin().y;
		ImGui::SameLine();
		UI::ShiftCursorY(size.y / 2.0f - 1.0f);
		UI::Image(icon, size);
		ImGui::SameLine();
		UI::ShiftCursorY(-(size.y / 2.0f) + 1.0f);
		ImGui::TextUnformatted(Utils::ToUpper(label).c_str());

		ImGui::PopID();

		return open;
	}

	static void Separator()
	{
		ImGui::Separator();
	}

	static bool Property(const char* label, std::string& value, const char* helpText = "")
	{
		bool modified = false;

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		modified = UI::InputText(fmt::format("##{0}", label).c_str(), &value);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertyMultiline(const char* label, std::string& value, const char* helpText = "")
	{
		bool modified = false;

		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		modified = UI::InputTextMultiline(fmt::format("##{0}", label).c_str(), &value);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static void Property(const char* label, const std::string& value, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);
		BeginDisabled();
		UI::InputText(fmt::format("##{0}", label).c_str(), (char*)value.c_str(), value.size(), ImGuiInputTextFlags_ReadOnly);
		EndDisabled();

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();
	}

	static void Property(const char* label, const char* value, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);
		BeginDisabled();
		UI::InputText(fmt::format("##{0}", label).c_str(), (char*)value, 256, ImGuiInputTextFlags_ReadOnly);
		EndDisabled();

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();
	}

	static bool Property(const char* label, char* value, size_t length, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::InputText(fmt::format("##{0}", label).c_str(), value, length);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool Property(const char* label, bool& value, const char* helpText = "")
	{
		bool modified = false;

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		modified = UI::Checkbox(fmt::format("##{0}", label).c_str(), &value);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool Property(const char* label, int8_t& value, int8_t min = 0, int8_t max = 0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::DragInt8(fmt::format("##{0}", label).c_str(), &value, 1.0f, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool Property(const char* label, int16_t& value, int16_t min = 0, int16_t max = 0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::DragInt16(fmt::format("##{0}", label).c_str(), &value, 1.0f, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool Property(const char* label, int32_t& value, int32_t min = 0, int32_t max = 0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::DragInt32(fmt::format("##{0}", label).c_str(), &value, 1.0f, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool Property(const char* label, int64_t& value, int64_t min = 0, int64_t max = 0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::DragInt64(fmt::format("##{0}", label).c_str(), &value, 1.0f, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool Property(const char* label, uint8_t& value, uint8_t minValue = 0, uint8_t maxValue = 0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::DragUInt8(fmt::format("##{0}", label).c_str(), &value, 1.0f, minValue, maxValue);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool Property(const char* label, uint16_t& value, uint16_t minValue = 0, uint16_t maxValue = 0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::DragUInt16(fmt::format("##{0}", label).c_str(), &value, 1.0f, minValue, maxValue);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool Property(const char* label, uint32_t& value, uint32_t minValue = 0, uint32_t maxValue = 0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::DragUInt32(fmt::format("##{0}", label).c_str(), &value, 1.0f, minValue, maxValue);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool Property(const char* label, uint64_t& value, uint64_t minValue = 0, uint64_t maxValue = 0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::DragUInt64(fmt::format("##{0}", label).c_str(), &value, 1.0f, minValue, maxValue);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertyRadio(const char* label, int& chosen, const std::map<int, const std::string_view>& options, const char* helpText = "", const std::map<int, const std::string_view>& optionHelpTexts = {})
	{
		bool modified = false;

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		for (auto [value, option] : options)
		{
			std::string radioLabel = fmt::format("{}##{}", option.data(), label);
			if (ImGui::RadioButton(radioLabel.c_str(), &chosen, value))
				modified = true;

			if (auto optionHelpText = optionHelpTexts.find(value); optionHelpText != optionHelpTexts.end())
			{
				if (std::strlen(optionHelpText->second.data()) != 0)
				{
					ImGui::SameLine();
					HelpMarker(optionHelpText->second.data());
				}
			}
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool Property(const char* label, float& value, float delta = 0.1f, float min = 0.0f, float max = 0.0f, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::DragFloat(fmt::format("##{0}", label).c_str(), &value, delta, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool Property(const char* label, double& value, float delta = 0.1f, double min = 0.0, double max = 0.0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::DragDouble(fmt::format("##{0}", label).c_str(), &value, delta, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool Property(const char* label, glm::vec2& value, float delta = 0.1f, float min = 0.0f, float max = 0.0f, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::DragFloat2(fmt::format("##{0}", label).c_str(), glm::value_ptr(value), delta, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool Property(const char* label, glm::vec3& value, float delta = 0.1f, float min = 0.0f, float max = 0.0f, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::DragFloat3(fmt::format("##{0}", label).c_str(), glm::value_ptr(value), delta, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool Property(const char* label, glm::vec4& value, float delta = 0.1f, float min = 0.0f, float max = 0.0f, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::DragFloat4(fmt::format("##{0}", label).c_str(), glm::value_ptr(value), delta, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool Property(const char* label, glm::bvec3& value, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(" X");
		ImGui::SameLine();
		bool modified = UI::Checkbox(fmt::format("##1{0}", label).c_str(), &value.x);
		ImGui::SameLine();
		ImGui::TextUnformatted(" Y");
		ImGui::SameLine();
		modified |= UI::Checkbox(fmt::format("##2{0}", label).c_str(), &value.y);
		ImGui::SameLine();
		ImGui::TextUnformatted(" Z");
		ImGui::SameLine();
		modified |= UI::Checkbox(fmt::format("##3{0}", label).c_str(), &value.z);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertySlider(const char* label, int& value, int min, int max, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::SliderInt32(GenerateID(), &value, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertySlider(const char* label, float& value, float min, float max, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::SliderFloat(GenerateID(), &value, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertySlider(const char* label, glm::vec2& value, float min, float max, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::SliderFloat2(GenerateID(), glm::value_ptr(value), min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertySlider(const char* label, glm::vec3& value, float min, float max, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::SliderFloat3(GenerateID(), glm::value_ptr(value), min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertySlider(const char* label, glm::vec4& value, float min, float max, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::SliderFloat4(GenerateID(), glm::value_ptr(value), min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertyInput(const char* label, int8_t& value, int8_t step = 1, int8_t stepFast = 1, ImGuiInputTextFlags flags = 0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::InputInt8(GenerateID(), &value, step, stepFast, flags);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertyInput(const char* label, int16_t& value, int16_t step = 1, int16_t stepFast = 1, ImGuiInputTextFlags flags = 0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::InputInt16(GenerateID(), &value, step, stepFast, flags);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertyInput(const char* label, int32_t& value, int32_t step = 1, int32_t stepFast = 1, ImGuiInputTextFlags flags = 0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::InputInt32(GenerateID(), &value, step, stepFast, flags);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertyInput(const char* label, int64_t& value, int64_t step = 1, int64_t stepFast = 1, ImGuiInputTextFlags flags = 0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::InputInt64(GenerateID(), &value, step, stepFast, flags);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertyInput(const char* label, uint8_t& value, uint8_t step = 1, uint8_t stepFast = 1, ImGuiInputTextFlags flags = 0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::InputUInt8(GenerateID(), &value, step, stepFast, flags);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertyInput(const char* label, uint16_t& value, uint16_t step = 1, uint16_t stepFast = 1, ImGuiInputTextFlags flags = 0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::InputUInt16(GenerateID(), &value, step, stepFast, flags);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertyInput(const char* label, uint32_t& value, uint32_t step = 1, uint32_t stepFast = 1, ImGuiInputTextFlags flags = 0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::InputUInt32(GenerateID(), &value, step, stepFast, flags);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertyInput(const char* label, uint64_t& value, uint64_t step = 0, uint64_t stepFast = 0, ImGuiInputTextFlags flags = 0, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::InputUInt64(GenerateID(), &value, step, stepFast, flags);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertyColor(const char* label, glm::vec3& value, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::ColorEdit3(GenerateID(), glm::value_ptr(value));

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertyColor(const char* label, glm::vec4& value, const char* helpText = "")
	{
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = UI::ColorEdit4(GenerateID(), glm::value_ptr(value));

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	template<typename TEnum, typename TUnderlying = int32_t>
	static bool PropertyDropdown(const char* label, const char** options, int32_t optionCount, TEnum& selected, const char* helpText = "")
	{
		TUnderlying selectedIndex = (TUnderlying)selected;

		const char* current = options[selectedIndex];
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = false;

		const std::string id = "##" + std::string(label);
		if (UI::BeginCombo(id.c_str(), current))
		{
			for (int i = 0; i < optionCount; i++)
			{
				const bool is_selected = (current == options[i]);
				if (ImGui::Selectable(options[i], is_selected))
				{
					current = options[i];
					selected = (TEnum)i;
					modified = true;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			UI::EndCombo();
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertyDropdown(const char* label, const char** options, int32_t optionCount, int32_t* selected, const char* helpText = "")
	{
		const char* current = options[*selected];
		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = false;

		const std::string id = "##" + std::string(label);
		if (UI::BeginCombo(id.c_str(), current))
		{
			for (int i = 0; i < optionCount; i++)
			{
				const bool is_selected = (current == options[i]);
				if (ImGui::Selectable(options[i], is_selected))
				{
					current = options[i];
					*selected = i;
					modified = true;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			UI::EndCombo();
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertyDropdownNoLabel(const char* strID, const char** options, int32_t optionCount, int32_t* selected)
	{
		const char* current = options[*selected];
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = false;

		const std::string id = "##" + std::string(strID);
		if (UI::BeginCombo(id.c_str(), current))
		{
			for (int i = 0; i < optionCount; i++)
			{
				const bool is_selected = (current == options[i]);
				if (ImGui::Selectable(options[i], is_selected))
				{
					current = options[i];
					*selected = i;
					modified = true;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			UI::EndCombo();
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	static bool PropertyDropdownNoLabel(const char* strID, const std::vector<std::string>& options, int32_t* selected, bool advanceColumn = true, bool fullWidth = true)
	{
		const char* current = options[*selected].c_str();
		ShiftCursorY(4.0f);

		if (fullWidth)
			ImGui::PushItemWidth(-1);

		bool modified = false;

		if (IsItemDisabled())
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);

		if ((GImGui->CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0)
			current = "---";

		const std::string id = "##" + std::string(strID);
		if (UI::BeginCombo(id.c_str(), current))
		{
			for (int i = 0; i < options.size(); i++)
			{
				const bool is_selected = (current == options[i]);
				if (ImGui::Selectable(options[i].c_str(), is_selected))
				{
					current = options[i].c_str();
					*selected = i;
					modified = true;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			UI::EndCombo();
		}

		if (IsItemDisabled())
			ImGui::PopStyleVar();

		if (fullWidth)
			ImGui::PopItemWidth();

		if (advanceColumn)
		{
			ImGui::NextColumn();
			Draw::Underline();
		}

		return modified;
	}

	static bool PropertyDropdown(const char* label, const std::vector<std::string>& options, int32_t optionCount, int32_t* selected, const char* helpText = "")
	{
		const char* current = options[*selected].c_str();

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		bool modified = false;

		const std::string id = "##" + std::string(label);
		if (UI::BeginCombo(id.c_str(), current))
		{
			for (int i = 0; i < optionCount; i++)
			{
				const bool is_selected = (current == options[i]);
				if (ImGui::Selectable(options[i].c_str(), is_selected))
				{
					current = options[i].c_str();
					*selected = i;
					modified = true;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			UI::EndCombo();
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return modified;
	}

	enum class PropertyAssetReferenceError
	{
		None = 0, InvalidMetadata
	};

	// TODO(Yan): move this somewhere better when restructuring API
	static AssetHandle s_PropertyAssetReferenceAssetHandle;

	struct PropertyAssetReferenceSettings
	{
		bool AdvanceToNextColumn = true;
		bool NoItemSpacing = false; // After label
		float WidthOffset = 0.0f;
		bool AllowMemoryOnlyAssets = false;
		ImVec4 ButtonLabelColor = ImGui::ColorConvertU32ToFloat4(Colors::Theme::text);
		ImVec4 ButtonLabelColorError = ImGui::ColorConvertU32ToFloat4(Colors::Theme::textError);
		bool ShowFullFilePath = false;
	};

	template<typename T>
	static bool PropertyAssetReference(const char* label, AssetHandle& outHandle, const char* helpText = "", PropertyAssetReferenceError* outError = nullptr, const PropertyAssetReferenceSettings& settings = {})
	{
		bool modified = false;
		if (outError)
			*outError = PropertyAssetReferenceError::None;

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		ImVec2 originalButtonTextAlign = ImGui::GetStyle().ButtonTextAlign;
		{
			ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
			float width = ImGui::GetContentRegionAvail().x - settings.WidthOffset;
			float itemHeight = 28.0f;

			std::string buttonText = "Null";
			bool valid = true;
			if (AssetManager::IsAssetHandleValid(outHandle))
			{
				auto object = AssetManager::GetAsset<T>(outHandle);
				valid = object && !object->IsFlagSet(AssetFlag::Invalid) && !object->IsFlagSet(AssetFlag::Missing);
				if (object && !object->IsFlagSet(AssetFlag::Missing))
				{
					if (settings.ShowFullFilePath)
						buttonText = Project::GetEditorAssetManager()->GetMetadata(outHandle).FilePath.string();
					else
						buttonText = Project::GetEditorAssetManager()->GetMetadata(outHandle).FilePath.stem().string();
				}
				else
				{
					buttonText = "Missing";
				}
			}

			if ((GImGui->CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0)
				buttonText = "---";

			// PropertyAssetReference could be called multiple times in same "context"
			// and so we need a unique id for the asset search popup each time.
			// notes
			// - don't use GenerateID(), that's inviting id clashes, which would be super confusing.
			// - don't store return from GenerateLabelId in a const char* here. Because its pointing to an internal
			//   buffer which may get overwritten by the time you want to use it later on.
			std::string assetSearchPopupID = GenerateLabelID("ARSP");
			{
				UI::ScopedColour buttonLabelColor(ImGuiCol_Text, valid ? settings.ButtonLabelColor : settings.ButtonLabelColorError);
				ImGui::Button(GenerateLabelID(buttonText), { width, itemHeight });

				const bool isHovered = ImGui::IsItemHovered();

				if (isHovered)
				{
					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						// NOTE(Peter): Ugly workaround since AssetEditorPanel includes ImGui.h (meaning ImGui.h can't include AssetEditorPanel).
						//				Will rework those includes at a later date...
						AssetEditorPanelInterface::OpenEditor(AssetManager::GetAsset<Asset>(outHandle));
					}
					else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					{
						ImGui::OpenPopup(assetSearchPopupID.c_str());
					}
				}
			}

			ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

			bool clear = false;
			if (Widgets::AssetSearchPopup(assetSearchPopupID.c_str(), T::GetStaticType(), outHandle, settings.AllowMemoryOnlyAssets, &clear))
			{
				if (clear)
					outHandle = 0;
				modified = true;
				s_PropertyAssetReferenceAssetHandle = outHandle;
			}
		}

		if (!IsItemDisabled())
		{
			if (ImGui::BeginDragDropTarget())
			{
				auto data = ImGui::AcceptDragDropPayload("asset_payload");

				if (data)
				{
					AssetHandle assetHandle = *(AssetHandle*)data->Data;
					s_PropertyAssetReferenceAssetHandle = assetHandle;
					Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
					if (asset && asset->GetAssetType() == T::GetStaticType())
					{
						outHandle = assetHandle;
						modified = true;
					}
				}

				ImGui::EndDragDropTarget();
			}
		}

		ImGui::PopItemWidth();
		if (settings.AdvanceToNextColumn)
		{
			ImGui::NextColumn();
			Draw::Underline();
		}

		return modified;
	}

	template<AssetType... TAssetTypes>
	static bool PropertyMultiAssetReference(const char* label, AssetHandle& outHandle, const char* helpText = "", PropertyAssetReferenceError* outError = nullptr, const PropertyAssetReferenceSettings& settings = PropertyAssetReferenceSettings())
	{
		bool modified = false;
		if (outError)
			*outError = PropertyAssetReferenceError::None;

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		std::initializer_list<AssetType> assetTypes = { TAssetTypes... };

		ImVec2 originalButtonTextAlign = ImGui::GetStyle().ButtonTextAlign;
		{
			ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
			float width = ImGui::GetContentRegionAvail().x - settings.WidthOffset;
			float itemHeight = 28.0f;

			std::string buttonText = "Null";
			if (AssetManager::IsAssetHandleValid(outHandle))
			{
				auto object = AssetManager::GetAsset<Asset>(outHandle);
				if (object && !object->IsFlagSet(AssetFlag::Missing))
				{
					if (settings.ShowFullFilePath)
						buttonText = Project::GetEditorAssetManager()->GetMetadata(outHandle).FilePath.string();
					else
						buttonText = Project::GetEditorAssetManager()->GetMetadata(outHandle).FilePath.stem().string();
				}
				else
				{
					buttonText = "Missing";
				}
			}

			if ((GImGui->CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0)
				buttonText = "---";

			// PropertyAssetReference could be called multiple times in same "context"
			// and so we need a unique id for the asset search popup each time.
			// notes
			// - don't use GenerateID(), that's inviting id clashes, which would be super confusing.
			// - don't store return from GenerateLabelId in a const char* here. Because its pointing to an internal
			//   buffer which may get overwritten by the time you want to use it later on.
			std::string assetSearchPopupID = GenerateLabelID("ARSP");

			ImGui::PushStyleColor(ImGuiCol_Text, settings.ButtonLabelColor);
			ImGui::Button(GenerateLabelID(buttonText), { width, itemHeight });

			const bool isHovered = ImGui::IsItemHovered();

			if (isHovered)
			{
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					// NOTE(Peter): Ugly workaround since AssetEditorPanel includes ImGui.h (meaning ImGui.h can't include AssetEditorPanel).
					//				Will rework those includes at a later date...
					AssetEditorPanelInterface::OpenEditor(AssetManager::GetAsset<Asset>(outHandle));
				}
				else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					ImGui::OpenPopup(assetSearchPopupID.c_str());
				}
			}
			ImGui::PopStyleColor();
			ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

			bool clear = false;
			if (Widgets::AssetSearchPopup(assetSearchPopupID.c_str(), outHandle, &clear, "Search Assets", ImVec2{ 250.0f, 350.0f }, assetTypes))
			{
				if (clear)
					outHandle = 0;
				modified = true;
				s_PropertyAssetReferenceAssetHandle = outHandle;
			}
		}

		if (ImGui::BeginDragDropTarget())
		{
			auto data = ImGui::AcceptDragDropPayload("asset_payload");

			if (data)
			{
				AssetHandle assetHandle = *(AssetHandle*)data->Data;
				s_PropertyAssetReferenceAssetHandle = assetHandle;
				const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(assetHandle);

				for (AssetType type : assetTypes)
				{
					if (metadata.Type == type)
					{
						outHandle = assetHandle;
						modified = true;
						break;
					}
				}
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::PopItemWidth();
		if (settings.AdvanceToNextColumn)
		{
			ImGui::NextColumn();
			Draw::Underline();
		}

		return modified;
	}


	template<typename TAssetType, typename TConversionType, typename Fn>
	static bool PropertyAssetReferenceWithConversion(const char* label, AssetHandle& outHandle, Fn&& conversionFunc, const char* helpText = "", PropertyAssetReferenceError* outError = nullptr, const PropertyAssetReferenceSettings& settings = {})
	{
		bool succeeded = false;
		if (outError)
			*outError = PropertyAssetReferenceError::None;

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		ImVec2 originalButtonTextAlign = ImGui::GetStyle().ButtonTextAlign;
		ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
		float width = ImGui::GetContentRegionAvail().x - settings.WidthOffset;
		UI::PushID();

		float itemHeight = 28.0f;

		std::string buttonText = "Null";
		bool valid = true;

		if (AssetManager::IsAssetHandleValid(outHandle))
		{
			auto object = AssetManager::GetAsset<TAssetType>(outHandle);
			valid = object && !object->IsFlagSet(AssetFlag::Invalid) && !object->IsFlagSet(AssetFlag::Missing);
			if (object && !object->IsFlagSet(AssetFlag::Missing))
			{
				buttonText = Project::GetEditorAssetManager()->GetMetadata(outHandle).FilePath.stem().string();
			}
			else
			{
				buttonText = "Missing";
			}
		}

		if ((GImGui->CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0)
			buttonText = "---";

		// PropertyAssetReferenceWithConversion could be called multiple times in same "context"
		// and so we need a unique id for the asset search popup each time.
		// notes
		// - don't use GenerateID(), that's inviting id clashes, which would be super confusing.
		// - don't store return from GenerateLabelId in a const char* here. Because its pointing to an internal
		//   buffer which may get overwritten by the time you want to use it later on.
		std::string assetSearchPopupID = GenerateLabelID("ARWCSP");
		{
			UI::ScopedColour buttonLabelColor(ImGuiCol_Text, valid ? settings.ButtonLabelColor : settings.ButtonLabelColorError);
			ImGui::Button(GenerateLabelID(buttonText), { width, itemHeight });

			const bool isHovered = ImGui::IsItemHovered();

			if (isHovered)
			{
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					// NOTE(Peter): Ugly workaround since AssetEditorPanel includes ImGui.h (meaning ImGui.h can't include AssetEditorPanel).
					//				Will rework those includes at a later date...
					AssetEditorPanelInterface::OpenEditor(AssetManager::GetAsset<Asset>(outHandle));
				}
				else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					ImGui::OpenPopup(assetSearchPopupID.c_str());
				}
			}
		}

		ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

		AssetHandle assetHandle = outHandle;

		bool clear = false;
		if (Widgets::AssetSearchPopup(assetSearchPopupID.c_str(), assetHandle, &clear, "Search Assets", ImVec2(250.0f, 350.0f), { TConversionType::GetStaticType(), TAssetType::GetStaticType() }))
		{
			if (clear)
			{
				outHandle = 0;
				assetHandle = 0;
				succeeded = true;
			}
		}
		UI::PopID();

		if (!IsItemDisabled())
		{
			if (ImGui::BeginDragDropTarget())
			{
				auto data = ImGui::AcceptDragDropPayload("asset_payload");

				if (data)
					assetHandle = *(AssetHandle*)data->Data;

				ImGui::EndDragDropTarget();
			}

			if (assetHandle != outHandle && assetHandle != 0)
			{
				s_PropertyAssetReferenceAssetHandle = assetHandle;

				Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
				if (asset)
				{
					// No conversion necessary 
					if (asset->GetAssetType() == TAssetType::GetStaticType())
					{
						outHandle = assetHandle;
						succeeded = true;
					}
					// Convert
					else if (asset->GetAssetType() == TConversionType::GetStaticType())
					{
						conversionFunc(asset.As<TConversionType>());
						succeeded = false; // Must be handled by conversion function
					}
				}
				else
				{
					if (outError)
						*outError = PropertyAssetReferenceError::InvalidMetadata;
				}
			}
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		Draw::Underline();

		return succeeded;
	}

	static bool PropertyEntityReference(const char* label, UUID& entityID, Ref<Scene> currentScene, const char* helpText = "")
	{
		bool receivedValidEntity = false;

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		ImVec2 originalButtonTextAlign = ImGui::GetStyle().ButtonTextAlign;
		{
			ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
			float width = ImGui::GetContentRegionAvail().x;
			float itemHeight = 28.0f;

			std::string buttonText = "Null";

			Entity entity = currentScene->TryGetEntityWithUUID(entityID);
			if (entity)
				buttonText = entity.GetComponent<TagComponent>().Tag;

			if ((GImGui->CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0)
				buttonText = "---";

			// PropertyEntityReference could be called multiple times in same "context"
			// and so we need a unique id for the asset search popup each time.
			// notes
			// - don't use GenerateID(), that's inviting id clashes, which would be super confusing.
			// - don't store return from GenerateLabelId in a const char* here. Because its pointing to an internal
			//   buffer which may get overwritten by the time you want to use it later on.
			std::string assetSearchPopupID = GenerateLabelID("ARSP");
			{
				UI::ScopedColour buttonLabelColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(Colors::Theme::text));
				ImGui::Button(GenerateLabelID(buttonText), { width, itemHeight });

				const bool isHovered = ImGui::IsItemHovered();
				if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					ImGui::OpenPopup(assetSearchPopupID.c_str());
			}

			ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

			bool clear = false;
			if (Widgets::EntitySearchPopup(assetSearchPopupID.c_str(), currentScene, entityID, &clear))
			{
				if (clear)
					entityID = 0;
				receivedValidEntity = true;
			}
		}

		if (!IsItemDisabled())
		{
			if (ImGui::BeginDragDropTarget())
			{
				auto data = ImGui::AcceptDragDropPayload("scene_entity_hierarchy");
				if (data)
				{
					entityID = *(UUID*)data->Data;
					receivedValidEntity = true;
				}

				ImGui::EndDragDropTarget();
			}
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return receivedValidEntity;
	}

	/// <summary>
	/// Same as PropertyEntityReference, except you can pass in components that the entity is required to have.
	/// </summary>
	/// <typeparam name="...TComponents"></typeparam>
	/// <param name="label"></param>
	/// <param name="entity"></param>
	/// <param name="requiresAllComponents">If true, the entity <b>MUST</b> have all of the components. If false the entity must have only one</param>
	/// <returns></returns>
	template<typename... TComponents>
	static bool PropertyEntityReferenceWithComponents(const char* label, UUID& entityID, Ref<Scene> context, const char* helpText = "", bool requiresAllComponents = true)
	{
		bool receivedValidEntity = false;

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		ImVec2 originalButtonTextAlign = ImGui::GetStyle().ButtonTextAlign;
		{
			ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
			float width = ImGui::GetContentRegionAvail().x;
			float itemHeight = 28.0f;

			std::string buttonText = "Null";

			Entity entity = context->TryGetEntityWithUUID(entityID);
			if (entity)
				buttonText = entity.GetComponent<TagComponent>().Tag;

			if ((GImGui->CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0)
				buttonText = "---";

			ImGui::Button(GenerateLabelID(buttonText), { width, itemHeight });
		}
		ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

		if (ImGui::BeginDragDropTarget())
		{
			auto data = ImGui::AcceptDragDropPayload("scene_entity_hierarchy");
			if (data)
			{
				UUID tempID = *(UUID*)data->Data;
				Entity temp = context->TryGetEntityWithUUID(tempID);

				if (requiresAllComponents)
				{
					if (temp.HasComponent<TComponents...>())
					{
						entityID = tempID;
						receivedValidEntity = true;
					}
				}
				else
				{
					if (temp.HasAny<TComponents...>())
					{
						entityID = tempID;
						receivedValidEntity = true;
					}
				}
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return receivedValidEntity;
	}

	template<typename T, typename Fn>
	static bool PropertyAssetReferenceTarget(const char* label, const char* assetName, AssetHandle& outHandle, Fn&& targetFunc, const char* helpText = "", const PropertyAssetReferenceSettings& settings = {})
	{
		bool modified = false;

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);
		if (settings.NoItemSpacing)
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f });

		ImVec2 originalButtonTextAlign = ImGui::GetStyle().ButtonTextAlign;
		ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
		float width = ImGui::GetContentRegionAvail().x - settings.WidthOffset;
		UI::PushID();

		float itemHeight = 28.0f;

		std::string buttonText = "Null";
		bool valid = true;

		if (AssetManager::IsAssetHandleValid(outHandle))
		{
			auto source = AssetManager::GetAsset<T>(outHandle);
			valid = source && !source->IsFlagSet(AssetFlag::Invalid) && !source->IsFlagSet(AssetFlag::Missing);
			if (source && !source->IsFlagSet(AssetFlag::Missing))
			{
				if (assetName)
				{
					buttonText = assetName;
				}
				else
				{
					buttonText = Project::GetEditorAssetManager()->GetMetadata(outHandle).FilePath.stem().string();
					assetName = buttonText.c_str();
				}
			}
			else
			{
				buttonText = "Missing";
			}
		}

		if ((GImGui->CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0)
			buttonText = "---";

		// PropertyAssetReferenceTarget could be called multiple times in same "context"
		// and so we need a unique id for the asset search popup each time.
		// notes
		// - don't use GenerateID(), that's inviting id clashes, which would be super confusing.
		// - don't store return from GenerateLabelId in a const char* here. Because its pointing to an internal
		//   buffer which may get overwritten by the time you want to use it later on.
		std::string assetSearchPopupID = GenerateLabelID("ARTSP");
		{
			UI::ScopedColour buttonLabelColor(ImGuiCol_Text, valid ? settings.ButtonLabelColor : settings.ButtonLabelColorError);
			ImGui::Button(GenerateLabelID(buttonText), { width, itemHeight });

			const bool isHovered = ImGui::IsItemHovered();

			if (isHovered)
			{
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					// NOTE(Peter): Ugly workaround since AssetEditorPanel includes ImGui.h (meaning ImGui.h can't include AssetEditorPanel).
					//				Will rework those includes at a later date...
					AssetEditorPanelInterface::OpenEditor(AssetManager::GetAsset<Asset>(outHandle));
				}
				else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					ImGui::OpenPopup(assetSearchPopupID.c_str());
				}
			}
		}

		ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

		bool clear = false;
		if (Widgets::AssetSearchPopup(assetSearchPopupID.c_str(), T::GetStaticType(), outHandle, &clear))
		{
			if (clear)
				outHandle = 0;

			targetFunc(AssetManager::GetAsset<T>(outHandle));
			modified = true;
		}

		UI::PopID();

		if (!IsItemDisabled())
		{
			if (ImGui::BeginDragDropTarget())
			{
				auto data = ImGui::AcceptDragDropPayload("asset_payload");

				if (data)
				{
					AssetHandle assetHandle = *(AssetHandle*)data->Data;
					s_PropertyAssetReferenceAssetHandle = assetHandle;
					Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
					if (asset && asset->GetAssetType() == T::GetStaticType())
					{
						targetFunc(asset.As<T>());
						modified = true;
					}
				}

				ImGui::EndDragDropTarget();
			}
		}

		ImGui::PopItemWidth();
		if (settings.AdvanceToNextColumn)
		{
			ImGui::NextColumn();
			Draw::Underline();
		}
		if (settings.NoItemSpacing)
			ImGui::PopStyleVar();
		return modified;
	}

	// To be used in conjunction with AssetSearchPopup. Call ImGui::OpenPopup for AssetSearchPopup if this is clicked.
	// Alternatively the click can be used to clear the reference.
	template<typename TAssetType>
	static bool AssetReferenceDropTargetButton(const char* label, Ref<TAssetType>& object, AssetType supportedType, bool& assetDropped, bool dropTargetEnabled = true)
	{
		bool clicked = false;

		UI::ScopedStyle border(ImGuiStyleVar_FrameBorderSize, 0.0f);
		UI::ScopedColourStack colors(ImGuiCol_Button, Colors::Theme::propertyField,
									 ImGuiCol_ButtonHovered, Colors::Theme::propertyField,
									 ImGuiCol_ButtonActive, Colors::Theme::propertyField);

		UI::PushID();

		// For some reason ImGui handles button's width differently
		// So need to manually set it if the user has pushed item width
		auto* window = ImGui::GetCurrentWindow();
		const bool itemWidthChanged = !window->DC.ItemWidthStack.empty();
		const float buttonWidth = itemWidthChanged ? window->DC.ItemWidth : 0.0f;

		if (object)
		{
			if (!object->IsFlagSet(AssetFlag::Missing))
			{
				UI::ScopedColour text(ImGuiCol_Text, object->IsFlagSet(AssetFlag::Invalid) ? Colors::Theme::textError : Colors::Theme::text);
				auto assetFileName = Project::GetEditorAssetManager()->GetMetadata(object->Handle).FilePath.stem().string();

				if ((GImGui->CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0)
					assetFileName = "---";

				if (ImGui::Button((char*)assetFileName.c_str(), { buttonWidth, 0.0f }))
					clicked = true;
			}
			else
			{
				if ((GImGui->CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0)
				{
					UI::ScopedColour text(ImGuiCol_Text, ImVec4(0.9f, 0.4f, 0.3f, 1.0f));
					if (ImGui::Button("---", { buttonWidth, 0.0f }))
						clicked = true;
				}
				else
				{
					UI::ScopedColour text(ImGuiCol_Text, Colors::Theme::textError);
					if (ImGui::Button("Missing", { buttonWidth, 0.0f }))
						clicked = true;
				}
			}
		}
		else
		{
			if ((GImGui->CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0)
			{
				UI::ScopedColour text(ImGuiCol_Text, Colors::Theme::muted);
				if (ImGui::Button("---", { buttonWidth, 0.0f }))
					clicked = true;
			}
			else
			{
				UI::ScopedColour text(ImGuiCol_Text, Colors::Theme::muted);
				if (ImGui::Button("Select Asset", { buttonWidth, 0.0f }))
					clicked = true;
			}
		}

		UI::PopID();

		if (dropTargetEnabled)
		{
			if (ImGui::BeginDragDropTarget())
			{
				auto data = ImGui::AcceptDragDropPayload("asset_payload");

				if (data)
				{
					AssetHandle assetHandle = *(AssetHandle*)data->Data;
					Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
					if (asset && asset->GetAssetType() == supportedType)
					{
						object = asset.As<TAssetType>();
						assetDropped = true;
					}
				}

				ImGui::EndDragDropTarget();
			}
		}

		DrawItemActivityOutline();
		return clicked;
	}

	static bool DrawComboPreview(const char* preview, float width = 100.0f)
	{
		bool pressed = false;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		ImGui::BeginHorizontal("horizontal_node_layout", ImVec2(width, 0.0f));
		ImGui::PushItemWidth(90.0f);
		ImGui::InputText("##selected_asset", (char*)preview, 512, ImGuiInputTextFlags_ReadOnly);
		pressed = ImGui::IsItemClicked();
		ImGui::PopItemWidth();

		ImGui::PushItemWidth(10.0f);
		pressed = ImGui::ArrowButton("combo_preview_button", ImGuiDir_Down) || pressed;
		ImGui::PopItemWidth();

		ImGui::EndHorizontal();
		ImGui::PopStyleVar();

		return pressed;
	}

	template<typename TAssetType>
	static bool PropertyAssetDropdown(const char* label, Ref<TAssetType>& object, const ImVec2& size, AssetHandle* selected)
	{
		bool modified = false;
		std::string preview;
		float itemHeight = size.y / 10.0f;

		if (AssetManager::IsAssetHandleValid(*selected))
			object = AssetManager::GetAsset<TAssetType>(*selected);

		if (object)
		{
			if (!object->IsFlagSet(AssetFlag::Missing))
				preview = Project::GetEditorAssetManager()->GetMetadata(object->Handle).FilePath.stem().string();
			else
				preview = "Missing";
		}
		else
		{
			preview = "Null";
		}

		auto& assets = AssetManager::GetLoadedAssets();
		AssetHandle current = *selected;

		ImGui::SetNextWindowSize(size);
		if (UI::BeginPopup(label, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
		{
			ImGui::SetKeyboardFocusHere(0);

			for (auto& [handle, asset] : assets)
			{
				if (asset->GetAssetType() != TAssetType::GetStaticType())
					continue;

				auto& metadata = Project::GetEditorAssetManager()->GetMetadata(handle);

				bool is_selected = (current == handle);
				if (ImGui::Selectable(metadata.FilePath.string().c_str(), is_selected))
				{
					current = handle;
					*selected = handle;
					modified = true;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}

			UI::EndPopup();
		}

		return modified;
	}

	static int s_CheckboxCount = 0;

	static void BeginCheckboxGroup(const char* label)
	{
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
	}

	static bool PropertyCheckboxGroup(const char* label, bool& value)
	{
		bool modified = false;

		if (++s_CheckboxCount > 1)
			ImGui::SameLine();

		ImGui::Text(label);
		ImGui::SameLine();
		return UI::Checkbox(GenerateID(), &value);
	}

	static bool Button(const char* label, const ImVec2& size = ImVec2(0, 0))
	{
		bool result = ImGui::Button(label, size);
		ImGui::NextColumn();
		return result;
	}

	static void EndCheckboxGroup()
	{
		ImGui::PopItemWidth();
		ImGui::NextColumn();
		s_CheckboxCount = 0;
	}

	enum class FieldDisableCondition
	{
		Never, NotWritable, Always
	};

	static bool DrawFieldValue(Ref<Scene> sceneContext, const std::string& fieldName, Ref<FieldStorage> storage)
	{
		if (!storage)
			return false;

		const FieldInfo* field = storage->GetFieldInfo();

		float min = 0.0f;
		float max = 0.0f;
		float delta = 0.1f;

		/*if (field->HasAttribute("Engine.ClampValueAttribute"))
		{
			MonoObject* attr = field->GetAttribute("Engine.ClampValueAttribute");
			ZONG_TRY_GET_FIELD_VALUE(min, "Engine.ClampValueAttribute", "Min", attr);
			ZONG_TRY_GET_FIELD_VALUE(max, "Engine.ClampValueAttribute", "Max", attr);
		}*/

		std::string id = fmt::format("{0}-{1}", fieldName, field->ID);
		ImGui::PushID(id.c_str());

		bool result = false;

		switch (field->Type)
		{
			case FieldType::Bool:
			{
				bool value = storage->GetValue<bool>();
				if (Property(fieldName.c_str(), value))
				{
					storage->SetValue(value);
					result = true;
				}
				break;
			}
			case FieldType::Int8:
			{
				int8_t value = storage->GetValue<int8_t>();
				if (Property(fieldName.c_str(), value, (int8_t)min, (int8_t)max))
				{
					storage->SetValue(value);
					result = true;
				}
				break;
			}
			case FieldType::Int16:
			{
				int16_t value = storage->GetValue<int16_t>();
				if (Property(fieldName.c_str(), value, (int16_t)min, (int16_t)max))
				{
					storage->SetValue(value);
					result = true;
				}
				break;
			}
			case FieldType::Int32:
			{
				int32_t value = storage->GetValue<int32_t>();
				if (Property(fieldName.c_str(), value, (int32_t)min, (int32_t)max))
				{
					storage->SetValue(value);
					result = true;
				}
				break;
			}
			case FieldType::Int64:
			{
				int64_t value = storage->GetValue<int64_t>();
				if (Property(fieldName.c_str(), value, (int64_t)min, (int64_t)max))
				{
					storage->SetValue(value);
					result = true;
				}
				break;
			}
			case FieldType::UInt8:
			{
				uint8_t value = storage->GetValue<uint8_t>();
				if (Property(fieldName.c_str(), value, (uint8_t)min, (uint8_t)max))
				{
					storage->SetValue(value);
					result = true;
				}
				break;
			}
			case FieldType::UInt16:
			{
				uint16_t value = storage->GetValue<uint16_t>();
				if (Property(fieldName.c_str(), value, (uint16_t)min, (uint16_t)max))
				{
					storage->SetValue(value);
					result = true;
				}
				break;
			}
			case FieldType::UInt32:
			{
				uint32_t value = storage->GetValue<uint32_t>();
				if (Property(fieldName.c_str(), value, (uint32_t)min, (uint32_t)max))
				{
					storage->SetValue(value);
					result = true;
				}
				break;
			}
			case FieldType::UInt64:
			{
				uint64_t value = storage->GetValue<uint64_t>();
				if (Property(fieldName.c_str(), value, (uint64_t)min, (uint64_t)max))
				{
					storage->SetValue(value);
					result = true;
				}
				break;
			}
			case FieldType::Float:
			{
				float value = storage->GetValue<float>();
				if (Property(fieldName.c_str(), value, delta, min, max))
				{
					storage->SetValue(value);
					result = true;
				}
				break;
			}
			case FieldType::Double:
			{
				double value = storage->GetValue<double>();
				if (Property(fieldName.c_str(), value, delta, (double)min, (double)max))
				{
					storage->SetValue(value);
					result = true;
				}
				break;
			}
			case FieldType::String:
			{
				std::string value = storage->GetValue<std::string>();
				if (Property(fieldName.c_str(), value))
				{
					storage->SetValue<std::string>(value);
					result = true;
				}
				break;
			}
			case FieldType::Vector2:
			{
				glm::vec2 value = storage->GetValue<glm::vec2>();
				if (Property(fieldName.c_str(), value, min, max))
				{
					storage->SetValue(value);
					result = true;
				}
				break;
			}
			case FieldType::Vector3:
			{
				glm::vec3 value = storage->GetValue<glm::vec3>();
				if (Property(fieldName.c_str(), value, min, max))
				{
					storage->SetValue(value);
					result = true;
				}
				break;
			}
			case FieldType::Vector4:
			{
				glm::vec4 value = storage->GetValue<glm::vec4>();
				if (Property(fieldName.c_str(), value, min, max))
				{
					storage->SetValue(value);
					result = true;
				}
				break;
			}
			case FieldType::Prefab:
			{
				AssetHandle handle = storage->GetValue<AssetHandle>();
				if (PropertyAssetReference<Prefab>(fieldName.c_str(), handle))
				{
					storage->SetValue(handle);
					result = true;
				}
				break;
			}
			case FieldType::Entity:
			{
				UUID uuid = storage->GetValue<UUID>();
				if (PropertyEntityReference(fieldName.c_str(), uuid, sceneContext))
				{
					storage->SetValue(uuid);
					result = true;
				}
				break;
			}
			case FieldType::Mesh:
			{
				AssetHandle handle = storage->GetValue<AssetHandle>();
				if (PropertyAssetReference<Mesh>(fieldName.c_str(), handle))
				{
					storage->SetValue(handle);
					result = true;
				}
				break;
			}
			case FieldType::StaticMesh:
			{
				AssetHandle handle = storage->GetValue<AssetHandle>();
				if (PropertyAssetReference<StaticMesh>(fieldName.c_str(), handle))
				{
					storage->SetValue(handle);
					result = true;
				}
				break;
			}
			case FieldType::Material:
			{
				AssetHandle handle = storage->GetValue<AssetHandle>();
				if (PropertyAssetReference<MaterialAsset>(fieldName.c_str(), handle))
				{
					storage->SetValue(handle);
					result = true;
				}
				break;
			}
			/*case UnmanagedType::PhysicsMaterial:
			{
				ColliderMaterial material = storage->GetValue<ColliderMaterial>(managedInstance);
				
				if (PropertyGridHeader(fmt::format("{} (ColliderMaterial)", fieldName)))
				{
					if (Property("Friction", material.Friction))
					{
						storage->SetValue(managedInstance, material);
						result = true;
					}

					if (Property("Restitution", material.Restitution))
					{
						storage->SetValue(managedInstance, material);
						result = true;
					}

					EndTreeNode();
				}

				break;
			}*/
			case FieldType::Texture2D:
			{
				AssetHandle handle = storage->GetValue<AssetHandle>();
				if (PropertyAssetReference<Texture2D>(fieldName.c_str(), handle))
					storage->SetValue(handle);
				break;
			}
			case FieldType::Scene:
			{
				AssetHandle handle = storage->GetValue<AssetHandle>();
				if (PropertyAssetReference<Scene>(fieldName.c_str(), handle))
					storage->SetValue(handle);
				break;
			}
		}

		ImGui::PopID();

		return result;
	}

	template<typename T>
	static bool PropertyAssetReferenceArray(const char* label, AssetHandle& outHandle, Ref<ArrayFieldStorage>& arrayStorage, uint32_t index, intptr_t& elementToRemove, const char* helpText = "", const PropertyAssetReferenceSettings& settings = {})
	{
		bool modified = false;

		ImGuiStyle& style = ImGui::GetStyle();

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);

		if (std::strlen(helpText) != 0)
		{
			ImGui::SameLine();
			HelpMarker(helpText);
		}

		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		const float buttonSize = ImGui::GetFrameHeight();
		float itemWidth = ImMax(1.0f, ImGui::CalcItemWidth() - (buttonSize + style.ItemInnerSpacing.x));
		ImGui::SetNextItemWidth(itemWidth);

		ImVec2 originalButtonTextAlign = style.ButtonTextAlign;
		{
			ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
			float itemHeight = 28.0f;

			std::string buttonText = "Null";
			bool valid = true;
			if (AssetManager::IsAssetHandleValid(outHandle))
			{
				auto object = AssetManager::GetAsset<T>(outHandle);
				valid = object && !object->IsFlagSet(AssetFlag::Invalid) && !object->IsFlagSet(AssetFlag::Missing);
				if (object && !object->IsFlagSet(AssetFlag::Missing))
				{
					buttonText = Project::GetEditorAssetManager()->GetMetadata(outHandle).FilePath.stem().string();
				}
				else
				{
					buttonText = "Missing";
				}
			}

			// PropertyAssetReferenceTarget could be called multiple times in same "context"
			// and so we need a unique id for the asset search popup each time.
			// notes
			// - don't use GenerateID(), that's inviting id clashes, which would be super confusing.
			// - don't store return from GenerateLabelId in a const char* here. Because its pointing to an internal
			//   buffer which may get overwritten by the time you want to use it later on.
			std::string assetSearchPopupID = GenerateLabelID("ARTSP");
			{
				UI::ScopedColour buttonLabelColor(ImGuiCol_Text, valid ? settings.ButtonLabelColor : settings.ButtonLabelColorError);
				ImGui::Button(GenerateLabelID(buttonText), { itemWidth, itemHeight });

				const bool isHovered = ImGui::IsItemHovered();

				if (isHovered)
				{
					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						// NOTE(Peter): Ugly workaround since AssetEditorPanel includes ImGui.h (meaning ImGui.h can't include AssetEditorPanel).
						//				Will rework those includes at a later date...
						AssetEditorPanelInterface::OpenEditor(AssetManager::GetAsset<Asset>(outHandle));
					}
					else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					{
						ImGui::OpenPopup(assetSearchPopupID.c_str());
					}
				}
			}

			ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

			bool clear = false;
			if (Widgets::AssetSearchPopup(assetSearchPopupID.c_str(), T::GetStaticType(), outHandle, &clear))
			{
				if (clear)
					outHandle = 0;
				modified = true;
			}
		}

		if (ImGui::BeginDragDropTarget())
		{
			auto data = ImGui::AcceptDragDropPayload("asset_payload");

			if (data)
			{
				AssetHandle assetHandle = *(AssetHandle*)data->Data;
				s_PropertyAssetReferenceAssetHandle = assetHandle;
				const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(assetHandle);

				if (metadata.Type == T::GetStaticType())
				{
					outHandle = assetHandle;
					modified = true;
				}
			}

			ImGui::EndDragDropTarget();
		}

		if (modified)
			arrayStorage->SetValue<uint64_t>(static_cast<uint32_t>(index), outHandle);

		const ImVec2 backupFramePadding = style.FramePadding;
		style.FramePadding.x = style.FramePadding.y;
		ImGui::SameLine(0, style.ItemInnerSpacing.x);

		if (ImGui::Button(GenerateLabelID("x"), ImVec2(buttonSize, buttonSize)))
			elementToRemove = intptr_t(index);

		style.FramePadding = backupFramePadding;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertyEntityReferenceArray(const char* label, UUID& entityID, Ref<Scene> context, Ref<ArrayFieldStorage>& arrayStorage, uintptr_t index, intptr_t& elementToRemove)
	{
		bool receivedValidEntity = false;

		ImGuiStyle& style = ImGui::GetStyle();

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(label);
		ImGui::NextColumn();
		ShiftCursorY(4.0f);
		ImGui::PushItemWidth(-1);

		const float buttonSize = ImGui::GetFrameHeight();
		float itemWidth = ImMax(1.0f, ImGui::CalcItemWidth() - (buttonSize + style.ItemInnerSpacing.x));
		ImGui::SetNextItemWidth(itemWidth);

		ImVec2 originalButtonTextAlign = ImGui::GetStyle().ButtonTextAlign;
		{
			ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
			float width = ImGui::GetContentRegionAvail().x;
			float itemHeight = 28.0f;

			std::string buttonText = "Null";

			Entity entity = context->TryGetEntityWithUUID(entityID);
			if (entity)
				buttonText = entity.GetComponent<TagComponent>().Tag;

			ImGui::Button(GenerateLabelID(buttonText), { width - buttonSize, itemHeight });
		}
		ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

		if (ImGui::BeginDragDropTarget())
		{
			auto data = ImGui::AcceptDragDropPayload("scene_entity_hierarchy");
			if (data)
			{
				entityID = *(UUID*)data->Data;
				receivedValidEntity = true;
			}

			ImGui::EndDragDropTarget();
		}

		if (receivedValidEntity)
			arrayStorage->SetValue<uint64_t>((uint32_t)index, entityID);

		const ImVec2 backupFramePadding = style.FramePadding;
		style.FramePadding.x = style.FramePadding.y;
		ImGui::SameLine(0, style.ItemInnerSpacing.x);

		if (ImGui::Button(GenerateLabelID("x"), ImVec2(buttonSize, buttonSize)))
			elementToRemove = index;

		style.FramePadding = backupFramePadding;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return receivedValidEntity;
	}

	static bool DrawFieldArray(Ref<Scene> sceneContext, const std::string& fieldName, Ref<ArrayFieldStorage> arrayStorage)
	{
		bool modified = false;
		intptr_t elementToRemove = -1;

		std::string arrayID = fmt::format("{0}FieldArray", fieldName);
		ImGui::PushID(arrayID.c_str());

		ShiftCursor(10.0f, 9.0f);
		ImGui::Text(fieldName.c_str());
		ImGui::NextColumn();
		ImGui::NextColumn();

		uintptr_t length = arrayStorage->GetLength();
		uintptr_t tempLength = length;
		FieldType nativeType = arrayStorage->GetFieldInfo()->Type;

		if (UI::PropertyInput("Length", tempLength, 1, 1, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			arrayStorage->Resize((uint32_t)tempLength);
			length = tempLength;
			modified = true;
		}

		ShiftCursorY(5.0f);

		auto drawScalarElement = [&arrayStorage, &elementToRemove, &arrayID](std::string_view indexString, uintptr_t index, ImGuiDataType dataType, auto& data, int32_t components, const char* format)
		{
			ImGuiStyle& style = ImGui::GetStyle();

			ShiftCursor(10.0f, 9.0f);
			ImGui::Text(indexString.data());
			ImGui::NextColumn();
			ShiftCursorY(4.0f);
			ImGui::PushItemWidth(-1);

			const float buttonSize = ImGui::GetFrameHeight();
			ImGui::SetNextItemWidth(ImMax(1.0f, ImGui::CalcItemWidth() - (buttonSize + style.ItemInnerSpacing.x)));

			size_t dataSize = ImGui::DataTypeGetInfo(dataType)->Size;
			if (components > 1)
			{
				if (UI::DragScalarN(GenerateID(), dataType, &data, components, 1.0f, (const void*)0, (const void*)0, format, 0))
					arrayStorage->SetValue<std::remove_reference_t<decltype(data)>>((uint32_t)index, data);
			}
			else
			{
				if (UI::DragScalar(GenerateID(), dataType, &data, 1.0f, (const void*)0, (const void*)0, format, (ImGuiSliderFlags)0))
					arrayStorage->SetValue<std::remove_reference_t<decltype(data)>>((uint32_t)index, data);
			}

			const ImVec2 backupFramePadding = style.FramePadding;
			style.FramePadding.x = style.FramePadding.y;
			ImGui::SameLine(0, style.ItemInnerSpacing.x);

			if (ImGui::Button(GenerateLabelID("x"), ImVec2(buttonSize, buttonSize)))
				elementToRemove = index;

			style.FramePadding = backupFramePadding;

			ImGui::PopItemWidth();
			ImGui::NextColumn();
			Draw::Underline();
		};

		auto drawStringElement = [&arrayStorage, &elementToRemove](std::string_view indexString, uintptr_t index, std::string& data)
		{
			ImGuiStyle& style = ImGui::GetStyle();

			ShiftCursor(10.0f, 9.0f);
			ImGui::Text(indexString.data());
			ImGui::NextColumn();
			ShiftCursorY(4.0f);
			ImGui::PushItemWidth(-1);

			const float buttonSize = ImGui::GetFrameHeight();
			ImGui::SetNextItemWidth(ImMax(1.0f, ImGui::CalcItemWidth() - (buttonSize + style.ItemInnerSpacing.x)));

			if (UI::InputText(GenerateID(), &data))
				arrayStorage->SetValue<std::string>((uint32_t)index, data);

			const ImVec2 backupFramePadding = style.FramePadding;
			style.FramePadding.x = style.FramePadding.y;
			ImGui::SameLine(0, style.ItemInnerSpacing.x);

			if (ImGui::Button(GenerateLabelID("x"), ImVec2(buttonSize, buttonSize)))
				elementToRemove = index;

			style.FramePadding = backupFramePadding;

			ImGui::PopItemWidth();
			ImGui::NextColumn();
			Draw::Underline();
		};

		for (uint32_t i = 0; i < (uint32_t)length; i++)
		{
			std::string idString = fmt::format("[{0}]{1}-{0}", i, arrayID);
			std::string indexString = fmt::format("[{0}]", i);

			ImGui::PushID(idString.c_str());

			switch (nativeType)
			{
				case FieldType::Bool:
				{
					bool value = arrayStorage->GetValue<bool>(i);
					if (UI::Property(indexString.c_str(), value))
					{
						arrayStorage->SetValue(i, value);
						modified = true;
					}
					break;
				}
				case FieldType::Int8:
				{
					int8_t value = arrayStorage->GetValue<int8_t>(i);
					drawScalarElement(indexString, i, ImGuiDataType_S8, value, 1, "%d");
					break;
				}
				case FieldType::Int16:
				{
					int16_t value = arrayStorage->GetValue<int16_t>(i);
					drawScalarElement(indexString, i, ImGuiDataType_S16, value, 1, "%d");
					break;
				}
				case FieldType::Int32:
				{
					int32_t value = arrayStorage->GetValue<int32_t>(i);
					drawScalarElement(indexString, i, ImGuiDataType_S32, value, 1, "%d");
					break;
				}
				case FieldType::Int64:
				{
					int64_t value = arrayStorage->GetValue<int64_t>(i);
					drawScalarElement(indexString, i, ImGuiDataType_S64, value, 1, "%d");
					break;
				}
				case FieldType::UInt8:
				{
					uint8_t value = arrayStorage->GetValue<uint8_t>(i);
					drawScalarElement(indexString, i, ImGuiDataType_U8, value, 1, "%d");
					break;
				}
				case FieldType::UInt16:
				{
					uint16_t value = arrayStorage->GetValue<uint16_t>(i);
					drawScalarElement(indexString, i, ImGuiDataType_U16, value, 1, "%d");
					break;
				}
				case FieldType::UInt32:
				{
					uint32_t value = arrayStorage->GetValue<uint32_t>(i);
					drawScalarElement(indexString, i, ImGuiDataType_U32, value, 1, "%d");
					break;
				}
				case FieldType::UInt64:
				{
					uint64_t value = arrayStorage->GetValue<uint64_t>(i);
					drawScalarElement(indexString, i, ImGuiDataType_U64, value, 1, "%d");
					break;
				}
				case FieldType::Float:
				{
					float value = arrayStorage->GetValue<float>(i);
					drawScalarElement(indexString, i, ImGuiDataType_Float, value, 1, "%.3f");
					break;
				}
				case FieldType::Double:
				{
					double value = arrayStorage->GetValue<double>(i);
					drawScalarElement(indexString, i, ImGuiDataType_Double, value, 1, "%.6f");
					break;
				}
				case FieldType::String:
				{
					std::string value = arrayStorage->GetValue<std::string>(i);
					drawStringElement(indexString, i, value);
					break;
				}
				case FieldType::Vector2:
				{
					glm::vec2 value = arrayStorage->GetValue<glm::vec2>(i);
					drawScalarElement(indexString, i, ImGuiDataType_Float, value, 2, "%.3f");
					break;
				}
				case FieldType::Vector3:
				{
					glm::vec3 value = arrayStorage->GetValue<glm::vec3>(i);
					drawScalarElement(indexString, i, ImGuiDataType_Float, value, 3, "%.3f");
					break;
				}
				case FieldType::Vector4:
				{
					glm::vec4 value = arrayStorage->GetValue<glm::vec4>(i);
					drawScalarElement(indexString, i, ImGuiDataType_Float, value, 4, "%.3f");
					break;
				}
				case FieldType::Prefab:
				{
					AssetHandle handle = arrayStorage->GetValue<AssetHandle>(i);
					PropertyAssetReferenceArray<Prefab>(indexString.c_str(), handle, arrayStorage, i, elementToRemove);
					break;
				}
				case FieldType::Entity:
				{
					UUID uuid = arrayStorage->GetValue<UUID>(i);
					PropertyEntityReferenceArray(indexString.c_str(), uuid, sceneContext, arrayStorage, i, elementToRemove);
					break;
				}
				case FieldType::Mesh:
				{
					AssetHandle handle = arrayStorage->GetValue<AssetHandle>(i);
					PropertyAssetReferenceArray<Mesh>(indexString.c_str(), handle, arrayStorage, i, elementToRemove);
					break;
				}
				case FieldType::StaticMesh:
				{
					AssetHandle handle = arrayStorage->GetValue<AssetHandle>(i);
					PropertyAssetReferenceArray<StaticMesh>(indexString.c_str(), handle, arrayStorage, i, elementToRemove);
					break;
				}
				case FieldType::Material:
				{
					AssetHandle handle = arrayStorage->GetValue<AssetHandle>(i);
					PropertyAssetReferenceArray<MaterialAsset>(indexString.c_str(), handle, arrayStorage, i, elementToRemove);
					break;
				}
				/*case UnmanagedType::PhysicsMaterial:
				{
					ColliderMaterial material = storage->GetValue<ColliderMaterial>(managedInstance);

					if (PropertyGridHeader(fmt::format("{} (ColliderMaterial)", fieldName)))
					{
						if (Property("Friction", material.Friction))
						{
							storage->SetValue(managedInstance, material);
							result = true;
						}

						if (Property("Restitution", material.Restitution))
						{
							storage->SetValue(managedInstance, material);
							result = true;
						}

						EndTreeNode();
					}

					AssetHandle handle = valueWrapper.GetOrDefault<AssetHandle>(0);
					PropertyAssetReferenceArray<PhysicsMaterial>(indexString.c_str(), handle, managedInstance, arrayStorage, i, elementToRemove);
					break;
				}*/
				case FieldType::Texture2D:
				{
					AssetHandle handle = arrayStorage->GetValue<AssetHandle>(i);
					PropertyAssetReferenceArray<Texture2D>(indexString.c_str(), handle, arrayStorage, i, elementToRemove);
					break;
				}
				case FieldType::Scene:
				{
					AssetHandle handle = arrayStorage->GetValue<AssetHandle>(i);
					PropertyAssetReferenceArray<Scene>(indexString.c_str(), handle, arrayStorage, i, elementToRemove);
					break;
				}
			}

			ImGui::PopID();
		}

		ImGui::PopID();

		if (elementToRemove != -1)
		{
			arrayStorage->RemoveAt((uint32_t)elementToRemove);
			modified = true;
		}

		return modified;
	}

}

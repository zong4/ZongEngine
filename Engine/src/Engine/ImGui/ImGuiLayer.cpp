#include "pch.h"
#include "ImGuiLayer.h"

#include "Colors.h"

#include "Engine/Renderer/Renderer.h"

#include "Engine/Platform/Vulkan/VulkanImGuiLayer.h"

#include "Engine/Renderer/RendererAPI.h"

#include <imgui.h>

// TODO(Yan): WIP
// Defined in imgui_impl_glfw.cpp
// extern bool g_DisableImGuiEvents;

namespace Hazel {
	 
	ImGuiLayer* ImGuiLayer::Create()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return hnew VulkanImGuiLayer();
		}
		ZONG_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void ImGuiLayer::SetDarkThemeColors()
	{
		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Resize Grip
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);

		// Scrollbar
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);

		// Check Mark
		colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);

		// Slider
		colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);

	}

	void ImGuiLayer::SetDarkThemeV2Colors()
	{
		auto& style = ImGui::GetStyle();
		auto& colors = ImGui::GetStyle().Colors;

		//========================================================
		/// Colours

		// Headers
		colors[ImGuiCol_Header]				= ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);
		colors[ImGuiCol_HeaderHovered]		= ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);
		colors[ImGuiCol_HeaderActive]		= ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);

		// Buttons
		colors[ImGuiCol_Button]				= ImColor(56, 56, 56, 200);
		colors[ImGuiCol_ButtonHovered]		= ImColor(70, 70, 70, 255);
		colors[ImGuiCol_ButtonActive]		= ImColor(56, 56, 56, 150);

		// Frame BG
		colors[ImGuiCol_FrameBg]			= ImGui::ColorConvertU32ToFloat4(Colors::Theme::propertyField);
		colors[ImGuiCol_FrameBgHovered]		= ImGui::ColorConvertU32ToFloat4(Colors::Theme::propertyField);
		colors[ImGuiCol_FrameBgActive]		= ImGui::ColorConvertU32ToFloat4(Colors::Theme::propertyField);

		// Tabs
		colors[ImGuiCol_Tab]				= ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
		colors[ImGuiCol_TabHovered]			= ImColor(255, 225, 135, 30);
		colors[ImGuiCol_TabActive]			= ImColor(255, 225, 135, 60);
		colors[ImGuiCol_TabUnfocused]		= ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
		colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabHovered];

		// Title
		colors[ImGuiCol_TitleBg]			= ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
		colors[ImGuiCol_TitleBgActive]		= ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
		colors[ImGuiCol_TitleBgCollapsed]	= ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Resize Grip
		colors[ImGuiCol_ResizeGrip]			= ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered]	= ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
		colors[ImGuiCol_ResizeGripActive]	= ImVec4(0.46f, 0.46f, 0.46f, 0.95f);

		// Scrollbar
		colors[ImGuiCol_ScrollbarBg]		= ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab]		= ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);

		// Check Mark
		colors[ImGuiCol_CheckMark]			= ImColor(200, 200, 200, 255);

		// Slider
		colors[ImGuiCol_SliderGrab]			= ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
		colors[ImGuiCol_SliderGrabActive]	= ImVec4(0.66f, 0.66f, 0.66f, 1.0f);

		// Text
		colors[ImGuiCol_Text]				= ImGui::ColorConvertU32ToFloat4(Colors::Theme::text);

		// Checkbox
		colors[ImGuiCol_CheckMark]			= ImGui::ColorConvertU32ToFloat4(Colors::Theme::text);

		// Separator
		colors[ImGuiCol_Separator]			= ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundDark);
		colors[ImGuiCol_SeparatorActive]	= ImGui::ColorConvertU32ToFloat4(Colors::Theme::highlight);
		colors[ImGuiCol_SeparatorHovered]	= ImColor(39, 185, 242, 150);

		// Window Background
		colors[ImGuiCol_WindowBg]			= ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
		colors[ImGuiCol_ChildBg]			= ImGui::ColorConvertU32ToFloat4(Colors::Theme::background);
		colors[ImGuiCol_PopupBg]			= ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundPopup);
		colors[ImGuiCol_Border]				= ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundDark);

		// Tables
		colors[ImGuiCol_TableHeaderBg]		= ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);
		colors[ImGuiCol_TableBorderLight]	= ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundDark);

		// Menubar
		colors[ImGuiCol_MenuBarBg]			= ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f };

		//========================================================
		/// Style
		style.FrameRounding = 2.5f;
		style.FrameBorderSize = 1.0f;
		style.IndentSpacing = 11.0f;
	}

	void ImGuiLayer::AllowInputEvents(bool allowEvents)
	{
		// TODO
		// g_DisableImGuiEvents = !allowEvents;
	}

}

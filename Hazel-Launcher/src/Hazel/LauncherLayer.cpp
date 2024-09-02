#include "LauncherLayer.h"

#include "Walnut/Application.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "Walnut/ImGuiUtilities.h"

#include "Hazel/Tiering/TieringSerializer.h"
#include "Hazel/Core/ApplicationSettings.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

#include <Windows.h>

using namespace Walnut;

namespace Hazel {

#include "Embed/HazelDefaultLauncherHeaderImage.embed"
#include "Embed/HazelGradientLogo.embed"
#include "Embed/WindowImages.embed"

	namespace Theme {
		constexpr auto accent = IM_COL32(236, 158, 36, 255);
		constexpr auto highlight = IM_COL32(39, 185, 242, 255);
		constexpr auto niceBlue = IM_COL32(83, 232, 254, 255);
		constexpr auto compliment = IM_COL32(78, 151, 166, 255);
		constexpr auto background = IM_COL32(36, 36, 36, 255);
		constexpr auto backgroundDark = IM_COL32(26, 26, 26, 255);
		constexpr auto titlebar = IM_COL32(21, 21, 21, 255);
		constexpr auto propertyField = IM_COL32(15, 15, 15, 255);
		constexpr auto text = IM_COL32(192, 192, 192, 255);
		constexpr auto textBrighter = IM_COL32(210, 210, 210, 255);
		constexpr auto textDarker = IM_COL32(128, 128, 128, 255);
		constexpr auto muted = IM_COL32(77, 77, 77, 255);
		constexpr auto groupHeader = IM_COL32(47, 47, 47, 255);
		//constexpr auto selection				= IM_COL32(191, 177, 155, 255);
		//constexpr auto selectionMuted			= IM_COL32(59, 57, 45, 255);
		constexpr auto selection = IM_COL32(237, 192, 119, 255);
		constexpr auto selectionMuted = IM_COL32(237, 201, 142, 23);

		//constexpr auto backgroundPopup			= IM_COL32(63, 73, 77, 255); // in between
		//constexpr auto backgroundPopup			= IM_COL32(63, 77, 76, 255); // most green
		constexpr auto backgroundPopup = IM_COL32(63, 70, 77, 255); // most blue
	}

	static void SetDarkThemeV2Colors()
	{
		auto& style = ImGui::GetStyle();
		auto& colors = ImGui::GetStyle().Colors;

		//========================================================
		/// Colours

		// Headers
		colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(Theme::groupHeader);
		colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(Theme::groupHeader);
		colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(Theme::groupHeader);

		// Buttons
		colors[ImGuiCol_Button] = ImColor(56, 56, 56, 200);
		colors[ImGuiCol_ButtonHovered] = ImColor(70, 70, 70, 255);
		colors[ImGuiCol_ButtonActive] = ImColor(56, 56, 56, 150);

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(Theme::propertyField);
		colors[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(Theme::propertyField);
		colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(Theme::propertyField);

		// Tabs
		colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(Theme::titlebar);
		colors[ImGuiCol_TabHovered] = ImColor(255, 225, 135, 30);
		colors[ImGuiCol_TabActive] = ImColor(255, 225, 135, 60);
		colors[ImGuiCol_TabUnfocused] = ImGui::ColorConvertU32ToFloat4(Theme::titlebar);
		colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabHovered];

		// Title
		colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(Theme::titlebar);
		colors[ImGuiCol_TitleBgActive] = ImGui::ColorConvertU32ToFloat4(Theme::titlebar);
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
		colors[ImGuiCol_CheckMark] = ImColor(200, 200, 200, 255);

		// Slider
		colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);

		// Text
		colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(Theme::text);

		// Checkbox
		colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(Theme::text);

		// Separator
		colors[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(Theme::backgroundDark);
		colors[ImGuiCol_SeparatorActive] = ImGui::ColorConvertU32ToFloat4(Theme::highlight);
		colors[ImGuiCol_SeparatorHovered] = ImColor(39, 185, 242, 150);

		// Window Background
		colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(Theme::titlebar);
		colors[ImGuiCol_ChildBg] = ImGui::ColorConvertU32ToFloat4(Theme::background);
		colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(Theme::backgroundPopup);
		colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(Theme::backgroundDark);

		// Tables
		colors[ImGuiCol_TableHeaderBg] = ImGui::ColorConvertU32ToFloat4(Theme::groupHeader);
		colors[ImGuiCol_TableBorderLight] = ImGui::ColorConvertU32ToFloat4(Theme::backgroundDark);

		// Menubar
		colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f };

		//========================================================
		/// Style
		style.FrameRounding = 2.5f;
		style.FrameBorderSize = 1.0f;
		style.IndentSpacing = 11.0f;
	}

	static int s_UIContextID = 0;
	static uint32_t s_Counter = 0;
	static char s_IDBuffer[16] = "##";
	static char s_LabelIDBuffer[1024];
	static char* s_MultilineBuffer = nullptr;
	static bool s_ReturnAfterDeactivation = false;

	static const char* GenerateID()
	{
		itoa(s_Counter++, s_IDBuffer + 2, 16);
		return s_IDBuffer;
	}

	static void HelpMarker(const char* desc)
	{
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

	LauncherLayer::LauncherLayer(const LauncherSpecification& specification)
		: m_Specification(specification)
	{
	}

	void LauncherLayer::OnAttach()
	{
#if 0
		ApplicationSettings settings(s_AppSettingsPath);
		settings.Set("Hello", "Lol");
		settings.SetFloat("Float", 5.526f);
		settings.SetInt("Int", 9817598);
		settings.Serialize();
		settings.Deserialize();
		system("pause");
#endif

#if 0
		int count;
		const GLFWvidmode* videoModes = glfwGetVideoModes(glfwGetPrimaryMonitor(), &count);
		
		for (int i = 0; i < count; i++)
			m_Resolutions.insert({ videoModes[i].width, videoModes[i].height,  videoModes[i].refreshRate });

		for (const auto& res : m_Resolutions)
		{
			std::cout << res.Width << ", " << res.Height << " @ " << res.RefreshRate << std::endl;
		}
#endif

		SetDarkThemeV2Colors();

		{
			uint32_t w, h;
			void* data = Image::Decode(g_HazelDefaultLauncherHeaderImage, sizeof(g_HazelDefaultLauncherHeaderImage), w, h);
			m_CoverImage = std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
			free(data);
		}
		{
			uint32_t w, h;
			void* data = Image::Decode(g_HazelGradientLogo, sizeof(g_HazelGradientLogo), w, h);
			m_LogoTex = std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
			free(data);
		}
		{
			uint32_t w, h;
			void* data = Image::Decode(g_WindowMinimizeImage, sizeof(g_WindowMinimizeImage), w, h);
			m_IconMinimize = std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
			free(data);
		}
		{
			uint32_t w, h;
			void* data = Image::Decode(g_WindowCloseImage, sizeof(g_WindowCloseImage), w, h);
			m_IconClose = std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
			free(data);
		}

		Application::Get().SetWindowUserData(this);

		glfwSetTitlebarHitTestCallback(Application::Get().GetWindow(), [](GLFWwindow* window, int x, int y, int* hit)
		{
			LauncherLayer* layer = (LauncherLayer*)glfwGetWindowUserPointer(window);
			*hit = layer->IsTitleBarHovered();
		});

		TieringSerializer::Deserialize(m_TieringSettings, m_Specification.TieringSettingsPath);
	}

	void LauncherLayer::OnUIRender()
	{
		s_Counter = 0;

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y));
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("Header", nullptr, window_flags);
		float titleBarHeight = UI_DrawTitlebar();
		ImGui::SetCursorPosY(titleBarHeight);

		// Cover image
		ImGui::Image(m_CoverImage->GetDescriptorSet(), { 960.0f, 230.0f });

		float y = ImGui::GetCursorPosY();

		ImGui::End();

		ImGui::PopStyleVar();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(80.0f, 20.0f));

		ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + y));

		float remainingVerticalSpace = viewport->Size.y - y;
		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, remainingVerticalSpace));

		ImGui::Begin("Settings", nullptr, window_flags);

		// Draw the settings popup
		{
			const bool darkBackground = false;

			int32_t sectionIdx = 0;

			float popupWidth = viewport->Size.x;

			auto beginSection = [&sectionIdx, darkBackground, popupWidth](const char* name)
			{
				if (sectionIdx > 0)
					UI::ShiftCursorY(5.5f);

				ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
				ImGui::TextUnformatted(name);
				ImGui::PopFont();
				UI::Draw::Underline(darkBackground ? Theme::background : Theme::backgroundDark);
				UI::ShiftCursorY(3.5f);

				bool result = ImGui::BeginTable("##section_table", 2, ImGuiTableFlags_SizingStretchSame);
				if (result)
				{
				//	ImGui::TableSetupColumn("Labels", ImGuiTableColumnFlags_WidthFixed, popupWidth * 0.2f);
				//	ImGui::TableSetupColumn("Widgets", ImGuiTableColumnFlags_WidthFixed, popupWidth * 0.8f);
				}

				sectionIdx++;
				return result;
			};

			auto endSection = []()
			{
				ImGui::EndTable();
			};

			auto text = [](const char* text, ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f ))
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::SetNextItemWidth(-1);
				return ImGui::TextColored(color, text);
			};

			auto slider = [](const char* label, float& value, float min = 0.0f, float max = 0.0f)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(label);
				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-1);
				return ImGui::SliderFloat(GenerateID(), &value, min, max);
			};
			
			auto sliderInt = [](const char* label, int& value, int min = 0, int max = 0)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(label);
				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-1);
				return ImGui::SliderInt(GenerateID(), &value, min, max);
			};

			auto drag = [](const char* label, float& value, float delta = 1.0f, float min = 0.0f, float max = 0.0f)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(label);
				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-1);
				return ImGui::DragFloat(GenerateID(), &value, delta, min, max);
			};

			auto checkbox = [](const char* label, bool& value)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(label);
				ImGui::TableSetColumnIndex(1);
				auto table = ImGui::GetCurrentTable();
				float columnWidth = ImGui::TableGetMaxColumnWidth(table, 1);
				UI::ShiftCursorX(columnWidth - ImGui::GetFrameHeight() - ImGui::GetStyle().ItemInnerSpacing.x);
				return ImGui::Checkbox(GenerateID(), &value);
			};

			auto dropdown = [](const char* label, const char** options, int32_t optionCount, int32_t* selected)
			{
				const char* current = options[*selected];
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(label);
				ImGui::TableSetColumnIndex(1);
				ImGui::PushItemWidth(-1);

				bool result = false;
				if (ImGui::BeginCombo(GenerateID(), current))
				{
					for (int i = 0; i < optionCount; i++)
					{
						const bool is_selected = (current == options[i]);
						if (ImGui::Selectable(options[i], is_selected))
						{
							current = options[i];
							*selected = i;
							result = true;
						}

						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				ImGui::PopItemWidth();

				return result;
			};

			UI::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(0, 5.5f));
			UI::ScopedStyle windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
			UI::ScopedStyle windowRounding(ImGuiStyleVar_PopupRounding, 4.0f);
			UI::ScopedColour windowColor(ImGuiCol_PopupBg, darkBackground ? Theme::backgroundDark : Theme::background);
			UI::ScopedStyle cellPadding(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 5.5f));
			{
#if 0
				if (beginSection("Game"))
				{
					text("Note: you can plug in a controller to play", ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
					sliderInt("Mouse/controller sensitivity", m_MouseSensitivity, 1, 10);
					
					{
					//	static const char* selectionModes[] = { "Off", "Low", "Medium", "High" };
					//	int particleLevel = (int)m_TieringSettings.RendererTS.AOQuality;
					//	if (dropdown("Ambient Occlusion", selectionModes, 3, &aoQuality))
					//		m_TieringSettings.RendererTS.AOQuality = (Tiering::Renderer::AmbientOcclusionQualitySetting)aoQuality;
					}

					endSection();
				}
#endif

				if (beginSection("Graphics"))
				{
					{
						static const char* selectionModes[] = { "0.25", "0.5", "0.67", "0.75", "1.0"};
						static float selectionModesFloat[] = { 0.25f, 0.5f, 0.67f, 0.75f, 1.0f};

						int rendererScale = 4;
						float rendererScaleF = m_TieringSettings.RendererTS.RendererScale;

						for (int i = 0; i < 5; i++)
						{
							if (glm::abs<float>(rendererScaleF - selectionModesFloat[i]) < 0.01f)
							{
								rendererScale = i;
								break;
							}

						}

						if (dropdown("Renderer Scale", selectionModes, 5, &rendererScale))
						{
							m_TieringSettings.RendererTS.RendererScale = selectionModesFloat[rendererScale];
						}

						if (ImGui::IsItemHovered())
						{
							ImGui::BeginTooltip();
							ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
							ImGui::TextUnformatted("Rendering resolution - reducing this will increase GPU performance.\n1.0 = native display resolution");
							ImGui::PopTextWrapPos();
							ImGui::EndTooltip();
						}

						{
							static const char* selectionModes[] = { "Windowed", "Fullscreen" };
							int mode = m_TieringSettings.RendererTS.Windowed ? 0 : 1;
							if (dropdown("Fullscreen Mode", selectionModes, 2, &mode))
								m_TieringSettings.RendererTS.Windowed = mode == 0;
						}

						{
							checkbox("VSync", m_TieringSettings.RendererTS.VSync);
						}

						ImGui::Separator();
					}

					{
						static const char* selectionModes[] = { "Low", "High" };
						int shadowQuality = (int)m_TieringSettings.RendererTS.ShadowQuality - 1;
						if (dropdown("Shadow Quality", selectionModes, 2, &shadowQuality))
							m_TieringSettings.RendererTS.ShadowQuality = (Tiering::Renderer::ShadowQualitySetting)(shadowQuality + 1);
					}
					{
						static const char* selectionModes[] = { "Low", "Medium", "High" };
						int shadowResolution = (int)m_TieringSettings.RendererTS.ShadowResolution - 1;
						if (dropdown("Shadow Resolution", selectionModes, 3, &shadowResolution))
							m_TieringSettings.RendererTS.ShadowResolution = (Tiering::Renderer::ShadowResolutionSetting)(shadowResolution + 1);
					}
					{
						static const char* selectionModes[] = { "Off", "High", "Ultra" };
						int aoQuality = (int)m_TieringSettings.RendererTS.AOQuality;
						if (dropdown("Ambient Occlusion", selectionModes, 3, &aoQuality))
							m_TieringSettings.RendererTS.AOQuality = (Tiering::Renderer::AmbientOcclusionQualitySetting)aoQuality;
					}
					{
						static const char* selectionModes[] = { "Off", "Medium", "High" };
						int ssrQuality = (int)m_TieringSettings.RendererTS.SSRQuality;
						if (dropdown("Screen Space Reflections", selectionModes, 3, &ssrQuality))
							m_TieringSettings.RendererTS.SSRQuality = (Tiering::Renderer::SSRQualitySetting)ssrQuality;
					}
				

					endSection();
				}
			}
		}

		auto avail = ImGui::GetContentRegionAvail();
		ImGui::SetCursorPos(ImVec2(avail.x * 0.5f + 50.0f - 80.0f * 0.5f, remainingVerticalSpace - 40.0f - 20.0f));
		if (ImGui::Button("Play", { 80.0f, 40.0f }))
		{
			TieringSerializer::Serialize(m_TieringSettings, m_Specification.TieringSettingsPath);

			PROCESS_INFORMATION processInfo;
			STARTUPINFOA startupInfo;
			ZeroMemory(&startupInfo, sizeof(STARTUPINFOA));
			startupInfo.cb = sizeof(startupInfo);

			std::string appExecutablePathStr = m_Specification.AppExecutablePath.string();

			if (std::filesystem::exists(appExecutablePathStr))
			{
				bool result = CreateProcessA(appExecutablePathStr.c_str(), NULL, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, m_Specification.AppExecutableTitle.c_str(), &startupInfo, &processInfo);
				if (result)
				{
					CloseHandle(processInfo.hThread);
					CloseHandle(processInfo.hProcess);
					Application::Get().Close();
				}
			}
			else
			{
				m_ShowErrorDialog = true;

			}
		}

		ImGui::PopStyleVar(3);

		ImGui::End();

		if (m_ShowErrorDialog)
		{
			m_ShowErrorDialog = false;
			ImGui::OpenPopup("Launch Error");
			ImGui::SetNextWindowSize(ImVec2(300, 120));
		}

		if (ImGui::BeginPopupModal("Launch Error", nullptr))
		{
			ImGui::Text("Failed to launch executable:");
			auto str = m_Specification.AppExecutablePath.string();
			ImGui::Text(str.c_str());

			
			if (ImGui::Button("Close"))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}

	float LauncherLayer::UI_DrawTitlebar()
	{
		const float titlebarHeight = 57.0f;
		const ImVec2 windowPadding = ImGui::GetCurrentWindow()->WindowPadding;

		ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y));
		const ImVec2 titlebarMin = ImGui::GetCursorScreenPos();
		const ImVec2 titlebarMax = { ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth() - windowPadding.y * 2.0f,
									 ImGui::GetCursorScreenPos().y + titlebarHeight };
		auto* drawList = ImGui::GetForegroundDrawList();
		drawList->AddRectFilled(titlebarMin, titlebarMax, Theme::titlebar);

		// Logo
		{
			const int logoWidth = 29;// m_LogoTex->GetWidth();
			const int logoHeight = 41;// m_LogoTex->GetHeight();
			const ImVec2 logoOffset(16.0f + windowPadding.x, 8.0f + windowPadding.y);
			const ImVec2 logoRectStart = { ImGui::GetItemRectMin().x + logoOffset.x, ImGui::GetItemRectMin().y + logoOffset.y };
			const ImVec2 logoRectMax = { logoRectStart.x + logoWidth, logoRectStart.y + logoHeight };
			drawList->AddImage(m_LogoTex->GetDescriptorSet(), logoRectStart, logoRectMax);
		}

		ImGui::BeginHorizontal("Titlebar", { ImGui::GetWindowWidth() - windowPadding.y * 2.0f, ImGui::GetFrameHeightWithSpacing() });

		static float moveOffsetX;
		static float moveOffsetY;
		const float w = ImGui::GetContentRegionAvail().x;
		const float buttonsAreaWidth = 94;

		// Title bar drag area

		// On Windows we hook into the GLFW win32 window internals

		ImGui::InvisibleButton("##titleBarDragZone", ImVec2(w - buttonsAreaWidth, titlebarHeight));
		m_TitleBarHovered = ImGui::IsItemHovered();

		// Draw Menubar
#if MENUBAR
		ImGui::SuspendLayout();
		{
			ImGui::SetItemAllowOverlap();
			const float logoOffset = 16.0f * 2.0f + 41.0f + windowPadding.x;
			ImGui::SetCursorPos(ImVec2(logoOffset, 4.0f));
			//UI_DrawMenubar();

			if (ImGui::IsItemHovered())
				m_TitleBarHovered = false;
		}

		const float menuBarRight = ImGui::GetItemRectMax().x - ImGui::GetCurrentWindow()->Pos.x;

		// Project name
		{

			UI::ScopedColour textColour(ImGuiCol_Text, Theme::textDarker);
			UI::ScopedColour border(ImGuiCol_Border, IM_COL32(40, 40, 40, 255));

			const std::string title = Project::GetActive()->GetConfig().Name;
			const ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
			const float rightOffset = ImGui::GetWindowWidth() / 5.0f;
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - rightOffset - textSize.x);
			UI::ShiftCursorY(1.0f + windowPadding.y);

			{
				UI::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[0]);
				ImGui::Text(title.c_str());
			}
			//UI::SetTooltip("Current project (" + Project::GetActive()->GetConfig().ProjectFileName + ")");

			UI::DrawBorder(UI::RectExpanded(UI::GetItemRect(), 24.0f, 68.0f), 1.0f, 3.0f, 0.0f, -60.0f);
		}

		// Current Scene name
		{
			UI::ScopedColour textColour(ImGuiCol_Text, Theme::text);
			const std::string sceneName = m_CurrentScene->GetName();

			ImGui::SetCursorPosX(menuBarRight);
			UI::ShiftCursorX(50.0f);

			{
				UI::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[0]);
				ImGui::Text(sceneName.c_str());
			}
			UI::SetTooltip("Current scene (" + m_SceneFilePath + ")");

			const float underlineThickness = 2.0f;
			const float underlineExpandWidth = 4.0f;
			ImRect itemRect = UI::RectExpanded(UI::GetItemRect(), underlineExpandWidth, 0.0f);

			// Horizontal line
			//itemRect.Min.y = itemRect.Max.y - underlineThickness;
			//itemRect = UI::RectOffset(itemRect, 0.0f, underlineThickness * 2.0f);

			// Vertical line
			itemRect.Max.x = itemRect.Min.x + underlineThickness;
			itemRect = UI::RectOffset(itemRect, -underlineThickness * 2.0f, 0.0f);

			drawList->AddRectFilled(itemRect.Min, itemRect.Max, Theme::muted, 2.0f);
		}
		ImGui::ResumeLayout();
#endif

		// Window buttons
		const ImU32 buttonColN = UI::ColourWithMultipliedValue(Theme::text, 0.9f);
		const ImU32 buttonColH = UI::ColourWithMultipliedValue(Theme::text, 1.2f);
		const ImU32 buttonColP = Theme::textDarker;
		const float buttonWidth = 14.0f;
		const float buttonHeight = 14.0f;

		// Minimize Button

		ImGui::Spring();
		UI::ShiftCursorY(8.0f);
		{
			const int iconWidth = m_IconMinimize->GetWidth();
			const int iconHeight = m_IconMinimize->GetHeight();
			const float padY = (buttonHeight - (float)iconHeight) / 2.0f;
			if (ImGui::InvisibleButton("Minimize", ImVec2(buttonWidth, buttonHeight)))
			{
				// TODO: move this stuff to a better place, like Window class
				if (auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow()))
				{
					Application::Get().QueueEvent([window]() { glfwIconifyWindow(window); });
				}
			}

			UI::DrawButtonImage(m_IconMinimize, buttonColN, buttonColH, buttonColP, UI::RectExpanded(UI::GetItemRect(), 0.0f, -padY));
		}


#if MAXIMIZE
		// Maximize Button

		ImGui::Spring(-1.0f, 17.0f);
		UI::ShiftCursorY(8.0f);
		{
			const int iconWidth = m_IconMaximize->GetWidth();
			const int iconHeight = m_IconMaximize->GetHeight();

			auto* window = static_cast<GLFWwindow*>(Application::Get()->GetWindow());
			bool isMaximized = (bool)glfwGetWindowAttrib(window, GLFW_MAXIMIZED);

			if (ImGui::InvisibleButton("Maximize", ImVec2(buttonWidth, buttonHeight)))
			{
				// TODO: move this stuff to a better place, like Window class
				if (isMaximized)
				{
					//Application::Get().QueueEvent([window]() { glfwRestoreWindow(window); });
				}
				else
				{
					//Application::Get().QueueEvent([]() { Application::Get().GetWindow().Maximize(); });
				}
			}

			UI::DrawButtonImage(isMaximized ? m_IconRestore : m_IconMaximize, buttonColN, buttonColH, buttonColP);
		}
#endif


		// Close Button
		ImGui::Spring(-1.0f, 15.0f);
		UI::ShiftCursorY(8.0f);
		{
			const int iconWidth = m_IconClose->GetWidth();
			const int iconHeight = m_IconClose->GetHeight();
			if (ImGui::InvisibleButton("Close", ImVec2(buttonWidth, buttonHeight)))
				Application::Get().Close();

			UI::DrawButtonImage(m_IconClose, Theme::text, UI::ColourWithMultipliedValue(Theme::text, 1.4f), buttonColP);
		}

		ImGui::Spring(-1.0f, 18.0f);
		ImGui::EndHorizontal();

		return titlebarHeight;
	}

}

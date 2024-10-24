#include "pch.h"
#include "EditorConsolePanel.h"
#include "Engine/Core/Application.h"
#include "Engine/Core/Events/SceneEvents.h"
#include "Engine/Editor/EditorResources.h"
#include "Engine/Editor/FontAwesome.h"
#include "Engine/ImGui/Colors.h"
#include "Engine/ImGui/ImGui.h"

#include "imgui/imgui_internal.h"

namespace Engine {

	static EditorConsolePanel* s_Instance = nullptr;

	static const ImVec4 s_InfoTint = ImVec4(0.0f, 0.431372549f, 1.0f, 1.0f);
	static const ImVec4 s_WarningTint = ImVec4(1.0f, 0.890196078f, 0.0588235294f, 1.0f);
	static const ImVec4 s_ErrorTint = ImVec4(1.0f, 0.309803922f, 0.309803922f, 1.0f);

	EditorConsolePanel::EditorConsolePanel()
	{
		ZONG_CORE_ASSERT(s_Instance == nullptr);
		s_Instance = this;

		m_MessageBuffer.reserve(500);
	}

	EditorConsolePanel::~EditorConsolePanel()
	{
		s_Instance = nullptr;
	}

	void EditorConsolePanel::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<ScenePreStartEvent>([this](ScenePreStartEvent& e)
		{
			if (m_ClearOnPlay)
			{
				std::scoped_lock<std::mutex> lock(m_MessageBufferMutex);
				m_MessageBuffer.clear();
			}
			return false;
		});
	}

	void EditorConsolePanel::OnImGuiRender(bool& isOpen)
	{
		if (ImGui::Begin("Log", &isOpen))
		{
			ImVec2 consoleSize = ImGui::GetContentRegionAvail();
			consoleSize.y -= 32.0f;

			RenderMenu({ consoleSize.x, 28.0f });
			RenderConsole(consoleSize);
		}
		ImGui::End();
	}

	void EditorConsolePanel::OnProjectChanged(const Ref<Project>& project)
	{
		std::scoped_lock<std::mutex> lock(m_MessageBufferMutex);
		m_MessageBuffer.clear();
	}

	void EditorConsolePanel::RenderMenu(const ImVec2& size)
	{
		UI::ScopedStyleStack frame(ImGuiStyleVar_FrameBorderSize, 0.0f, ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::BeginChild("Toolbar", size);

		if (ImGui::Button("Clear", { 75.0f, 28.0f }))
		{
			std::scoped_lock<std::mutex> lock(m_MessageBufferMutex);
			m_MessageBuffer.clear();
		}

		ImGui::SameLine();

		const auto& style = ImGui::GetStyle();
		const std::string clearOnPlayText = fmt::format("{} Clear on Play", m_ClearOnPlay ? ZONG_ICON_CHECK : ZONG_ICON_TIMES);
		ImVec4 textColor = m_ClearOnPlay ? style.Colors[ImGuiCol_Text] : style.Colors[ImGuiCol_TextDisabled];
		if (UI::ColoredButton(clearOnPlayText.c_str(), GetToolbarButtonColor(m_ClearOnPlay), textColor, ImVec2(110.0f, 28.0f)))
			m_ClearOnPlay = !m_ClearOnPlay;

		{
			const ImVec2 buttonSize(28.0f, 28.0f);

			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 100.0f, 0.0f);
			textColor = (m_MessageFilters & (int16_t)ConsoleMessageFlags::Info) ? s_InfoTint : style.Colors[ImGuiCol_TextDisabled];
			if (UI::ColoredButton(ZONG_ICON_INFO_CIRCLE, GetToolbarButtonColor(m_MessageFilters & (int16_t)ConsoleMessageFlags::Info), textColor, buttonSize))
				m_MessageFilters ^= (int16_t)ConsoleMessageFlags::Info;

			ImGui::SameLine();
			textColor = (m_MessageFilters & (int16_t)ConsoleMessageFlags::Warning) ? s_WarningTint : style.Colors[ImGuiCol_TextDisabled];
			if (UI::ColoredButton(ZONG_ICON_EXCLAMATION_TRIANGLE, GetToolbarButtonColor(m_MessageFilters & (int16_t)ConsoleMessageFlags::Warning), textColor, buttonSize))
				m_MessageFilters ^= (int16_t)ConsoleMessageFlags::Warning;

			ImGui::SameLine();
			textColor = (m_MessageFilters & (int16_t)ConsoleMessageFlags::Error) ? s_ErrorTint : style.Colors[ImGuiCol_TextDisabled];
			if (UI::ColoredButton(ZONG_ICON_EXCLAMATION_CIRCLE, GetToolbarButtonColor(m_MessageFilters & (int16_t)ConsoleMessageFlags::Error), textColor, buttonSize))
				m_MessageFilters ^= (int16_t)ConsoleMessageFlags::Error;
		}

		ImGui::EndChild();
	}

	void EditorConsolePanel::RenderConsole(const ImVec2& size)
	{
		static const char* s_Columns[] = { "Type", "Timestamp", "Message" };

		UI::Table("Console", s_Columns, 3, size, [&]()
		{
			std::scoped_lock<std::mutex> lock(m_MessageBufferMutex);
			
			float scrollY = ImGui::GetScrollY();
			if (scrollY < m_PreviousScrollY)
				m_EnableScrollToLatest = false;

			if (scrollY >= ImGui::GetScrollMaxY())
				m_EnableScrollToLatest = true;

			m_PreviousScrollY = scrollY;

			float rowHeight = 24.0f;
			for (uint32_t i = 0; i < m_MessageBuffer.size(); i++)
			{
				const auto& msg = m_MessageBuffer[i];

				if (!(m_MessageFilters & (int16_t)msg.Flags))
					continue;

				ImGui::PushID(&msg);

				const bool clicked = UI::TableRowClickable(msg.ShortMessage.c_str(), rowHeight);

				UI::Separator(ImVec2(4.0f, ImGui::CalcTextSize(msg.ShortMessage.c_str()).y), GetMessageColor(msg));
				ImGui::SameLine();
				ImGui::Text(GetMessageType(msg));
				ImGui::TableNextColumn();
				UI::ShiftCursorX(4.0f);

				std::stringstream timeString;
				tm* timeBuffer = localtime(&msg.Time);
				timeString << std::put_time(timeBuffer, "%T");
				ImGui::Text(timeString.str().c_str());

				ImGui::TableNextColumn();
				UI::ShiftCursorX(4.0f);
				ImGui::Text(msg.ShortMessage.c_str());

				if (i == m_MessageBuffer.size() - 1 && m_ScrollToLatest)
				{
					ImGui::ScrollToItem();
					m_ScrollToLatest = false;
				}

				if (clicked)
				{
					ImGui::OpenPopup("Detailed Message");
					auto[width, height] = Application::Get().GetWindow().GetSize();
					auto[xPos, yPos] = Application::Get().GetWindow().GetWindowPos();
					//ImVec2 size = ImGui::GetMainViewport()->Size;
					ImGui::SetNextWindowSize({ (float)width * 0.5f, (float)height * 0.5f });
					ImGui::SetNextWindowPos({ xPos + (float)width / 2.0f, yPos + (float)height / 2.5f }, 0, { 0.5, 0.5 });
					m_DetailedPanelOpen = true;
				}

				if (m_DetailedPanelOpen)
				{
					UI::ScopedStyle windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
					UI::ScopedStyle framePadding(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 8.0f));

					if (ImGui::BeginPopupModal("Detailed Message", &m_DetailedPanelOpen, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
					{
						ImGui::TextWrapped(msg.LongMessage.c_str());
						if (ImGui::Button("Copy To Clipboard", ImVec2(120.0f, 28.0f)))
						{
							ImGui::SetClipboardText(msg.LongMessage.c_str());
						}
						ImGui::EndPopup();
					}
				}

				ImGui::PopID();
			}
		});
	}

	const char* EditorConsolePanel::GetMessageType(const ConsoleMessage& message) const
	{
		if (message.Flags & (int16_t)ConsoleMessageFlags::Info) return "Info";
		if (message.Flags & (int16_t)ConsoleMessageFlags::Warning) return "Warning";
		if (message.Flags & (int16_t)ConsoleMessageFlags::Error) return "Error";
		return "Unknown";
	}

	const ImVec4& EditorConsolePanel::GetMessageColor(const ConsoleMessage& message) const
	{
		//if (message.Flags & (int16_t)ConsoleMessageFlags::Info) return s_InfoButtonOnTint;
		if (message.Flags & (int16_t)ConsoleMessageFlags::Warning) return s_WarningTint;
		if (message.Flags & (int16_t)ConsoleMessageFlags::Error) return s_ErrorTint;
		return s_InfoTint;
	}

	ImVec4 EditorConsolePanel::GetToolbarButtonColor(const bool value) const
	{
		const auto& style = ImGui::GetStyle();
		return value ? style.Colors[ImGuiCol_Header] : style.Colors[ImGuiCol_FrameBg];
	}

	void EditorConsolePanel::PushMessage(const ConsoleMessage& message)
	{
		if (s_Instance == nullptr)
			return;

		{
			std::scoped_lock<std::mutex> lock(s_Instance->m_MessageBufferMutex);
			s_Instance->m_MessageBuffer.push_back(message);
		}

		if (s_Instance->m_EnableScrollToLatest)
			s_Instance->m_ScrollToLatest = true;
	}

}

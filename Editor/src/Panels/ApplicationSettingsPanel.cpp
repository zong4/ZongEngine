#include "ApplicationSettingsPanel.h"

#include "Engine/Editor/EditorApplicationSettings.h"
#include "Engine/Editor/EditorResources.h"

#include "Engine/ImGui/ImGui.h"

namespace Engine {

	ApplicationSettingsPanel::ApplicationSettingsPanel()
	{
		m_Pages.push_back({ "Editor", [this](){ DrawEditorPage(); }});
		m_Pages.push_back({ "Scripting", [this](){ DrawScriptingPage(); }});
		m_Pages.push_back({ "Renderer", [this]() { DrawRendererPage(); } });
	}

	ApplicationSettingsPanel::~ApplicationSettingsPanel()
	{
		m_Pages.clear();
	}

	void ApplicationSettingsPanel::OnImGuiRender(bool& isOpen)
	{
		if (!isOpen)
			return;

		if (ImGui::Begin("Application Settings", &isOpen))
		{
			UI::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
			UI::ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

			ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable
				| ImGuiTableFlags_SizingFixedFit
				| ImGuiTableFlags_BordersInnerV;

			UI::PushID();
			if (ImGui::BeginTable("", 2, tableFlags, ImVec2(ImGui::GetContentRegionAvail().x,
				ImGui::GetContentRegionAvail().y - 8.0f)))
			{
				ImGui::TableSetupColumn("Page List", 0, 300.0f);
				ImGui::TableSetupColumn("Page Contents", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				DrawPageList();

				ImGui::TableSetColumnIndex(1);
				if (m_CurrentPage < m_Pages.size())
				{
					ImGui::BeginChild("##page_contents", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetWindowHeight()));
					m_Pages[m_CurrentPage].RenderFunction();
					ImGui::EndChild();
				}
				else
				{
					ZONG_CORE_ERROR_TAG("ApplicationSettingsPanel", "Invalid page selected!");
				}

				ImGui::EndTable();
			}

			UI::PopID();
		}

		ImGui::End();
	}

	void ApplicationSettingsPanel::DrawPageList()
	{
		if (!ImGui::BeginChild("##page_list"))
			return;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32_DISABLE);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32_DISABLE);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32_DISABLE);

		for (uint32_t i = 0; i < m_Pages.size(); i++)
		{
			UI::ShiftCursorX(ImGuiStyleVar_WindowPadding);
			const auto& page = m_Pages[i];

			const char* label = UI::GenerateLabelID(page.Name);
			bool selected = i == m_CurrentPage;

			// ImGui item height hack
			auto* window = ImGui::GetCurrentWindow();
			window->DC.CurrLineSize.y = 20.0f;
			window->DC.CurrLineTextBaseOffset = 3.0f;
			//---------------------------------------------

			const ImRect itemRect = { window->WorkRect.Min.x, window->DC.CursorPos.y,
									  window->WorkRect.Max.x, window->DC.CursorPos.y + window->DC.CurrLineSize.y };

			const bool isItemClicked = [&itemRect, &label]
			{
				if (ImGui::ItemHoverable(itemRect, ImGui::GetID(label)))
				{
					return ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Left);
				}
				return false;
			}();

			const bool isWindowFocused = ImGui::IsWindowFocused();


			auto fillWithColour = [&](const ImColor& colour)
			{
				const ImU32 bgColour = ImGui::ColorConvertFloat4ToU32(colour);
				ImGui::GetWindowDrawList()->AddRectFilled(itemRect.Min, itemRect.Max, bgColour);
			};

			ImGuiTreeNodeFlags flags = (selected ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_SpanFullWidth;

			// Fill background
			//----------------
			if (selected || isItemClicked)
			{
				if (isWindowFocused)
					fillWithColour(Colors::Theme::selection);
				else
				{
					const ImColor col = UI::ColourWithMultipliedValue(Colors::Theme::selection, 0.8f);
					fillWithColour(UI::ColourWithMultipliedSaturation(col, 0.7f));
				}

				ImGui::PushStyleColor(ImGuiCol_Text, Colors::Theme::backgroundDark);
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Colors::Theme::groupHeader);
			}

			if (ImGui::Selectable(label, &selected))
				m_CurrentPage = i;

			ImGui::PopStyleColor();

			UI::ShiftCursorY(3.0f);
		}
		
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();

		// Draw side shadow
		ImRect windowRect = UI::RectExpanded(ImGui::GetCurrentWindow()->Rect(), 0.0f, 10.0f);
		ImGui::PushClipRect(windowRect.Min, windowRect.Max, false);
		UI::DrawShadowInner(EditorResources::ShadowTexture, 20, windowRect, 1.0f, windowRect.GetHeight() / 4.0f, false, true, false, false);
		ImGui::PopClipRect();

		ImGui::EndChild();
	}

	void ApplicationSettingsPanel::DrawRendererPage()
	{
		auto& editorSettings = EditorApplicationSettings::Get();
		bool saveSettings = false;

		UI::BeginPropertyGrid();
		{
			ImGui::Text("Nothing here for now...");
		}
		UI::EndPropertyGrid();

		if (saveSettings)
			EditorApplicationSettingsSerializer::SaveSettings();
	}

	void ApplicationSettingsPanel::DrawScriptingPage()
	{
		auto& editorSettings = EditorApplicationSettings::Get();
		bool saveSettings = false;

		UI::BeginPropertyGrid();
		{
			saveSettings |= UI::Property("Show Hidden Fields", editorSettings.ShowHiddenFields, "Determines if Script Components will display \"hidden\" fields, e.g fields that have been deleted, or are non-public.");

			saveSettings |= UI::Property("Debugger Listening Port", editorSettings.ScriptDebuggerListenPort, 1024, 49151, "The port that the Mono Debugger will listen on. Requires restart if changed. Only used with the Engine Tools extension for Visual Studio. (Default: 2550)"); // Anything between 1024 and 49151 should be User ports and should be safe (could still clash)
		}
		UI::EndPropertyGrid();

		if (saveSettings)
			EditorApplicationSettingsSerializer::SaveSettings();
	}


	void ApplicationSettingsPanel::DrawEditorPage()
	{
		auto& editorSettings = EditorApplicationSettings::Get();
		bool saveSettings = false;

		UI::BeginPropertyGrid();
		{
			saveSettings |= UI::Property("Advanced Mode", editorSettings.AdvancedMode, "Shows hidden options in Editor which can be used to fix things.");

			saveSettings |= UI::Property("Highlight Unset Meshes", editorSettings.HighlightUnsetMeshes, "Highlights Entities a yellow color that have a Mesh or a Static Mesh component but don't have a value set for those Entities.");
			
			
		}
		UI::EndPropertyGrid();

		if (UI::PropertyGridHeader("Gizmo Snap Values"))
		{
			UI::BeginPropertyGrid();
			{
				saveSettings |= UI::Property("Translation Snap Value", editorSettings.TranslationSnapValue, 0.1f, 0.0f, 1000.0f, "The translation snap value of the Gizmo when holding control and using the gizmos.");
				saveSettings |= UI::Property("Rotation Snap Value", editorSettings.RotationSnapValue, 0.1f, 0.0f, 1000.0f, "The rotation snap value of the Gizmo when holding control and using the gizmos.");
				saveSettings |= UI::Property("Scale Snap Value", editorSettings.ScaleSnapValue, 0.1f, 0.0f, 1000.0f, "The scale snap value of the Gizmo when holding control and using the gizmos.");
				UI::EndPropertyGrid();
			}
			ImGui::TreePop();
		}


		if (saveSettings)
			EditorApplicationSettingsSerializer::SaveSettings();
	}

}

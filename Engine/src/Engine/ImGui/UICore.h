#pragma once

#include "Engine/Renderer/Texture.h"
#include "Engine/ImGui/Colors.h"
#include "Engine/ImGui/ImGuiUtilities.h"

#include <imgui/imgui.h>

namespace Hazel::UI {

	const char* GenerateID();
	const char* GenerateLabelID(std::string_view label);
	void PushID();
	void PopID();

	bool IsInputEnabled();
	void SetInputEnabled(bool enabled);

	void ShiftCursorX(float distance);
	void ShiftCursorY(float distance);
	void ShiftCursor(float x, float y);

	void BeginPropertyGrid(uint32_t columns = 2);
	void EndPropertyGrid();

	bool BeginTreeNode(const char* name, bool defaultOpen = true);
	void EndTreeNode();

	bool ColoredButton(const char* label, const ImVec4& backgroundColor, ImVec2 buttonSize = { 16.0f, 16.0f });
	bool ColoredButton(const char* label, const ImVec4& backgroundColor, const ImVec4& foregroundColor, ImVec2 buttonSize = { 16.0f, 16.0f });


	template<typename T>
	static void Table(const char* tableName, const char** columns, uint32_t columnCount, const ImVec2& size, T callback)
	{
		if (size.x <= 0.0f || size.y <= 0.0f)
			return;

		float edgeOffset = 4.0f;

		ScopedStyle cellPadding(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 0.0f));
		ImColor backgroundColor = ImColor(Colors::Theme::background);
		const ImColor colRowAlt = ColourWithMultipliedValue(backgroundColor, 1.2f);
		ScopedColour rowColor(ImGuiCol_TableRowBg, backgroundColor);
		ScopedColour rowAltColor(ImGuiCol_TableRowBgAlt, colRowAlt);
		ScopedColour tableColor(ImGuiCol_ChildBg, backgroundColor);

		ImGuiTableFlags flags = ImGuiTableFlags_NoPadInnerX
			| ImGuiTableFlags_Resizable
			| ImGuiTableFlags_Reorderable
			| ImGuiTableFlags_ScrollY
			| ImGuiTableFlags_RowBg;

		if (!ImGui::BeginTable(tableName, columnCount, flags, size))
			return;

		const float cursorX = ImGui::GetCursorScreenPos().x;

		for (uint32_t i = 0; i < columnCount; i++)
			ImGui::TableSetupColumn(columns[i]);

		// Headers
		{
			const ImColor activeColor = ColourWithMultipliedValue(backgroundColor, 1.3f);
			ScopedColourStack headerCol(ImGuiCol_HeaderHovered, activeColor, ImGuiCol_HeaderActive, activeColor);

			ImGui::TableSetupScrollFreeze(ImGui::TableGetColumnCount(), 1);
			ImGui::TableNextRow(ImGuiTableRowFlags_Headers, 22.0f);

			for (uint32_t i = 0; i < columnCount; i++)
			{
				ImGui::TableSetColumnIndex(i);
				const char* columnName = ImGui::TableGetColumnName(i);
				ImGui::PushID(columnName);
				ShiftCursor(edgeOffset * 3.0f, edgeOffset * 2.0f);
				ImGui::TableHeader(columnName);
				ShiftCursor(-edgeOffset * 3.0f, -edgeOffset * 2.0f);
				ImGui::PopID();
			}
			ImGui::SetCursorScreenPos(ImVec2(cursorX, ImGui::GetCursorScreenPos().y));
			Draw::Underline(true, 0.0f, 5.0f);
		}

		callback();
		ImGui::EndTable();
	}

	bool TableRowClickable(const char* id, float rowHeight);
	void Separator(ImVec2 size, ImVec4 color);

	bool IsWindowFocused(const char* windowName, const bool checkRootWindow = true);
	void HelpMarker(const char* desc);

	//=========================================================================================
	/// Images / Textures (Requires e.g Vulkan Implementation)

	ImTextureID GetTextureID(Ref<Texture2D> texture);

	void Image(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	void Image(const Ref<Image2D>& image, uint32_t layer, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	void ImageMip(const Ref<Image2D>& image, uint32_t mip, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	void Image(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	bool ImageButton(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
	bool ImageButton(const char* stringID, const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
	bool ImageButton(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
	bool ImageButton(const char* stringID, const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));

	bool ImageButton(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec4& tint);

}

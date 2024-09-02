#pragma once

#include "Hazel/Core/Ref.h"

#include <imgui/imgui.h>

namespace Hazel {
	class Texture2D;
}

namespace ImGui {
	bool TreeNodeWithIcon(Hazel::Ref<Hazel::Texture2D> icon, ImGuiID id, ImGuiTreeNodeFlags flags, const char* label, const char* label_end, ImColor iconTint = IM_COL32_WHITE);
	bool TreeNodeWithIcon(Hazel::Ref<Hazel::Texture2D> icon, const void* ptr_id, ImGuiTreeNodeFlags flags, ImColor iconTint, const char* fmt, ...);
	bool TreeNodeWithIcon(Hazel::Ref<Hazel::Texture2D> icon, const char* label, ImGuiTreeNodeFlags flags, ImColor iconTint = IM_COL32_WHITE);

} // namespace UI

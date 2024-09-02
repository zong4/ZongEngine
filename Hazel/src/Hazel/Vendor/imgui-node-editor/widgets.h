#pragma once
#include <imgui.h>
//#include "drawing.h"

namespace ax {
namespace Widgets {

//using Drawing::IconType;

#if 0
void Icon(const ImVec2& size, IconType type, bool filled, const ImVec4& color = ImVec4(1, 1, 1, 1), const ImVec4& innerColor = ImVec4(0, 0, 0, 0));
#endif
void ImageIcon(const ImVec2& size, ImTextureID image, bool filled, float iconSpacing, const ImVec4& color = ImVec4(1, 1, 1, 1), const ImVec4& innerColor = ImVec4(0, 0, 0, 0));

void IconGrid(const ImVec2& size, bool filled, const ImVec4& color = ImVec4(1, 1, 1, 1), const ImVec4& innerColor = ImVec4(0, 0, 0, 0));

} // namespace Widgets
} // namespace ax
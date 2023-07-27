#pragma once

#include "platform/pch.hpp"

namespace zong
{
namespace platform
{

class Input
{
public:
    static bool isKeyPressed(core::KeyCode key);

    static bool      isMouseButtonPressed(core::MouseCode button);
    static glm::vec2 mousePosition();
    static float     mouseX();
    static float     mouseY();
};

} // namespace platform
} // namespace zong
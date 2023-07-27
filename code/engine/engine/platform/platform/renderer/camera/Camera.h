#pragma once

#include "platform/pch.hpp"

namespace zong
{
namespace platform
{

class Camera
{
protected:
    glm::mat4 _projection;

public:
    Camera() : _projection(glm::mat4(1.0f)) {}
    Camera(glm::mat4 const& projection) : _projection(projection) {}
    virtual ~Camera() = default;
};

} // namespace platform
} // namespace zong
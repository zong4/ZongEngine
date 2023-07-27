#pragma once

#include "Camera.h"

namespace zong
{
namespace platform
{

class OrthographicCamera
{
private:
    glm::mat4 _projectionMatrix;
    glm::mat4 _viewMatrix;
    glm::mat4 _viewProjectionMatrix;

    glm::vec3 _position = {0.0f, 0.0f, 0.0f};
    float     _rotation = 0.0f;

public:
    inline const glm::vec3& position() const { return _position; }
    inline float            rotation() const { return _rotation; }
    inline const glm::mat4& projectionMatrix() const { return _projectionMatrix; }
    inline const glm::mat4& viewMatrix() const { return _viewMatrix; }
    inline const glm::mat4& viewProjectionMatrix() const { return _viewProjectionMatrix; }

    void setPosition(glm::vec3 const& position);
    void setRotation(float rotation);
    void setProjection(float left, float right, float bottom, float top);

public:
    OrthographicCamera(float left, float right, float bottom, float top);

private:
    void recalculateViewMatrix();
};

} // namespace platform
} // namespace zong

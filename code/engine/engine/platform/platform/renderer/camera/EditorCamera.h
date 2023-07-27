#pragma once

#include "Camera.h"

namespace zong
{
namespace platform
{

class EditorCamera : public Camera
{
private:
    float _fov = 45.0f, _aspectRatio = 1.778f, _nearClip = 0.1f, _farClip = 1000.0f;

    glm::mat4 _viewMatrix = glm::mat4();
    glm::vec3 _position   = {0.0f, 0.0f, 0.0f};
    glm::vec3 _focalPoint = {0.0f, 0.0f, 0.0f};

    glm::vec2 _initialMousePosition = {0.0f, 0.0f};

    float _distance = 10.0f;
    float _pitch = 0.0f, _yaw = 0.0f;

    float _viewportWidth = 1280, _viewportHeight = 720;

public:
    inline float distance() const { return _distance; }

    inline const glm::mat4& viewMatrix() const { return _viewMatrix; }
    inline glm::mat4        viewProjection() const { return _projection * _viewMatrix; }

    glm::vec3               upDirection() const;
    glm::vec3               rightDirection() const;
    glm::vec3               forwardDirection() const;
    inline const glm::vec3& position() const { return _position; }
    glm::quat               orientation() const;

    inline float pitch() const { return _pitch; }
    inline float yaw() const { return _yaw; }

    inline void setDistance(float distance) { _distance = distance; }

    void setViewportSize(float width, float height);

public:
    EditorCamera() : Camera() {}
    EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

    void onUpdate(core::Timestep const& ts);
    void onEvent(core::Event& e);

private:
    void updateProjection();
    void updateView();

    inline glm::vec3 calculatePosition() const { return _focalPoint - forwardDirection() * _distance; }

    std::pair<float, float> panSpeed() const;
    inline static float     rotationSpeed() { return 0.8f; }
    float                   zoomSpeed() const;

    bool onMouseScroll(core::MouseScrolledEvent& e);

    void mousePan(glm::vec2 const& delta);
    void mouseRotate(glm::vec2 const& delta);
    void mouseZoom(float delta);
};

} // namespace platform
} // namespace zong

#pragma once

#include "OrthographicCamera.h"

namespace zong
{
namespace platform
{

class OrthographicCameraController
{
private:
    float              _aspectRatio;
    float              _zoomLevel = 1.0f;
    OrthographicCamera _camera;

    bool _rotation;

    glm::vec3 _cameraPosition         = {0.0f, 0.0f, 0.0f};
    float     _cameraRotation         = 0.0f; // In degrees, in the anti-clockwise direction
    float     _cameraTranslationSpeed = 5.0f, _cameraRotationSpeed = 180.0f;

public:
    OrthographicCamera&       camera() { return _camera; }
    const OrthographicCamera& camera() const { return _camera; }

    inline float zoomLevel() const { return _zoomLevel; }

    inline void setZoomLevel(float level) { _zoomLevel = level; }

public:
    OrthographicCameraController(float aspectRatio, bool rotation = false);

    void onUpdate(core::Timestep const& ts);
    void onEvent(core::Event& e);

    void onResize(float width, float height);

private:
    bool onMouseScrolled(core::MouseScrolledEvent& e);
    bool onWindowResized(core::WindowResizeEvent& e);
};

} // namespace platform
} // namespace zong
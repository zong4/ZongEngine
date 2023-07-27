#include "EditorCamera.h"

#include "../../input/Input.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

glm::vec3 zong::platform::EditorCamera::upDirection() const
{
    return glm::rotate(orientation(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 zong::platform::EditorCamera::rightDirection() const
{
    return glm::rotate(orientation(), glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 zong::platform::EditorCamera::forwardDirection() const
{
    return glm::rotate(orientation(), glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::quat zong::platform::EditorCamera::orientation() const
{
    return glm::quat(glm::vec3(-pitch(), -yaw(), 0.0f));
}

void zong::platform::EditorCamera::setViewportSize(float width, float height)
{
    _viewportWidth  = width;
    _viewportHeight = height;
    updateProjection();
}

zong::platform::EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
    : Camera(glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip)),
      _fov(fov),
      _aspectRatio(aspectRatio),
      _nearClip(nearClip),
      _farClip(farClip)
{
    updateView();
}

void zong::platform::EditorCamera::onUpdate(core::Timestep const& ts)
{
    if (Input::isKeyPressed(core::KeyCode::LeftAlt))
    {
        glm::vec2 const& mouse{Input::mouseX(), Input::mouseY()};
        glm::vec2 const  delta = (mouse - _initialMousePosition) * 0.003f;
        _initialMousePosition  = mouse;

        if (Input::isMouseButtonPressed(core::MouseCode::ButtonMiddle))
            mousePan(delta);
        else if (Input::isMouseButtonPressed(core::MouseCode::ButtonLeft))
            mouseRotate(delta);
        else if (Input::isMouseButtonPressed(core::MouseCode::ButtonRight))
            mouseZoom(delta.y);
    }

    updateView();
}

void zong::platform::EditorCamera::onEvent(core::Event& e)
{
    core::EventDispatcher dispatcher(e);
    dispatcher.dispatch<core::MouseScrolledEvent>(ZONG_BIND_EVENT_FN(EditorCamera::onMouseScroll));
}

void zong::platform::EditorCamera::updateProjection()
{
    _aspectRatio = _viewportWidth / _viewportHeight;
    _projection  = glm::perspective(glm::radians(_fov), _aspectRatio, _nearClip, _farClip);
}

void zong::platform::EditorCamera::updateView()
{
    // m_Yaw = m_Pitch = 0.0f; // Lock the camera's rotation
    _position = calculatePosition();

    glm::quat orientation = this->orientation();
    _viewMatrix           = glm::translate(glm::mat4(1.0f), _position) * glm::toMat4(orientation);
    _viewMatrix           = glm::inverse(_viewMatrix);
}

std::pair<float, float> zong::platform::EditorCamera::panSpeed() const
{
    float x       = std::min(_viewportWidth / 1000.0f, 2.4f); // max = 2.4f
    float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

    float y       = std::min(_viewportHeight / 1000.0f, 2.4f); // max = 2.4f
    float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

    return {xFactor, yFactor};
}

float zong::platform::EditorCamera::zoomSpeed() const
{
    float distance = _distance * 0.2f;
    distance       = std::max(distance, 0.0f);
    float speed    = distance * distance;
    speed          = std::min(speed, 100.0f); // max speed = 100
    return speed;
}

bool zong::platform::EditorCamera::onMouseScroll(core::MouseScrolledEvent& e)
{
    float const delta = e.getYOffset() * 0.1f;
    mouseZoom(delta);
    updateView();
    return false;
}

void zong::platform::EditorCamera::mousePan(glm::vec2 const& delta)
{
    auto [xSpeed, ySpeed] = panSpeed();
    _focalPoint += -rightDirection() * delta.x * xSpeed * _distance;
    _focalPoint += upDirection() * delta.y * ySpeed * _distance;
}

void zong::platform::EditorCamera::mouseRotate(glm::vec2 const& delta)
{
    float const yawSign = upDirection().y < 0 ? -1.0f : 1.0f;
    _yaw += yawSign * delta.x * rotationSpeed();
    _pitch += delta.y * rotationSpeed();
}

void zong::platform::EditorCamera::mouseZoom(float delta)
{
    _distance -= delta * zoomSpeed();
    if (_distance < 1.0f)
    {
        _focalPoint += forwardDirection();
        _distance = 1.0f;
    }
}

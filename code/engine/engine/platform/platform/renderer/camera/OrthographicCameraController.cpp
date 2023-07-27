#include "OrthographicCameraController.h"

#include "../../input/Input.h"

zong::platform::OrthographicCameraController::OrthographicCameraController(float aspectRatio, bool rotation)
    : _aspectRatio(aspectRatio),
      _camera(-_aspectRatio * _zoomLevel, _aspectRatio * _zoomLevel, -_zoomLevel, _zoomLevel),
      _rotation(rotation)
{
}

void zong::platform::OrthographicCameraController::onUpdate(core::Timestep const& ts)
{
    ZONG_PROFILE_FUNCTION();

    if (Input::isKeyPressed(core::KeyCode::A))
    {
        _cameraPosition.x -= cos(glm::radians(_cameraRotation)) * _cameraTranslationSpeed * ts;
        _cameraPosition.y -= sin(glm::radians(_cameraRotation)) * _cameraTranslationSpeed * ts;
    }
    else if (Input::isKeyPressed(core::KeyCode::D))
    {
        _cameraPosition.x += cos(glm::radians(_cameraRotation)) * _cameraTranslationSpeed * ts;
        _cameraPosition.y += sin(glm::radians(_cameraRotation)) * _cameraTranslationSpeed * ts;
    }

    if (Input::isKeyPressed(core::KeyCode::W))
    {
        _cameraPosition.x += -sin(glm::radians(_cameraRotation)) * _cameraTranslationSpeed * ts;
        _cameraPosition.y += cos(glm::radians(_cameraRotation)) * _cameraTranslationSpeed * ts;
    }
    else if (Input::isKeyPressed(core::KeyCode::S))
    {
        _cameraPosition.x -= -sin(glm::radians(_cameraRotation)) * _cameraTranslationSpeed * ts;
        _cameraPosition.y -= cos(glm::radians(_cameraRotation)) * _cameraTranslationSpeed * ts;
    }

    if (_rotation)
    {
        if (Input::isKeyPressed(core::KeyCode::Q))
            _cameraRotation += _cameraRotationSpeed * ts;
        if (Input::isKeyPressed(core::KeyCode::E))
            _cameraRotation -= _cameraRotationSpeed * ts;

        if (_cameraRotation > 180.0f)
            _cameraRotation -= 360.0f;
        else if (_cameraRotation <= -180.0f)
            _cameraRotation += 360.0f;

        _camera.setRotation(_cameraRotation);
    }

    _camera.setPosition(_cameraPosition);

    _cameraTranslationSpeed = _zoomLevel;
}

void zong::platform::OrthographicCameraController::onEvent(core::Event& e)
{
    ZONG_PROFILE_FUNCTION();

    core::EventDispatcher dispatcher(e);
    dispatcher.dispatch<core::MouseScrolledEvent>(ZONG_BIND_EVENT_FN(OrthographicCameraController::onMouseScrolled));
    dispatcher.dispatch<core::WindowResizeEvent>(ZONG_BIND_EVENT_FN(OrthographicCameraController::onWindowResized));
}

void zong::platform::OrthographicCameraController::onResize(float width, float height)
{
    ZONG_PROFILE_FUNCTION();

    _aspectRatio = width / height;
    _camera.setProjection(-_aspectRatio * _zoomLevel, _aspectRatio * _zoomLevel, -_zoomLevel, _zoomLevel);
}

bool zong::platform::OrthographicCameraController::onMouseScrolled(core::MouseScrolledEvent& e)
{
    ZONG_PROFILE_FUNCTION();

    _zoomLevel -= e.getYOffset() * 0.25f;
    _zoomLevel = std::max(_zoomLevel, 0.25f);
    _camera.setProjection(-_aspectRatio * _zoomLevel, _aspectRatio * _zoomLevel, -_zoomLevel, _zoomLevel);
    return false;
}

bool zong::platform::OrthographicCameraController::onWindowResized(core::WindowResizeEvent& e)
{
    ZONG_PROFILE_FUNCTION();

    onResize(static_cast<float>(e.getWidth()), static_cast<float>(e.getHeight()));
    return false;
}

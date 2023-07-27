#include "OrthographicCamera.h"

#include <glm/gtc/matrix_transform.hpp>

void zong::platform::OrthographicCamera::setPosition(glm::vec3 const& position)
{
    _position = position;
    recalculateViewMatrix();
}

void zong::platform::OrthographicCamera::setRotation(float rotation)
{
    _rotation = rotation;
    recalculateViewMatrix();
}

zong::platform::OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
    : _projectionMatrix(glm::ortho(left, right, bottom, top, -1.0f, 1.0f)),
      _viewMatrix(1.0f),
      _viewProjectionMatrix(_projectionMatrix * _viewMatrix)
{
}

void zong::platform::OrthographicCamera::setProjection(float left, float right, float bottom, float top)
{
    ZONG_PROFILE_FUNCTION();

    _projectionMatrix     = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
    _viewProjectionMatrix = _projectionMatrix * _viewMatrix;
}

void zong::platform::OrthographicCamera::recalculateViewMatrix()
{
    ZONG_PROFILE_FUNCTION();

    glm::mat4 const transform =
        glm::translate(glm::mat4(1.0f), _position) * glm::rotate(glm::mat4(1.0f), glm::radians(_rotation), glm::vec3(0, 0, 1));

    _viewMatrix           = glm::inverse(transform);
    _viewProjectionMatrix = _projectionMatrix * _viewMatrix;
}

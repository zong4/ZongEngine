#pragma once

#include "Engine/Renderer/Camera.h"

namespace Hazel {

	class SceneCamera : public Camera 
	{
	public:
		enum class ProjectionType { Perspective = 0, Orthographic = 1 };
	public:
		void SetPerspective(float verticalFOV, float nearClip = 0.1f, float farClip = 1000.0f);
		void SetOrthographic(float size, float nearClip = -1.0f, float farClip = 1.0f);
		void SetViewportSize(uint32_t width, uint32_t height);

		void SetDegPerspectiveVerticalFOV(const float degVerticalFov) { m_DegPerspectiveFOV = degVerticalFov; }
		void SetRadPerspectiveVerticalFOV(const float degVerticalFov) { m_DegPerspectiveFOV = glm::degrees(degVerticalFov); }
		float GetDegPerspectiveVerticalFOV() const { return m_DegPerspectiveFOV; }
		float GetRadPerspectiveVerticalFOV() const { return glm::radians(m_DegPerspectiveFOV); }
		void SetPerspectiveNearClip(const float nearClip) { m_PerspectiveNear = nearClip; }
		float GetPerspectiveNearClip() const { return m_PerspectiveNear; }
		void SetPerspectiveFarClip(const float farClip) { m_PerspectiveFar = farClip; }
		float GetPerspectiveFarClip() const { return m_PerspectiveFar; }

		void SetOrthographicSize(const float size) { m_OrthographicSize = size; }
		float GetOrthographicSize() const { return m_OrthographicSize; }
		void SetOrthographicNearClip(const float nearClip) { m_OrthographicNear = nearClip; }
		float GetOrthographicNearClip() const { return m_OrthographicNear; }
		void SetOrthographicFarClip(const float farClip) { m_OrthographicFar = farClip; }
		float GetOrthographicFarClip() const { return m_OrthographicFar; }

		void SetProjectionType(ProjectionType type) { m_ProjectionType = type; }
		ProjectionType GetProjectionType() const { return m_ProjectionType; }
	private:
		ProjectionType m_ProjectionType = ProjectionType::Perspective;

		float m_DegPerspectiveFOV = 45.0f;
		float m_PerspectiveNear = 0.1f, m_PerspectiveFar = 1000.0f;

		float m_OrthographicSize = 10.0f;
		float m_OrthographicNear = -1.0f, m_OrthographicFar = 1.0f;
	};

}

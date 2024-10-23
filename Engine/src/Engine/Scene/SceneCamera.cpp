#include "hzpch.h"
#include "SceneCamera.h"

namespace Hazel {


	void SceneCamera::SetPerspective(float degVerticalFOV, float nearClip, float farClip)
	{
		m_ProjectionType = ProjectionType::Perspective;
		m_DegPerspectiveFOV = degVerticalFOV;
		m_PerspectiveNear = nearClip;
		m_PerspectiveFar = farClip;
	}

	void SceneCamera::SetOrthographic(float size, float nearClip, float farClip)
	{
		m_ProjectionType = ProjectionType::Orthographic;
		m_OrthographicSize = size;
		m_OrthographicNear = nearClip;
		m_OrthographicFar = farClip;
	}

	void SceneCamera::SetViewportSize(uint32_t width, uint32_t height)
	{
		switch (m_ProjectionType)
		{
		case ProjectionType::Perspective:
			SetPerspectiveProjectionMatrix(glm::radians(m_DegPerspectiveFOV), (float)width, (float)height, m_PerspectiveNear, m_PerspectiveFar);
			break;
		case ProjectionType::Orthographic:
			float aspect = (float)width / (float)height;
			float width = m_OrthographicSize * aspect;
			float height = m_OrthographicSize;
			SetOrthoProjectionMatrix(width, height, m_OrthographicNear, m_OrthographicFar);
			break;
		}
	}

}

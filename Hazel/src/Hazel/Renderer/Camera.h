#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>

namespace Hazel
{

	class Camera
	{
	public:
		Camera() = default;
		Camera(const glm::mat4& projection, const glm::mat4& unReversedProjection);
		Camera(const float degFov, const float width, const float height, const float nearP, const float farP);
		virtual ~Camera() = default;

		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const glm::mat4& GetUnReversedProjectionMatrix() const { return m_UnReversedProjectionMatrix; }

		void SetProjectionMatrix(const glm::mat4 projection, const glm::mat4 unReversedProjection)
		{
			m_ProjectionMatrix = projection;
			m_UnReversedProjectionMatrix = unReversedProjection;
		}

		void SetPerspectiveProjectionMatrix(const float radFov, const float width, const float height, const float nearP, const float farP)
		{
			m_ProjectionMatrix = glm::perspectiveFov(radFov, width, height, farP, nearP);
			m_UnReversedProjectionMatrix = glm::perspectiveFov(radFov, width, height, nearP, farP);
		}

		void SetOrthoProjectionMatrix(const float width, const float height, const float nearP, const float farP)
		{
			//TODO(Karim): Make sure this is correct.
			m_ProjectionMatrix = glm::ortho(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, farP, nearP);
			m_UnReversedProjectionMatrix = glm::ortho(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, nearP, farP);
		}

		float GetExposure() const { return m_Exposure; }
		float& GetExposure() { return m_Exposure; }
	protected:
		float m_Exposure = 0.8f;
	private:
		glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
		//Currently only needed for shadow maps and ImGuizmo
		glm::mat4 m_UnReversedProjectionMatrix = glm::mat4(1.0f);
	};

}

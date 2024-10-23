#include "hzpch.h"
#include "DebugRenderer.h"

namespace Hazel {

	void DebugRenderer::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color)
	{
		m_RenderQueue.emplace_back([p0, p1, color](Ref<Renderer2D> renderer)
		{
			renderer->DrawLine(p0, p1, color);
		});
	}

	void DebugRenderer::DrawQuadBillboard(const glm::vec3& translation, const glm::vec2& size, const glm::vec4& color)
	{
		m_RenderQueue.emplace_back([translation, size, color](Ref<Renderer2D> renderer)
		{
			renderer->DrawQuadBillboard(translation, size, color);
		});
	}

	void DebugRenderer::SetLineWidth(float thickness)
	{
		m_RenderQueue.emplace_back([thickness](Ref<Renderer2D> renderer)
		{
			renderer->SetLineWidth(thickness);
		});
	}

}

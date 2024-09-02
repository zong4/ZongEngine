#pragma once

#include "Renderer2D.h"

#include <vector>

namespace Hazel {

	//
	// Utility class which queues rendering work for once-per-frame
	// flushing by Scene (intended to be used for debug graphics)
	//
	class DebugRenderer : public RefCounted
	{
	public:
		using RenderQueue = std::vector<std::function<void(Ref<Renderer2D>)>>;
	public:
		DebugRenderer() = default;
		~DebugRenderer() = default;

		void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color = glm::vec4(1.0f));
		void DrawQuadBillboard(const glm::vec3& translation, const glm::vec2& size, const glm::vec4& color = glm::vec4(1.0f));
		
		void SetLineWidth(float thickness);

		RenderQueue& GetRenderQueue() { return m_RenderQueue; }
		void ClearRenderQueue() { m_RenderQueue.clear(); }
	private:
		RenderQueue m_RenderQueue;
	};

}

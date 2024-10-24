#pragma once

#include "Engine/Editor/EditorPanel.h"
#include "Engine/Renderer/SceneRenderer.h"

namespace Engine {

	class SceneRendererPanel : public EditorPanel
	{
	public:
		SceneRendererPanel() = default;
		virtual ~SceneRendererPanel() = default;

		void SetContext(const Ref<SceneRenderer>& context) { m_Context = context; }
		virtual void OnImGuiRender(bool& isOpen) override;
	private:
		Ref<SceneRenderer> m_Context;
	};

}

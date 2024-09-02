#pragma once

#include "Hazel/Editor/EditorPanel.h"
#include "Hazel/Renderer/SceneRenderer.h"

namespace Hazel {

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

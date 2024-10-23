#pragma once

#include "Engine/Scene/Scene.h"
#include "SelectionManager.h"

#include "EditorPanel.h"

namespace Hazel {

	class ECSDebugPanel : public EditorPanel
	{
	public:
		ECSDebugPanel(Ref<Scene> context);
		~ECSDebugPanel();

		virtual void OnImGuiRender(bool& open) override;

		virtual void SetSceneContext(const Ref<Scene>& context) { m_Context = context; }
	private:
		Ref<Scene> m_Context;
	};

}

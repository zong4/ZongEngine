#pragma once

#include "Engine/Editor/EditorPanel.h"
#include "Engine/Physics/PhysicsScene.h"

namespace Hazel {

	class PhysicsStatsPanel : public EditorPanel
	{
	public:
		PhysicsStatsPanel();
		~PhysicsStatsPanel();

		virtual void SetSceneContext(const Ref<Scene>& context) override;
		virtual void OnImGuiRender(bool& isOpen) override;

	private:
		Ref<PhysicsScene> m_PhysicsScene;
	};

}

#pragma once

#include "Engine/Editor/EditorPanel.h"

namespace Engine {

	class PhysicsCapturesPanel : public EditorPanel
	{
	public:
		virtual void OnImGuiRender(bool& isOpen) override;

	};

}

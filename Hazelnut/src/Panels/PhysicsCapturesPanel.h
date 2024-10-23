#pragma once

#include "Engine/Editor/EditorPanel.h"

namespace Hazel {

	class PhysicsCapturesPanel : public EditorPanel
	{
	public:
		virtual void OnImGuiRender(bool& isOpen) override;

	};

}

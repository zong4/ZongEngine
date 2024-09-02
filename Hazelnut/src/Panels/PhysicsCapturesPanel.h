#pragma once

#include "Hazel/Editor/EditorPanel.h"

namespace Hazel {

	class PhysicsCapturesPanel : public EditorPanel
	{
	public:
		virtual void OnImGuiRender(bool& isOpen) override;

	};

}

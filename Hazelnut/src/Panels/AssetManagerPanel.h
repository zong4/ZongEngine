#pragma once

#include "Engine/Editor/EditorPanel.h"

namespace Hazel {

	class AssetManagerPanel : public EditorPanel
	{
	public:
		AssetManagerPanel() = default;
		virtual ~AssetManagerPanel() = default;

		virtual void OnImGuiRender(bool& isOpen) override;
	};

}

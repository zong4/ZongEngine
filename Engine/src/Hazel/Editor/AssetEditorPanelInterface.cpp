#include "hzpch.h"
#include "AssetEditorPanelInterface.h"
#include "AssetEditorPanel.h"

namespace Hazel {

	// NOTE(Peter): Ugly workaround since AssetEditorPanel includes ImGui.h (meaning ImGui.h can't include AssetEditorPanel).
	//				Will rework those includes at a later date...
	void AssetEditorPanelInterface::OpenEditor(const Ref<Asset>& asset)
	{
		AssetEditorPanel::OpenEditor(asset);
	}

}


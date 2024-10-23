#pragma once

namespace Hazel {

	struct EditorApplicationSettings
	{
		//---------- Scripting ------------
		bool ShowHiddenFields = false;
		int ScriptDebuggerListenPort = 2550;

		//---------- Content Browser ------------
		bool ContentBrowserShowAssetTypes = true;
		int ContentBrowserThumbnailSize = 128;
		
		//---------- Editor ------------
		bool AdvancedMode = false;
		bool HighlightUnsetMeshes = true;
		float TranslationSnapValue = 0.5f;
		float RotationSnapValue = 45.0f;
		float ScaleSnapValue = 0.5f;

		static EditorApplicationSettings& Get();
	};

	class EditorApplicationSettingsSerializer
	{
	public:
		static void Init();

		static void LoadSettings();
		static void SaveSettings();
	};

}

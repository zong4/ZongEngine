#pragma once

#include "Hazel/Core/Log.h"
#include "Hazel/Project/Project.h"
#include "Hazel/Scene/Scene.h"

#include "Hazel/Editor/EditorPanel.h"

namespace Hazel {

	class ProjectSettingsWindow : public EditorPanel
	{
	public:
		ProjectSettingsWindow();
		~ProjectSettingsWindow();

		virtual void OnImGuiRender(bool& isOpen) override;
		virtual void OnProjectChanged(const Ref<Project>& project) override;

	private:
		void RenderGeneralSettings();
		void RenderRendererSettings();
		void RenderAudioSettings();
		void RenderScriptingSettings();
		void RenderPhysicsSettings();
		void RenderLogSettings();
	private:
		Ref<Project> m_Project;
		AssetHandle m_DefaultScene;
		int32_t m_SelectedLayer = -1;
		char m_NewLayerNameBuffer[255];

	};

}

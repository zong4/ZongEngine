#pragma once

#include "AssetEditorPanel.h"
#include "Engine/Asset/MeshColliderAsset.h"
#include "Engine/Renderer/SceneRenderer.h"
#include "Engine/Physics/PhysicsTypes.h"

namespace Engine {

	class MeshColliderEditor : public AssetEditor
	{
	private:
		struct MeshColliderTabData
		{
			std::string Name;
			Ref<MeshColliderAsset> ColliderAsset = nullptr;

			Ref<Scene> ViewportScene;
			Ref<SceneRenderer> ViewportRenderer;
			Entity ColliderEntity;
			Entity SkyLight;
			Entity DirectionalLight;
			EditorCamera ViewportCamera{ 45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f };

			bool ViewportFocused = false;
			bool ViewportHovered = false;

			// Docking Stuff
			bool ResetDockspace = true;
			ImGuiID DockSpaceID = 0;
			ImGuiID SettingsDockID = 0;
			ImGuiID ViewportDockID = 0;

			std::string ViewportPanelName;
			std::string SettingsPanelName;

			bool IsOpen = true;
			bool JustCreated = true;
			bool NeedsSaving = false;
			bool NeedsCooking = false;
		};

	public:
		MeshColliderEditor();
		~MeshColliderEditor();

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnEvent(Event& e) override;
		virtual void SetSceneContext(const Ref<Scene>& context) override
		{
			if (!context)
			{
				m_EditorScene = nullptr;
				return;
			}

			if (context->IsEditorScene())
				m_EditorScene = context;
		}

		virtual void OnImGuiRender() override;

		virtual void SetAsset(const Ref<Asset>& asset) override;

		virtual void OnOpen() override {}
		virtual void Render() override {}
		virtual void OnClose() override {}

	private:
		void RenderTab(const std::shared_ptr<MeshColliderTabData>& tabData);
		void RenderViewportPanel(const std::shared_ptr<MeshColliderTabData>& tabData);
		void RenderSettingsPanel(const std::shared_ptr<MeshColliderTabData>& tabData);

		void UpdatePreviewEntity(const std::shared_ptr<MeshColliderTabData>& tabData);
		bool CookMeshCollider(const std::shared_ptr<MeshColliderTabData>& tabData);
		void RenderCookingOutput();
		void DestroyTab(const std::shared_ptr<MeshColliderTabData>& tabData);

		void Save(const std::shared_ptr<MeshColliderTabData>& tabData);

	private:
		std::unordered_map<AssetHandle, std::shared_ptr<MeshColliderTabData>> m_OpenTabs;
		AssetHandle m_ClosedTab = AssetHandle(0);

		std::shared_ptr<MeshColliderTabData> m_SelectedTabData;
		bool m_WindowOpen = false;
		bool m_RebuildDockspace = true;
		ImGuiID m_DockSpaceID = 0;
		std::string m_TabToFocus = "";

		bool m_ShowCookingResults = true;
		bool m_IsCookingResultsOpen = false;
		std::shared_ptr<MeshColliderTabData> m_LastCookedTabData;
		ECookingResult m_LastSimpleCookingResult = ECookingResult::None;
		ECookingResult m_LastComplexCookingResult = ECookingResult::None;

		bool m_Maximized = false;
		ImVec2 m_OldPosition, m_OldSize;

		Ref<Scene> m_EditorScene;
	};

}

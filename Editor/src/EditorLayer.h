#pragma once

#include "Engine.h"

#include "Engine/ImGui/ImGuiLayer.h"
#include "Engine/Editor/EditorCamera.h"
#include "imgui/imgui_internal.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

#include "Engine/Editor/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/ProjectSettingsWindow.h"
#include "Panels/ApplicationSettingsPanel.h"
#include "Engine/Editor/EditorConsolePanel.h"
#include "Engine/Editor/ECSDebugPanel.h"

#include "Engine/Editor/PanelManager.h"

#include "Engine/Project/UserPreferences.h"

#include "Engine/Renderer/UI/Font.h"

#include <future>

namespace Engine {

	class EditorLayer : public Layer
	{
	public:
		EditorLayer(const Ref<UserPreferences>& userPreferences);
		virtual ~EditorLayer() override;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(Timestep ts) override;

		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& e) override;

		void OnRender2D();

		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void OpenProject();
		void OpenProject(const std::filesystem::path& filepath);
		void CreateProject(std::filesystem::path projectPath);
		void EmptyProject();
		void UpdateCurrentProject();
		void SaveProject();
		void CloseProject(bool unloadProject = true);
		void NewScene(const std::string& name = "UntitledScene");
		bool OpenScene();
		bool OpenScene(const std::filesystem::path& filepath, const bool checkAutoSave);
		bool OpenScene(const AssetMetadata& assetMetadata);
		void SaveScene();
		void SaveSceneAuto();
		void SaveSceneAs();

		void OnCreateMeshFromMeshSource(Entity entity, Ref<MeshSource> meshSource);
		void SceneHierarchyInvalidMetadataCallback(Entity entity, AssetHandle handle);
		void SceneHierarchySetEditorCameraTransform(Entity entity);

	private:
		std::pair<float, float> GetMouseViewportSpace(bool primaryViewport);
		std::pair<glm::vec3, glm::vec3> CastRay(const EditorCamera& camera, float mx, float my);

		std::vector<std::function<void()>> m_PostSceneUpdateQueue;

		struct SelectionData
		{
			Engine::Entity Entity;
			Submesh* Mesh = nullptr;
			float Distance = 0.0f;
		};
		void OnEntityDeleted(Entity e);

		void OnScenePlay();
		void OnSceneStop();
		void OnSceneStartSimulation();
		void OnSceneStopSimulation();
		void OnSceneTransition(AssetHandle scene);

		void UpdateWindowTitle(const std::string& sceneName);
		
		void UI_DrawMenubar();
		// Returns titlebar height
		float UI_DrawTitlebar();
		void UI_HandleManualWindowResize();
		bool UI_TitleBarHitTest(int x, int y) const;

		void UI_GizmosToolbar();
		void UI_CentralToolbar();
		void UI_ViewportSettings();
		void UI_DrawGizmos();
		void UI_HandleAssetDrop();

		// Popups
		void UI_ShowNewProjectPopup();
		void UI_ShowLoadAutoSavePopup();
		void UI_ShowCreateNewMeshPopup();
		void UI_ShowInvalidAssetMetadataPopup();
		void UI_ShowNoMeshPopup();
		void UI_ShowNoSkeletonPopup();
		void UI_ShowNoAnimationPopup();
		void UI_ShowNewScenePopup();
		void UI_ShowWelcomePopup();
		void UI_ShowAboutPopup();
		
		void UI_BuildAssetPackDialog();

		// Statistics Panel Rendering
		void UI_StatisticsPanel();

		float GetSnapValue();

		void DeleteEntity(Entity entity);

		void UpdateSceneRendererSettings();
		void QueueSceneTransition(AssetHandle scene);

		void BuildShaderPack();
		void BuildAssetPack();
		void RegenerateProjectScriptSolution(const std::filesystem::path& projectPath);
	private:
		Ref<UserPreferences> m_UserPreferences;

		Scope<PanelManager> m_PanelManager;
		bool m_ShowStatisticsPanel = false;

		Ref<Scene> m_RuntimeScene, m_EditorScene, m_SimulationScene, m_CurrentScene;
		Ref<SceneRenderer> m_ViewportRenderer;
		Ref<SceneRenderer> m_SecondViewportRenderer;
		Ref<SceneRenderer> m_FocusedRenderer;
		Ref<Renderer2D> m_Renderer2D;
		std::string m_SceneFilePath;

		EditorCamera m_EditorCamera;
		EditorCamera m_SecondEditorCamera;

		float m_LineWidth = 2.0f;

		bool m_TitleBarHovered = false;

		glm::vec2 m_ViewportBounds[2];
		glm::vec2 m_SecondViewportBounds[2];
		int m_GizmoType = -1; // -1 = no gizmo
		int m_GizmoMode = 0; // 0 = local

		float m_SnapValue = 0.5f;
		float m_RotationSnapValue = 45.0f;
		bool m_DrawOnTopBoundingBoxes = true;

		// ImGui Tools
		bool m_ShowMetricsTool = false;
		bool m_ShowStackTool = false;
		bool m_ShowStyleEditor = false;

		bool m_StartedCameraClickInViewport = false;

		bool m_ShowBoundingBoxes = false;
		bool m_ShowBoundingBoxSelectedMeshOnly = true;
		bool m_ShowBoundingBoxSubmeshes = false;

		bool m_ShowIcons = true;
		bool m_ShowGizmos = true;
		bool m_ShowGizmosInPlayMode = false;

		bool m_ViewportPanelMouseOver = false;
		bool m_ViewportPanelFocused = false;
		bool m_AllowViewportCameraEvents = false;

		bool m_ViewportPanel2MouseOver = false;
		bool m_ViewportPanel2Focused = false;

		bool m_ShowSecondViewport = false;

		bool m_EditorCameraInRuntime = false;

		enum class TransformationTarget { MedianPoint, IndividualOrigins };
		TransformationTarget m_MultiTransformTarget = TransformationTarget::MedianPoint;

		struct LoadAutoSavePopupData
		{
			std::string FilePath;
			std::string FilePathAuto;
		} m_LoadAutoSavePopupData;

		struct CreateNewMeshPopupData
		{
			Ref<MeshSource> MeshToCreate;
			std::string CreateMeshFilenameBuffer;
			std::string CreateSkeletonFilenameBuffer;
			std::vector<std::string> CreateAnimationFilenameBuffer;
			std::string CreateGraphFilenameBuffer;
			Entity TargetEntity;

			CreateNewMeshPopupData()
			{
				CreateMeshFilenameBuffer = "";
				CreateSkeletonFilenameBuffer = "";
				CreateGraphFilenameBuffer = "";
				MeshToCreate = nullptr;
				TargetEntity = {};
			}

		} m_CreateNewMeshPopupData;

		struct InvalidAssetMetadataPopupData
		{
			AssetMetadata Metadata;
		} m_InvalidAssetMetadataPopupData;

		enum class SceneState
		{
			Edit = 0, Play = 1, Pause = 2, Simulate = 3
		};
		SceneState m_SceneState = SceneState::Edit;

		float m_TimeSinceLastSave = 0.0f; // time (in seconds) since scene was last saved.  Counts up only when scene is in Edit mode. If exceeds 300s then scene is automatically saved

		enum class SelectionMode
		{
			Entity = 0, SubMesh = 1
		};

		SelectionMode m_SelectionMode = SelectionMode::Entity;

		float m_RequiredProjectVersion = 0.0f;
		bool m_ProjectUpdateNeeded = false;
		bool m_ShowProjectUpdatedPopup = false;

		std::thread m_AssetPackThread;
		std::future<Ref<AssetPack>> m_AssetPackFuture;
		std::atomic<bool> m_AssetPackBuildInProgress = false;
		std::atomic<float> m_AssetPackBuildProgress = 0.0f;
		std::string m_AssetPackBuildMessage;
		bool m_OpenAssetPackDialog = false;

		void SerializeEnvironmentMap();
	};

}

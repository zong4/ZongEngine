#pragma once

#include "Hazel/Renderer/Mesh.h"

#include "Hazel/Scene/Scene.h"

#include "Hazel/Editor/EditorCamera.h"

#include "AssetEditorPanel.h"

namespace Hazel {

	// NOTE(Yan): this is SEVERELY WIP
	class MeshViewerPanel : public AssetEditor
	{
	private:
		struct MeshScene
		{
			Ref<Scene> m_Scene;
			Ref<SceneRenderer> m_SceneRenderer;
			EditorCamera m_Camera { 45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f };
			std::string m_Name;
			Ref<MeshSource> m_MeshAsset;
			Ref<Mesh> m_Mesh;
			Entity m_MeshEntity;
			Entity m_DirectionaLight;
			bool m_ViewportPanelFocused = false;
			bool m_ViewportPanelMouseOver = false;
			bool m_ResetDockspace = true;
			bool m_IsViewportVisible = false;
		};
	public:
		MeshViewerPanel();
		~MeshViewerPanel();

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnEvent(Event& e) override;
		void RenderViewport();
		virtual void OnImGuiRender() override;

		virtual void SetAsset(const Ref<Asset>& asset) override;

		virtual void OnOpen() override {}
		virtual void OnClose() override {}
		virtual void Render() override {}

		void ResetCamera(EditorCamera& camera);
	private:
		void RenderMeshTab(ImGuiID dockspaceID, const std::shared_ptr<MeshScene>& sceneData);
		void DrawMeshNode(const Ref<MeshSource>& meshAsset, const Ref<Mesh>& mesh);
		void MeshNodeHierarchy(const Ref<MeshSource>& meshAsset, Ref<Mesh> mesh, uint32_t nodeIndex, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);
	private:
		bool m_ResetDockspace = true;
	
		std::unordered_map<std::string, std::shared_ptr<MeshScene>> m_OpenMeshes;
		glm::vec2 m_ViewportBounds[2];

		bool m_WindowOpen = false;

		std::string m_TabToFocus;
	};

}

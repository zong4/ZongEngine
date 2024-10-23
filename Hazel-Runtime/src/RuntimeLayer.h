#pragma once

#include "Hazel.h"

#include "Engine/ImGui/ImGuiLayer.h"
#include "Engine/Editor/EditorCamera.h"
#include "imgui/imgui_internal.h"

#include <glm/glm.hpp>

#include <string>

#include "Engine/Editor/SceneHierarchyPanel.h"

namespace Hazel {

	class RuntimeLayer : public Layer
	{
	public:
		RuntimeLayer(std::string_view projectPath);
		virtual ~RuntimeLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(Timestep ts) override;

		virtual void OnEvent(Event& e) override;
		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void OpenProject();
		void OpenScene(const std::string& filepath);

		void LoadScene(AssetHandle sceneHandle);
	private:
		void OnScenePlay();
		void OnSceneStop();
		
		void OnRender2D();

		void QueueSceneTransition(AssetHandle handle);

		void UpdateWindowTitle(const std::string& sceneName);

		void LoadSceneAssets();
		void LoadPrefabAssets(Ref<Prefab> prefab);

		void DrawFPSStats();
		void DrawDebugStats();
		void DrawVersionInfo();
		void DrawString(const std::string& string, const glm::vec2& position, const glm::vec4& color = glm::vec4(1.0f), float size = 50.0f, bool shadow = true);
		void UpdateFPSStat();
		void UpdatePerformanceTimers();

		bool ShouldShowIntroVersion() const;
	private:
		Ref<Scene> m_RuntimeScene;
		Ref<SceneRenderer> m_SceneRenderer;
		Ref<Renderer2D> m_Renderer2D;

		Ref<RenderCommandBuffer> m_CommandBuffer;
		Ref<RenderPass> m_SwapChainRP;
		Ref<Material> m_SwapChainMaterial;

		Ref<AssetPack> m_AssetPack;

		std::string m_ProjectPath;
		bool m_ReloadScriptOnPlay = true;

		std::vector<std::function<void()>> m_PostSceneUpdateQueue;

		glm::mat4 m_Renderer2DProj;

		uint32_t m_Width = 0, m_Height = 0;
		uint32_t m_FramesPerSecond = 0;
		float m_UpdateFPSTimer = 0.0f;
		float m_UpdatePerformanceTimer = 0.0f;

		Application::PerformanceTimers m_PerformanceTimers;
		float m_FrameTime = 0.0f;
		float m_CPUTime = 0.0f;
		float m_GPUTime = 0.0f;
		float m_IntroVersionDuration = 8.0f; // Show version information for first 8 seconds

		// For debugging
		EditorCamera m_EditorCamera;

		bool m_AllowViewportCameraEvents = false;
		bool m_DrawOnTopBoundingBoxes = false;

		bool m_UIShowBoundingBoxes = false;
		bool m_UIShowBoundingBoxesOnTop = false;

		bool m_ViewportPanelMouseOver = false;
		bool m_ViewportPanelFocused = false;

		bool m_ShowPhysicsSettings = false;

		bool m_ShowDebugDisplay = false;
	};

}

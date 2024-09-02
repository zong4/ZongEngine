#include "hzpch.h"
#include "MeshViewerPanel.h"

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Asset/MeshSerializer.h"
#include "Hazel/ImGui/ImGui.h"
#include "Hazel/Math/Math.h"
#include "Hazel/Renderer/SceneRenderer.h"

#include <assimp/scene.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <filesystem>
#include <format>
#include <stack>

namespace Hazel {

	namespace Utils {
		glm::mat4 Mat4FromAIMatrix4x4(const aiMatrix4x4& matrix);
	}

	MeshViewerPanel::MeshViewerPanel()
		: AssetEditor("MeshViewerPanel")
	{
	
	}

	MeshViewerPanel::~MeshViewerPanel()
	{
	}

	void MeshViewerPanel::OnUpdate(Timestep ts)
	{
		for (auto&& [name, sceneData] : m_OpenMeshes)
		{
			sceneData->m_Camera.SetActive(sceneData->m_ViewportPanelFocused);
			sceneData->m_Camera.OnUpdate(ts);
			// m_Scene->OnUpdate(ts);
			if (sceneData->m_IsViewportVisible)
				sceneData->m_Scene->OnRenderEditor(sceneData->m_SceneRenderer, ts, sceneData->m_Camera);
		}
	}

	void MeshViewerPanel::OnEvent(Event& e)
	{
		for (auto&& [name, sceneData] : m_OpenMeshes)
		{
			if (sceneData->m_ViewportPanelMouseOver)
				sceneData->m_Camera.OnEvent(e);
		}
	}

	void MeshViewerPanel::RenderViewport()
	{
	}

	namespace Utils {

		static std::stack<float> s_MinXSizes, s_MinYSizes;
		static std::stack<float> s_MaxXSizes, s_MaxYSizes;

		static void PushMinSizeX(float minSize)
		{
			ImGuiStyle& style = ImGui::GetStyle();
			s_MinXSizes.push(style.WindowMinSize.x);
			style.WindowMinSize.x = minSize;
		}

		static void PopMinSizeX()
		{
			ImGuiStyle& style = ImGui::GetStyle();
			style.WindowMinSize.x = s_MinXSizes.top();
			s_MinXSizes.pop();
		}

		static void PushMinSizeY(float minSize)
		{
			ImGuiStyle& style = ImGui::GetStyle();
			s_MinYSizes.push(style.WindowMinSize.y);
			style.WindowMinSize.y = minSize;
		}

		static void PopMinSizeY()
		{
			ImGuiStyle& style = ImGui::GetStyle();
			style.WindowMinSize.y = s_MinYSizes.top();
			s_MinYSizes.pop();
		}

	}

	void MeshViewerPanel::OnImGuiRender()
	{
		if (m_OpenMeshes.empty() || !m_WindowOpen)
			return;

		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		auto boldFont = io.Fonts->Fonts[0];
		auto largeFont = io.Fonts->Fonts[1];

		const char* windowName = "Asset Viewer";
		// Dockspace
		{
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse;
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin(windowName, &m_WindowOpen, window_flags);
			ImGui::PopStyleVar();
		
			auto window = ImGui::GetCurrentWindow();
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if (window->TitleBarRect().Contains(ImGui::GetMousePos()))
				{
					auto monitor = ImGui::GetPlatformIO().Monitors[window->Viewport->PlatformMonitor];
					ImGui::SetWindowPos(windowName, { monitor.WorkPos });
					ImGui::SetWindowSize(windowName, { monitor.WorkSize });
				}
			}

			if (m_ResetDockspace)
			{
				m_ResetDockspace = false;
				{
					ImGuiID assetViewerDockspaceID = ImGui::GetID("AssetViewerDockspace");
					ImGui::DockBuilderRemoveNode(assetViewerDockspaceID);
					ImGuiID rootNode = ImGui::DockBuilderAddNode(assetViewerDockspaceID, ImGuiDockNodeFlags_DockSpace);
					ImGui::DockBuilderFinish(assetViewerDockspaceID);
				}
			}

			ImGuiID assetViewerDockspaceID = ImGui::GetID("AssetViewerDockspace");
			ImGui::DockSpace(assetViewerDockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

			for (auto&& [name, sceneData] : m_OpenMeshes)
			{
				RenderMeshTab(assetViewerDockspaceID, sceneData);
			}
			ImGui::End();
		}

		if (!m_TabToFocus.empty())
		{
			ImGui::SetWindowFocus(m_TabToFocus.c_str());
			m_TabToFocus.clear();
		}

		// Window has been closed
		if (!m_WindowOpen)
		{
			m_OpenMeshes.clear();
		}

	}

	void MeshViewerPanel::SetAsset(const Ref<Asset>& asset)
	{
		Ref<MeshSource> meshSource = asset.As<MeshSource>();

		auto path = Project::GetEditorAssetManager()->GetFileSystemPath(asset->Handle).string();
		size_t found = path.find_last_of("/\\");
		std::string name = found != std::string::npos ? path.substr(found + 1) : path;
		found = name.find_last_of(".");
		name = found != std::string::npos ? name.substr(0, found) : name;

		if (m_OpenMeshes.find(name) != m_OpenMeshes.end())
		{
			ImGui::SetWindowFocus(name.c_str());
			m_WindowOpen = true;
			return;
		}

		auto& sceneData = m_OpenMeshes[name] = std::make_shared<MeshScene>();
		sceneData->m_MeshSource = meshSource;
		sceneData->m_Name = name;
		sceneData->m_Scene = Ref<Scene>::Create("MeshViewerPanel", true);
		sceneData->m_MeshEntity = sceneData->m_Scene->CreateEntity("Mesh");
		sceneData->m_Mesh = Ref<Mesh>::Create(sceneData->m_MeshSource->Handle, /*generateColliders=*/false);
		sceneData->m_MeshEntity.AddComponent<SubmeshComponent>(sceneData->m_Mesh->Handle);
		sceneData->m_MeshEntity.AddComponent<SkyLightComponent>().DynamicSky = true;

		sceneData->m_DirectionaLight = sceneData->m_Scene->CreateEntity("DirectionalLight");
		sceneData->m_DirectionaLight.AddComponent<DirectionalLightComponent>();
		sceneData->m_DirectionaLight.GetComponent<TransformComponent>().SetRotationEuler(glm::radians(glm::vec3{80.0f, 10.0f, 0.0f}));
		sceneData->m_SceneRenderer = Ref<SceneRenderer>::Create(sceneData->m_Scene);
		sceneData->m_SceneRenderer->SetShadowSettings(-15.0f, 15.0f, 0.95f);

		ResetCamera(sceneData->m_Camera);
		m_TabToFocus = name.c_str();
		m_WindowOpen = true;
	}

	void MeshViewerPanel::ResetCamera(EditorCamera& camera)
	{
		camera = EditorCamera(45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f);
	}

	void MeshViewerPanel::RenderMeshTab(ImGuiID dockspaceID, const std::shared_ptr<MeshScene>& sceneData)
	{
		ImGui::PushID(sceneData->m_Name.c_str());
		const char* meshTabName = sceneData->m_Name.c_str();

		std::string toolBarName = std::format("##{}-{}", meshTabName, "toolbar");
		std::string viewportPanelName = std::format("Viewport##{}", meshTabName);
		std::string propertiesPanelName = std::format("Properties##{}", meshTabName);

		{
			ImGui::Begin(meshTabName, nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse);

			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("Window"))
				{
					if (ImGui::MenuItem("Reset Layout"))
						sceneData->m_ResetDockspace = true;

					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}


			ImGuiID tabDockspaceID = ImGui::GetID("MeshViewerDockspace");
			if (sceneData->m_ResetDockspace)
			{
				sceneData->m_ResetDockspace = false;

				ImGui::DockBuilderDockWindow(meshTabName, dockspaceID);

				// Setup dockspace
				ImGui::DockBuilderRemoveNode(tabDockspaceID);

				ImGuiID rootNode = ImGui::DockBuilderAddNode(tabDockspaceID, ImGuiDockNodeFlags_DockSpace);
				ImGuiID dockDown;
				ImGuiID dockUp = ImGui::DockBuilderSplitNode(rootNode, ImGuiDir_Up, 0.05f, nullptr, &dockDown);
				ImGuiID dockLeft;
				ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockDown, ImGuiDir_Right, 0.5f, nullptr, &dockLeft);

				ImGui::DockBuilderDockWindow(toolBarName.c_str(), dockUp);
				ImGui::DockBuilderDockWindow(viewportPanelName.c_str(), dockLeft);
				ImGui::DockBuilderDockWindow(propertiesPanelName.c_str(), dockRight);
				ImGui::DockBuilderFinish(tabDockspaceID);
			}

			Utils::PushMinSizeX(370.0f);
			ImGui::DockSpace(tabDockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

			{
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

				ImGuiWindowClass window_class;
				window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_AutoHideTabBar;
				ImGui::SetNextWindowClass(&window_class);

				ImGui::Begin(toolBarName.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
				ImGui::Button("Tool A", ImVec2(64, 64));
				ImGui::SameLine();
				ImGui::Button("Tool B", ImVec2(64, 64));
				ImGui::SameLine();
				ImGui::Button("Tool C", ImVec2(64, 64));
				ImGui::End();

				ImGui::SetNextWindowClass(&window_class);
				ImGui::Begin(viewportPanelName.c_str(), nullptr, ImGuiWindowFlags_NoCollapse);

				sceneData->m_ViewportPanelMouseOver = ImGui::IsWindowHovered();
				sceneData->m_ViewportPanelFocused = ImGui::IsWindowFocused();
				auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar
				auto viewportSize = ImGui::GetContentRegionAvail();

				if (viewportSize.y > 0)
				{
					sceneData->m_SceneRenderer->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
					sceneData->m_Camera.SetPerspectiveProjectionMatrix(glm::radians(45.0f), viewportSize.x, viewportSize.y, 0.1f, 1000.0f);
					sceneData->m_Camera.SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);

					if (sceneData->m_SceneRenderer->GetFinalPassImage())
					{
						UI::ImageButton(sceneData->m_SceneRenderer->GetFinalPassImage(), viewportSize, { 0, 1 }, { 1, 0 });
						sceneData->m_IsViewportVisible = ImGui::IsItemVisible();
					}

					static int counter = 0;
					auto windowSize = ImGui::GetWindowSize();
					ImVec2 minBound = ImGui::GetWindowPos();
					minBound.x += viewportOffset.x;
					minBound.y += viewportOffset.y;

					ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.y };
					m_ViewportBounds[0] = { minBound.x, minBound.y };
					m_ViewportBounds[1] = { maxBound.x, maxBound.y };
				}
				ImGui::End();
				ImGui::PopStyleVar();
			}

			{
				ImGui::Begin(propertiesPanelName.c_str(), nullptr, ImGuiWindowFlags_NoCollapse);
				DrawMeshNode(sceneData->m_MeshSource, sceneData->m_Mesh);
				ImGui::End();
			}

			ImGui::End();
			Utils::PopMinSizeX();
		}
		ImGui::PopID();
	}

	void MeshViewerPanel::DrawMeshNode(const Ref<MeshSource>& meshAsset, const Ref<Mesh>& mesh)
	{
#if 0
		// Mesh Hierarchy
		auto rootNode = meshAsset->m_Scene->mRootNode;
		MeshNodeHierarchy(meshAsset, mesh, 0);

		if (ImGui::Button("Create Mesh"))
		{
			std::filesystem::path meshPath = meshAsset->GetFilePath();
			std::filesystem::path directoryPath = meshPath.parent_path();
			std::string filename = std::format("{0}.hmesh", meshPath.stem().string());
			Ref<Mesh> serializedMesh = Project::GetEditorAssetManager()->CreateNewAsset<Mesh>(filename, directoryPath.string(), mesh);
		}
#endif
	}

	void MeshViewerPanel::MeshNodeHierarchy(const Ref<MeshSource>& meshAsset, Ref<Mesh> mesh, uint32_t nodeIndex, const glm::mat4& parentTransform, uint32_t level)
	{
#if 0
		glm::mat4 localTransform = Utils::Mat4FromAIMatrix4x4(node->mTransformation);
		glm::mat4 transform = parentTransform * localTransform;

		HZ_CORE_ASSERT(meshAsset->m_NodeMap.find(node) != meshAsset->m_NodeMap.end());
		auto& meshIndices = meshAsset->m_NodeMap.at(node);
		if (!meshIndices.empty())
		{
			ImGui::PushID(node->mName.C_Str());
			uint32_t meshIndex = meshIndices.front();
			auto& submeshes = mesh->GetSubmeshes();

			bool checked = std::find(submeshes.begin(), submeshes.end(), meshIndex) != submeshes.end();
			if (ImGui::Checkbox("##checkbox", &checked))
			{
				if (checked)
					submeshes.emplace_back(meshIndex);
				else
					submeshes.erase(std::find(submeshes.begin(), submeshes.end(), meshIndex));
			}
			ImGui::PopID();
			ImGui::SameLine();
		}
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		if (node->mNumChildren == 0)
			flags |= ImGuiTreeNodeFlags_Leaf;
		if (ImGui::TreeNodeEx(node->mName.C_Str(), flags))
		{
#if TRANSFORM_INFO
			{
				glm::vec3 translation, rotation, scale;
				Math::DecomposeTransform(transform, translation, rotation, scale);
				ImGui::Text("World Transform");
				ImGui::Text("  Translation: %.2f, %.2f, %.2f", translation.x, translation.y, translation.z);
				ImGui::Text("  Scale: %.2f, %.2f, %.2f", scale.x, scale.y, scale.z);
			}
			{
				glm::vec3 translation, rotation, scale;
				Math::DecomposeTransform(transform, translation, rotation, scale);
				ImGui::Text("Local Transform");
				ImGui::Text("  Translation: %.2f, %.2f, %.2f", translation.x, translation.y, translation.z);
				ImGui::Text("  Scale: %.2f, %.2f, %.2f", scale.x, scale.y, scale.z);
			}
#endif

			for (uint32_t i = 0; i < node->mNumChildren; i++)
				MeshNodeHierarchy(meshAsset, mesh, node->mChildren[i], transform, level + 1);

			ImGui::TreePop();
		}
#endif
	}

}

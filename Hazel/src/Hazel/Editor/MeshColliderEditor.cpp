#include "hzpch.h"
#include "MeshColliderEditor.h"

#include "Hazel/Asset/AssetImporter.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Physics/PhysicsSystem.h"
#include "Hazel/Editor/EditorResources.h"
#include "Hazel/Debug/Profiler.h"

#include <format>
#include <stack>

namespace Hazel {

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

	MeshColliderEditor::MeshColliderEditor()
		: AssetEditor("MeshColliderEditor")
	{
	}

	MeshColliderEditor::~MeshColliderEditor()
	{
	}

	void MeshColliderEditor::OnUpdate(Timestep ts)
	{
		if (m_SelectedTabData == nullptr)
			return;

		m_SelectedTabData->ViewportCamera.SetActive(m_SelectedTabData->ViewportFocused);
		m_SelectedTabData->ViewportCamera.OnUpdate(ts);
		m_SelectedTabData->ViewportScene->OnRenderEditor(m_SelectedTabData->ViewportRenderer, ts, m_SelectedTabData->ViewportCamera);
	}

	void MeshColliderEditor::OnEvent(Event& e)
	{
		if (m_SelectedTabData == nullptr)
			return;

		if (m_SelectedTabData->ViewportHovered)
			m_SelectedTabData->ViewportCamera.OnEvent(e);
	}

	void MeshColliderEditor::SetAsset(const Ref<Asset>& asset)
	{
		m_WindowOpen = true;

		const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(asset->Handle);
		std::string name = metadata.FilePath.stem().string();

		if (auto it = m_OpenTabs.find(asset->Handle); it != m_OpenTabs.end())
		{
			m_SelectedTabData = it->second;
			ImGui::SetWindowFocus(name.c_str());
			return;
		}

		auto tabData = m_OpenTabs[asset->Handle] = std::make_shared<MeshColliderTabData>();
		tabData->Name = name;
		tabData->ColliderAsset = asset.As<MeshColliderAsset>();

		tabData->ViewportScene = Ref<Scene>::Create("MeshColliderPreviewScene", true, false);
		tabData->ColliderEntity = tabData->ViewportScene->CreateEntity("ColliderEntity");
		tabData->ColliderEntity.GetComponent<TransformComponent>().Scale = tabData->ColliderAsset->PreviewScale;
		if (tabData->ColliderAsset->ColliderMesh != 0)
			CookMeshCollider(tabData);

		UpdatePreviewEntity(tabData);

		tabData->SkyLight = tabData->ViewportScene->CreateEntity("SkyLight");
		auto& skyLight = tabData->SkyLight.AddComponent<SkyLightComponent>();
		skyLight.DynamicSky = true;
		Ref<TextureCube> preethamEnv = Renderer::CreatePreethamSky(skyLight.TurbidityAzimuthInclination.x, skyLight.TurbidityAzimuthInclination.y, skyLight.TurbidityAzimuthInclination.z);
		skyLight.SceneEnvironment = AssetManager::AddMemoryOnlyAsset(Ref<Environment>::Create(preethamEnv, preethamEnv));

		tabData->DirectionalLight = tabData->ViewportScene->CreateEntity("DirectionalLight");
		tabData->DirectionalLight.AddComponent<DirectionalLightComponent>();
		tabData->DirectionalLight.GetComponent<TransformComponent>().SetRotationEuler(glm::radians(glm::vec3{80.0f, 10.0f, 0.0f}));

		tabData->ViewportRenderer = Ref<SceneRenderer>::Create(tabData->ViewportScene);
		auto& rendererOptions = tabData->ViewportRenderer->GetOptions();
		rendererOptions.PhysicsColliderMode = SceneRendererOptions::PhysicsColliderView::All;
		rendererOptions.ShowPhysicsColliders = true;

		tabData->ViewportPanelName = std::format("Viewport##{}-{}", tabData->Name, "Viewport");
		tabData->SettingsPanelName = std::format("Settings##{}-{}", tabData->Name, "Settings");
		tabData->ResetDockspace = true;

		m_TabToFocus = name;
		m_SelectedTabData = tabData;
	}

	void MeshColliderEditor::OnImGuiRender()
	{
		HZ_PROFILE_FUNC();

		if (m_OpenTabs.empty() || !m_WindowOpen)
			return;

		static size_t focusFrameCounter = 0;

		const char* windowTitle = "Mesh Collider Editor";

		// Dockspace
		{
			Utils::PushMinSizeY(100.0f);
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar;
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin(windowTitle, &m_WindowOpen, window_flags);
			ImGui::PopStyleVar();

			// Maximize
			auto window = ImGui::GetCurrentWindow();
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if (window->TitleBarRect().Contains(ImGui::GetMousePos()))
				{
					if (!m_Maximized)
					{
						m_OldPosition = ImGui::GetWindowPos();
						m_OldSize = ImGui::GetWindowSize();

						auto monitor = ImGui::GetPlatformIO().Monitors[window->Viewport->PlatformMonitor];
						ImGui::SetWindowPos(windowTitle, monitor.WorkPos);
						ImGui::SetWindowSize(windowTitle, monitor.WorkSize);
					}
					else
					{
						ImGui::SetWindowPos(windowTitle, m_OldPosition);
						ImGui::SetWindowSize(windowTitle, m_OldSize);
					}

					m_Maximized = !m_Maximized;
				}
			}

			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("Options"))
				{
					ImGui::MenuItem("Show Cooking Results", nullptr, &m_ShowCookingResults);
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Window"))
				{
					if (ImGui::MenuItem("Reset Layout"))
						m_SelectedTabData->ResetDockspace = true;

					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			// Rebuild Dockspace
			if (m_RebuildDockspace)
			{
				m_RebuildDockspace = false;
				m_DockSpaceID = ImGui::GetID("MeshColliderEditorDockspace");
				ImGui::DockBuilderRemoveNode(m_DockSpaceID);
				ImGuiID rootNode = ImGui::DockBuilderAddNode(m_DockSpaceID, ImGuiDockNodeFlags_DockSpace);
				ImGui::DockBuilderFinish(m_DockSpaceID);
			}

			{
				UI::ScopedColourStack dockspaceColors(
					ImGuiCol_TitleBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
					ImGuiCol_TitleBgActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
					ImGuiCol_TitleBgCollapsed, ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
					ImGuiCol_TabActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f), // NOTE(Peter): Disable tab bar underline
					ImGuiCol_TabUnfocusedActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f) // NOTE(Peter): Disable tab bar underline
				);

				ImGui::DockSpace(m_DockSpaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_NoCloseButton | ImGuiDockNodeFlags_NoWindowMenuButton);
			}

			for (auto& [handle, tabData] : m_OpenTabs)
				RenderTab(tabData);

			if ((uint64_t)m_ClosedTab != 0)
			{
				m_OpenTabs.erase(m_ClosedTab);
				m_ClosedTab = AssetHandle(0);
			}

			ImGui::End();
			Utils::PopMinSizeY();
		}

		if (!m_TabToFocus.empty())
		{
			focusFrameCounter++;

			// NOTE(Peter): Workaround for ImGui windows not being focusable until after the second frame...
			if (focusFrameCounter >= 2)
			{
				ImGui::SetWindowFocus(m_TabToFocus.c_str());
				m_TabToFocus.clear();
				focusFrameCounter = 0;
			}
		}

		if (m_OpenTabs.empty() || !m_WindowOpen)
		{
			if (!m_OpenTabs.empty())
			{
				for (auto& [handle, tabData] : m_OpenTabs)
				{
					if (tabData->NeedsSaving)
						AssetImporter::Serialize(tabData->ColliderAsset);

					DestroyTab(tabData);
				}
			}

			m_WindowOpen = false;
			m_OpenTabs.clear();
			m_SelectedTabData = nullptr;
		}
	}

	void MeshColliderEditor::RenderTab(const std::shared_ptr<MeshColliderTabData>& tabData)
	{
		HZ_PROFILE_FUNC();
		const std::string dockspaceName = std::format("##{}-DockSpace", tabData->Name);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 2.0f));
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

		if (tabData->NeedsSaving)
			flags |= ImGuiWindowFlags_UnsavedDocument;

		bool drawTab = ImGui::Begin(tabData->Name.c_str(), &tabData->IsOpen, flags);
		ImGui::PopStyleVar(2);

		if (drawTab)
		{
			m_SelectedTabData = tabData;

			if (tabData->ResetDockspace)
			{
				tabData->DockSpaceID = ImGui::GetID(dockspaceName.c_str());
				ImGui::DockBuilderDockWindow(tabData->Name.c_str(), m_DockSpaceID);

				ImGui::DockBuilderRemoveNode(tabData->DockSpaceID);

				ImGuiID rootNode = ImGui::DockBuilderAddNode(tabData->DockSpaceID, ImGuiDockNodeFlags_DockSpace);
				ImGuiID dockLeft;
				ImGuiID dockRight = ImGui::DockBuilderSplitNode(rootNode, ImGuiDir_Right, 0.5f, nullptr, &dockLeft);

				ImGui::DockBuilderDockWindow(tabData->ViewportPanelName.c_str(), dockLeft);
				ImGui::DockBuilderDockWindow(tabData->SettingsPanelName.c_str(), dockRight);
				ImGui::DockBuilderSetNodeSize(dockRight, ImVec2(ImGui::GetContentRegionAvail().x / 2.0f, ImGui::GetContentRegionAvail().y));
				ImGui::DockBuilderFinish(tabData->DockSpaceID);

				tabData->ResetDockspace = false;
			}

			Utils::PushMinSizeX(300.0f);
			ImGui::DockSpace(tabData->DockSpaceID);

			RenderViewportPanel(tabData);
			RenderSettingsPanel(tabData);

			Utils::PopMinSizeX();
		}

		if (!tabData->IsOpen)
		{
			AssetImporter::Serialize(tabData->ColliderAsset);
			m_ClosedTab = tabData->ColliderAsset->Handle;

			if (m_SelectedTabData == tabData)
				m_SelectedTabData = nullptr;

			DestroyTab(tabData);
		}

		ImGui::End();
	}

	void MeshColliderEditor::RenderViewportPanel(const std::shared_ptr<MeshColliderTabData>& tabData)
	{
		HZ_PROFILE_FUNC();
		UI::ScopedStyle windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin(tabData->ViewportPanelName.c_str(), nullptr, ImGuiWindowFlags_NoCollapse);

		tabData->ViewportDockID = ImGui::GetWindowDockID();
		tabData->ViewportHovered = ImGui::IsWindowHovered();
		tabData->ViewportFocused = ImGui::IsWindowFocused();

		auto viewportOffset = ImGui::GetCursorPos();
		auto viewportSize = ImGui::GetContentRegionAvail();

		if (viewportSize.x > 0 && viewportSize.y > 0)
		{
			tabData->ViewportScene->SetViewportSize((uint64_t)viewportSize.x, (uint64_t)viewportSize.y);
			tabData->ViewportRenderer->SetViewportSize((uint64_t)viewportSize.x, (uint64_t)viewportSize.y);
			tabData->ViewportCamera.SetViewportSize((uint64_t)viewportSize.x, (uint64_t)viewportSize.y);
			tabData->ViewportCamera.SetPerspectiveProjectionMatrix(glm::radians(45.0f), viewportSize.x, viewportSize.y, 0.1f, 1000.0f);

			Ref<Image2D> finalImage = tabData->ViewportRenderer->GetFinalPassImage();
			if (finalImage)
				UI::Image(finalImage, viewportSize, { 0, 1 }, { 1, 0 });
		}

		// Shadow
		{
			ImGuiDockNode* viewportDockNode = ImGui::DockBuilderGetNode(tabData->ViewportDockID);
			ImGuiDockNode* settingsDockNode = ImGui::DockBuilderGetNode(tabData->SettingsDockID);

			if (viewportDockNode != nullptr && settingsDockNode != nullptr)
			{
				ImVec2 viewportPos = viewportDockNode->Pos;
				ImVec2 settingsPos = settingsDockNode->Pos;

				if (viewportPos.y == settingsPos.y)
				{
					bool isLeftOfSettings = viewportPos.x < settingsPos.x;
					ImRect windowRect = ImGui::GetCurrentWindow()->Rect();
					ImGui::PushClipRect(windowRect.Min, windowRect.Max, false);
					UI::DrawShadowInner(EditorResources::ShadowTexture, 20, windowRect, 1.0f, windowRect.GetHeight() / 4.0f, !isLeftOfSettings, isLeftOfSettings, false, false);
					ImGui::PopClipRect();
				}
				else
				{
					bool isAboveSettings = viewportPos.y < settingsPos.y;
					ImRect windowRect = ImGui::GetCurrentWindow()->Rect();
					ImGui::PushClipRect(windowRect.Min, windowRect.Max, false);
					UI::DrawShadowInner(EditorResources::ShadowTexture, 20, windowRect, 1.0f, windowRect.GetHeight() / 4.0f, false, false, !isAboveSettings, isAboveSettings);
					ImGui::PopClipRect();
				}
			}
		}

		ImGui::End();
	}

	void MeshColliderEditor::RenderSettingsPanel(const std::shared_ptr<MeshColliderTabData>& tabData)
	{
		HZ_PROFILE_FUNC();
		const auto& meshMetadata = Project::GetEditorAssetManager()->GetMetadata(tabData->ColliderAsset->ColliderMesh);
		const bool hasValidMesh = meshMetadata.IsValid() && (meshMetadata.Type == AssetType::Mesh || meshMetadata.Type == AssetType::StaticMesh);

		ImGui::Begin(tabData->SettingsPanelName.c_str(), nullptr, ImGuiWindowFlags_NoCollapse);
		tabData->SettingsDockID = ImGui::GetWindowDockID();

		// Mesh Selection
		if(UI::PropertyGridHeader("Mesh"))
		{
			UI::BeginPropertyGrid();
			AssetHandle oldMeshHandle = tabData->ColliderAsset->ColliderMesh;
			tabData->NeedsCooking |= UI::PropertyMultiAssetReference<AssetType::Mesh, AssetType::StaticMesh>("Collider Mesh", tabData->ColliderAsset->ColliderMesh, "Specifies what mesh this collider will use.");

			// Workaround having to cook the mesh in order for the new mesh to appear
			if (oldMeshHandle != tabData->ColliderAsset->ColliderMesh)
				UpdatePreviewEntity(tabData);

			UI::EndPropertyGrid();
			ImGui::TreePop();

			// NOTE(Peter): Changing material won't require us to cook the collider again
			if (hasValidMesh && UI::PropertyGridHeader("Collider Material"))
			{
				UI::BeginPropertyGrid();
				tabData->NeedsSaving |= UI::Property("Friction", tabData->ColliderAsset->Material.Friction, 0.1f, 0.0f, 0.0f, "Specifies the friction of this collision mesh. (Default: 0.5)");

				tabData->NeedsSaving |= UI::Property("Restitution", tabData->ColliderAsset->Material.Restitution, 0.1f, 0.0f, 0.0f, "Specifies the restitution (bounciness) of this collision mesh. (Default: 0.15)");

				UI::EndTreeNode();
				UI::EndPropertyGrid();
			}
		}

		// Collider Settings
		if (hasValidMesh && UI::PropertyGridHeader("Collider Settings"))
		{
			UI::BeginPropertyGrid();
			tabData->NeedsSaving |= UI::Property("Always Share Shape", tabData->ColliderAsset->AlwaysShareShape, "Forces ALL entities that use this collider to share the collider data as opposed to making copies of it. (Default: False)");

			static const char* s_ColliderUsageOptions[] = { "Default", "Use Complex as Simple", "Use Simple as Complex" };
			tabData->NeedsCooking |= UI::PropertyDropdown<ECollisionComplexity, uint8_t>("Collision Complexity", s_ColliderUsageOptions, 3, tabData->ColliderAsset->CollisionComplexity, "Specifies the collision complexity of the collider mesh.");
			tabData->NeedsCooking |= UI::Property("Scale", tabData->ColliderAsset->ColliderScale, 0.1f, 0.0f, 0.0f,
				"The scale of the collider. This value is a scalar of the entity scale. (Default: [1, 1, 1])");
			UI::EndPropertyGrid();
			ImGui::TreePop();
		}

		// Cooking settings
		if (hasValidMesh)
		{
			if(UI::PropertyGridHeader("Cooking Settings"))
			{
				UI::BeginPropertyGrid();

				tabData->NeedsCooking |= UI::Property("Vertex Welding", tabData->ColliderAsset->EnableVertexWelding, "Enables Vertex Welding, which will merge multiple vertices into a single vertex if they are withing a given distance. (Default: True)");
				UI::BeginDisabled(!tabData->ColliderAsset->EnableVertexWelding);
				tabData->NeedsCooking |= UI::Property("Weld Tolerance", tabData->ColliderAsset->VertexWeldTolerance, 0.05f, 0.05f, 1.0f,
					"Weld Tolerance controls how close 2 vertices has to be to each other before they merge. (Default: 0.1)");
				tabData->ColliderAsset->VertexWeldTolerance = glm::max(0.05f, tabData->ColliderAsset->VertexWeldTolerance);
				UI::EndDisabled();

				UI::EndPropertyGrid();
				ImGui::TreePop();
			}

			if (tabData->ColliderAsset->CollisionComplexity != ECollisionComplexity::UseComplexAsSimple && UI::PropertyGridHeader("Simple Collider Settings"))
			{
				UI::BeginPropertyGrid();
				tabData->NeedsCooking |= UI::Property("Check Zero-Area Triangles", tabData->ColliderAsset->CheckZeroAreaTriangles,
					"If enabled, any triangle with an area less than the \"Zero-Area Threshold\" will not be included. (Default: True)");

				UI::BeginDisabled(!tabData->ColliderAsset->CheckZeroAreaTriangles);
				tabData->NeedsCooking |= UI::Property("Zero-Area Threshold", tabData->ColliderAsset->AreaTestEpsilon,
					0.1f, 0.0f, 0.0f, "Defines the minimum area a triangle can have before getting discarded. (Default: 0.06)");
				UI::EndDisabled();

				tabData->NeedsCooking |= UI::Property("Shift Vertices to Origin", tabData->ColliderAsset->ShiftVerticesToOrigin,
					"If enabled the vertices will be shifted to be around the origin. Only enable if you encounter issues. (Default: False)");

				UI::EndPropertyGrid();
				ImGui::TreePop();
			}

			if (tabData->ColliderAsset->CollisionComplexity != ECollisionComplexity::UseSimpleAsComplex && UI::PropertyGridHeader("Complex Collider Settings"))
			{
				UI::BeginPropertyGrid();
				tabData->NeedsCooking |= UI::Property("Flip Normals", tabData->ColliderAsset->FlipNormals,
					"If enabled the vertices will have their normals flipped. Effectively changing the winding order of the triangles. (Default: False)");
				UI::EndPropertyGrid();
				ImGui::TreePop();
			}

			// Preview Settings
			if(UI::PropertyGridHeader("Preview Settings"))
			{
				UI::BeginPropertyGrid();
				tabData->NeedsSaving |= UI::Property("Scale", tabData->ColliderAsset->PreviewScale, 0.1f, 0.0f, 0.0f,
					"Modifies the scale of the preview entity. Does NOT affect the collider in any way. (Default: [1, 1, 1])");
				tabData->ColliderEntity.GetComponent<TransformComponent>().Scale = tabData->ColliderAsset->PreviewScale;
				UI::EndPropertyGrid();
				ImGui::PopID();
			}

			UI::ScopedStyle framePadding(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 5.0f));

			UI::BeginDisabled(!tabData->NeedsCooking);
			if (ImGui::Button("Cook Mesh"))
			{
				CookMeshCollider(tabData);
				UpdatePreviewEntity(tabData);
				tabData->NeedsSaving = true;
			}
			UI::SetTooltip("\"Cooks\" this collider asset, meaning it will generate the collider data.");
			UI::EndDisabled();

			ImGui::SameLine();

			UI::BeginDisabled(!tabData->NeedsSaving);
			if (ImGui::Button("Save"))
				Save(tabData);
			UI::EndDisabled();
		}

		//NOTE(Tim): This section of the code is needed to correctly set the layout when the asset is first created, 
		//		     because the tabs have to rendered for 1 frame for ResetDockspace to work properly.
		if (tabData->JustCreated)
		{
			tabData->ResetDockspace = true;
			tabData->JustCreated = false;
		}

		ImGui::End();

		RenderCookingOutput();
	}

	void MeshColliderEditor::UpdatePreviewEntity(const std::shared_ptr<MeshColliderTabData>& tabData)
	{
		HZ_PROFILE_FUNC();
		const auto& colliderMetadata = Project::GetEditorAssetManager()->GetMetadata(tabData->ColliderAsset->ColliderMesh);

		if (!colliderMetadata.IsValid())
			return;

		if (colliderMetadata.Type == AssetType::StaticMesh)
		{
			tabData->ColliderEntity.RemoveComponentIfExists<SubmeshComponent>();
			tabData->ColliderEntity.RemoveComponentIfExists<StaticMeshComponent>();
			tabData->ColliderEntity.AddComponent<StaticMeshComponent>(tabData->ColliderAsset->ColliderMesh);

			tabData->ColliderEntity.RemoveComponentIfExists<MeshColliderComponent>();
			tabData->ColliderEntity.AddComponent<MeshColliderComponent>(tabData->ColliderAsset->Handle);
		}
		else if (colliderMetadata.Type == AssetType::Mesh)
		{
			/*tabData->ColliderEntity.RemoveComponentIfExists<MeshComponent>();
			tabData->ColliderEntity.RemoveComponentIfExists<StaticMeshComponent>();
			tabData->ColliderEntity.AddComponent<MeshComponent>(tabData->ColliderAsset->ColliderMesh);*/

			tabData->ViewportScene->DestroyEntity(tabData->ColliderEntity);
			tabData->ColliderEntity = tabData->ViewportScene->InstantiateMesh(AssetManager::GetAsset<Mesh>(tabData->ColliderAsset->ColliderMesh));

			auto view = tabData->ViewportScene->GetAllEntitiesWith<SubmeshComponent>();
			for (auto enttID : view)
			{
				Entity entity = { enttID, tabData->ViewportScene.Raw() };
				auto& meshCollider = entity.AddComponent<MeshColliderComponent>(tabData->ColliderAsset->Handle);
				meshCollider.SubmeshIndex = entity.GetComponent<SubmeshComponent>().SubmeshIndex;
			}
		}
	}

	bool MeshColliderEditor::CookMeshCollider(const std::shared_ptr<MeshColliderTabData>& tabData)
	{
		//std::tie(m_LastSimpleCookingResult, m_LastComplexCookingResult) = CookingFactory::CookMesh(tabData->ColliderAsset, true);
		//tabData->NeedsCooking = false;

		//// If using the default behavior both colliders must successfully cook
		//if (tabData->ColliderAsset->CollisionComplexity == ECollisionComplexity::Default && m_LastSimpleCookingResult == ECookingResult::Success && m_LastComplexCookingResult == ECookingResult::Success)
		//{
		//	return true;
		//}

		//// If using UseSimpleAsComplex only the simple collider has to cook successfully
		//if (tabData->ColliderAsset->CollisionComplexity == ECollisionComplexity::UseSimpleAsComplex && m_LastSimpleCookingResult == ECookingResult::Success)
		//{
		//	return true;
		//}

		//// If using UseComplexAsSimple only the complex collider has to cook successfully
		//if (tabData->ColliderAsset->CollisionComplexity == ECollisionComplexity::UseComplexAsSimple && m_LastComplexCookingResult == ECookingResult::Success)
		//{
		//	return true;
		//}

		//if (!m_ShowCookingResults)
		//	return false;

		//if (m_LastCookedTabData)
		//	m_LastCookedTabData.reset();

		//m_LastCookedTabData = std::make_shared<MeshColliderTabData>();
		//m_LastCookedTabData->Name = tabData->Name;
		//m_LastCookedTabData->ColliderAsset = tabData->ColliderAsset;
		//m_IsCookingResultsOpen = true;
		return false;
	}

	void MeshColliderEditor::RenderCookingOutput()
	{
		HZ_PROFILE_FUNC();
		if (m_IsCookingResultsOpen && !ImGui::IsPopupOpen("Mesh Collider Cooking Output"))
		{
			ImGui::OpenPopup("Mesh Collider Cooking Output");
		}

		ImGui::SetNextWindowSize(ImVec2(600, 0));
		if (ImGui::BeginPopupModal("Mesh Collider Cooking Output", &m_IsCookingResultsOpen))
		{
			const auto& colliderMeshMetadata = Project::GetEditorAssetManager()->GetMetadata(m_LastCookedTabData->ColliderAsset->ColliderMesh);
			const std::string filename = colliderMeshMetadata.FilePath.filename().string();

			auto getMessageForFailedCookingResult = [](ECookingResult cookingResult)
			{
				switch (cookingResult)
				{
					case Hazel::ECookingResult::ZeroAreaTestFailed: return "Failed to find 4 initial vertices without a small triangle.";
					case Hazel::ECookingResult::PolygonLimitReached:
						return "Successfully cooked mesh collider, but mesh polygon count exceeds 255. Either reduce the number of polygons(or use a simplified mesh for the collider), or enable the \"Quantize Input\" option.";
					case Hazel::ECookingResult::LargeTriangle: return "Failed to cook triangle mesh because one or more triangles are too large. Tessellate the mesh to reduce triangle size.";
					case Hazel::ECookingResult::InvalidMesh: return "Failed to cook mesh because an invalid mesh was provided. Please make sure you provide a valid mesh.";
					case Hazel::ECookingResult::Failure: return "An unknown error occurred. Please check the output logs.";
				}

				return "";
			};

			// Simple Collider
			if (UI::PropertyGridHeader("Simple Collider"))
			{
				UI::Fonts::PushFont("Bold");
				if (m_LastSimpleCookingResult == ECookingResult::Success)
					ImGui::Text("Successfully cooked simple mesh collider for %s", filename.c_str());
				else
					ImGui::Text("Failed to cook simple mesh collider for %s", filename.c_str());
				UI::Fonts::PopFont();

				if (m_LastSimpleCookingResult == ECookingResult::Success)
				{
					const auto& colliderData = PhysicsSystem::GetMeshCache().GetMeshData(m_LastCookedTabData->ColliderAsset);
					ImGui::Text("Submeshes: %d", colliderData.SimpleColliderData.Submeshes.size());
				}
				else
				{
					const auto& physxMessage = PhysicsSystem::GetLastErrorMessage();
					ImGui::TextWrapped("Message: %s\nPhysX Message: %s", getMessageForFailedCookingResult(m_LastSimpleCookingResult), physxMessage.c_str());
				}

				UI::EndTreeNode();
			}

			// Complex Collider
			if (UI::PropertyGridHeader("Complex Collider"))
			{
				UI::Fonts::PushFont("Large");
				if (m_LastSimpleCookingResult == ECookingResult::Success)
					ImGui::Text("Successfully cooked complex mesh collider for %s", filename.c_str());
				else
					ImGui::Text("Failed to cook complex mesh collider for %s", filename.c_str());
				UI::Fonts::PopFont();

				if (m_LastComplexCookingResult == ECookingResult::Success)
				{
					const auto& colliderData = PhysicsSystem::GetMeshCache().GetMeshData(m_LastCookedTabData->ColliderAsset);
					ImGui::Text("Submeshes: %d", colliderData.ComplexColliderData.Submeshes.size());
				}
				else
				{
					const auto& physxMessage = PhysicsSystem::GetLastErrorMessage();
					ImGui::TextWrapped("Message: %s\nPhysX Message: %s", getMessageForFailedCookingResult(m_LastComplexCookingResult), physxMessage.c_str());
				}

				UI::EndTreeNode();
			}

			ImGui::EndPopup();
		}
	}

	void MeshColliderEditor::DestroyTab(const std::shared_ptr<MeshColliderTabData>& tabData)
	{
		tabData->ViewportScene->DestroyEntity(tabData->ColliderEntity);
		tabData->ViewportScene->DestroyEntity(tabData->DirectionalLight);
		AssetHandle environmentHandle = tabData->SkyLight.GetComponent<SkyLightComponent>().SceneEnvironment;
		auto environmentAsset = AssetManager::GetAsset<Environment>(environmentHandle);
		Project::GetEditorAssetManager()->RemoveAsset(environmentHandle);
		tabData->ViewportScene->DestroyEntity(tabData->SkyLight);
		tabData->ViewportRenderer->SetScene(nullptr);

		tabData->ViewportRenderer = nullptr;
		tabData->ViewportScene = nullptr;
		tabData->ColliderAsset = nullptr;
	}

	void MeshColliderEditor::Save(const std::shared_ptr<MeshColliderTabData>& tabData)
	{
		if (tabData->NeedsCooking)
			CookMeshCollider(tabData);

		auto view = m_EditorScene->GetAllEntitiesWith<MeshColliderComponent>();
		for (auto entityID : view)
		{
			Entity entity = { entityID, m_EditorScene.Raw() };
			auto& meshCollider = entity.GetComponent<MeshColliderComponent>();

			if (meshCollider.ColliderAsset != tabData->ColliderAsset->Handle)
				continue;

			meshCollider.UseSharedShape = tabData->ColliderAsset->AlwaysShareShape;
			meshCollider.CollisionComplexity = tabData->ColliderAsset->CollisionComplexity;
		}

		AssetImporter::Serialize(tabData->ColliderAsset);

		UpdatePreviewEntity(tabData);
		tabData->NeedsSaving = false;
	}

}

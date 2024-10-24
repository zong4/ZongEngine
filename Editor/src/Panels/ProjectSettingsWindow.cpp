#include "ProjectSettingsWindow.h"

#include "Engine/ImGui/ImGui.h"

#include "Engine/Project/ProjectSerializer.h"
#include "Engine/Asset/AssetManager.h"
#include "Engine/Physics/PhysicsSystem.h"
#include "Engine/Physics/PhysicsLayer.h"
#include "Engine/Core/Input.h"
#include "Engine/Renderer/Renderer.h"

#include "Engine/Audio/AudioEngine.h"
#include "Engine/Audio/DSP/Reverb/Reverb.h"

namespace Engine {

	ImFont* g_BoldFont = nullptr;

	static bool s_SerializeProject = false;

	ProjectSettingsWindow::ProjectSettingsWindow()
	{
		memset(m_NewLayerNameBuffer, 0, 255);
	}

	ProjectSettingsWindow::~ProjectSettingsWindow()
	{
	}

	void ProjectSettingsWindow::OnImGuiRender(bool& isOpen)
	{
		if (m_Project == nullptr)
		{
			isOpen = false;
			return;
		}

		if (ImGui::Begin("Project Settings", &isOpen))
		{
			RenderGeneralSettings();
			RenderRendererSettings();
			RenderAudioSettings();
			RenderScriptingSettings();
			RenderPhysicsSettings();
			RenderLogSettings();

			// TODO(Peter): We should definitely have these kinds of Physics 2D settings in the project settings
			//				But the 2D physics really needs a major overhaul at some point if we actually want to support it
			//if (m_SceneState == SceneState::Edit)
			//{
			//	float physics2DGravity = m_EditorScene->GetPhysics2DGravity();
			//	float physics2DGravityDelta = physics2DGravity / 1000;
			//	if (UI::Property("Gravity", physics2DGravity, physics2DGravityDelta, -10000.0f, 10000.0f))
			//	{
			//		m_EditorScene->SetPhysics2DGravity(physics2DGravity);
			//	}
			//}
			//else if (m_SceneState == SceneState::Play)
			//{
			//	float physics2DGravity = m_RuntimeScene->GetPhysics2DGravity();
			//	float physics2DGravityDelta = physics2DGravity / 1000;
			//	if (UI::Property("Gravity", physics2DGravity, physics2DGravityDelta, -10000.0f, 10000.0f))
			//	{
			//		m_RuntimeScene->SetPhysics2DGravity(physics2DGravity);
			//	}
			//}
		}

		ImGui::End();

		if (s_SerializeProject)
		{
			ProjectSerializer serializer(m_Project);
			serializer.Serialize(m_Project->m_Config.ProjectDirectory + "/" + m_Project->m_Config.ProjectFileName);
			s_SerializeProject = false;
		}
	}

	void ProjectSettingsWindow::OnProjectChanged(const Ref<Project>& project)
	{
		m_Project = project;
		m_DefaultScene = Project::GetEditorAssetManager()->GetAssetHandleFromFilePath(project->GetConfig().StartScene);
	}

	void ProjectSettingsWindow::RenderGeneralSettings()
	{
		ImGui::PushID("GeneralSettings");
		if (UI::PropertyGridHeader("General"))
		{
			UI::BeginPropertyGrid();

			{
				UI::ScopedDisable disable;
				UI::Property("Name", m_Project->m_Config.Name);
				UI::Property("Asset Directory", m_Project->m_Config.AssetDirectory);
				UI::Property("Asset Registry Path", m_Project->m_Config.AssetRegistryPath);
				UI::Property("Audio Commands Registry Path", m_Project->m_Config.AudioCommandsRegistryPath);
				UI::Property("Mesh Path", m_Project->m_Config.MeshPath);
				UI::Property("Mesh Source Path", m_Project->m_Config.MeshSourcePath);
				UI::Property("Animation Path", m_Project->m_Config.AnimationPath);
				UI::Property("Project Directory", m_Project->m_Config.ProjectDirectory);
			}
			
			if (UI::Property("Auto-save active scene", m_Project->m_Config.EnableAutoSave))
				s_SerializeProject = true;

			if (UI::PropertySlider("Auto save interval (seconds)", m_Project->m_Config.AutoSaveIntervalSeconds, 60, 7200)) // 1 minute to 2 hours allowed range for auto-save.  Somewhat arbitrary...
				s_SerializeProject = true;

			if (UI::PropertyAssetReference<Scene>("Startup Scene", m_DefaultScene))
			{
				const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(m_DefaultScene);
				if (metadata.IsValid())
				{
					m_Project->m_Config.StartScene = metadata.FilePath.string();
					s_SerializeProject = true;
				}
			}

			UI::EndPropertyGrid();
			ImGui::TreePop();
		}
		else
			UI::ShiftCursorY(-(ImGui::GetStyle().ItemSpacing.y + 1.0f));

		ImGui::PopID();
	}

	void ProjectSettingsWindow::RenderRendererSettings()
	{
		ImGui::PushID("RendererSettings");
		if (UI::PropertyGridHeader("Renderer", false))
		{
			UI::BeginPropertyGrid();

			auto& rendererConfig = Renderer::GetConfig();

			UI::Property("Compute HDR Environment Maps", rendererConfig.ComputeEnvironmentMaps);

			static const char* mapSizes[] = { "128", "256", "512", "1024", "2048", "4096" };
			int32_t currentEnvMapSize = (int32_t)glm::log2((float)rendererConfig.EnvironmentMapResolution) - 7;
			if (UI::PropertyDropdown("Environment Map Size", mapSizes, 6, &currentEnvMapSize))
			{
				rendererConfig.EnvironmentMapResolution = (uint32_t)glm::pow(2, currentEnvMapSize + 7);
			}

			int32_t currentIrradianceMapSamples = (int32_t)glm::log2((float)rendererConfig.IrradianceMapComputeSamples) - 7;
			if (UI::PropertyDropdown("Irradiance Map Compute Samples", mapSizes, 6, &currentIrradianceMapSamples))
			{
				rendererConfig.IrradianceMapComputeSamples = (uint32_t)glm::pow(2, currentIrradianceMapSamples + 7);
			}

			UI::EndPropertyGrid();
			ImGui::TreePop();
		}
		else
			UI::ShiftCursorY(-(ImGui::GetStyle().ItemSpacing.y + 1.0f));

		ImGui::PopID();
	}

	void ProjectSettingsWindow::RenderAudioSettings()
	{
		ImGui::PushID("AudioSettings");

		if (UI::PropertyGridHeader("Audio", false))
		{
			// General Audio Settings
			UI::BeginPropertyGrid();
			{
				auto userConfig = MiniAudioEngine::Get().GetUserConfiguration();

				if (UI::Property("Stream files longer than (seconds)", userConfig.FileStreamingDurationThreshold, 0.1f))
				{
					userConfig.FileStreamingDurationThreshold = glm::max(2.0, userConfig.FileStreamingDurationThreshold);
					MiniAudioEngine::Get().SetUserConfiguration(userConfig);
				}
			}
			UI::EndPropertyGrid();

			// Scene Reverb Properties
			if (auto* masterReverb = MiniAudioEngine::Get().GetMasterReverb())
			{
				ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
				ImGui::TextUnformatted("Master Reverb");
				ImGui::PopFont();

				using namespace Audio;
			
				UI::BeginPropertyGrid();

				float preDelay = masterReverb->GetParameter(DSP::EReverbParameters::PreDelay);
				float roomSize = masterReverb->GetParameter(DSP::EReverbParameters::RoomSize) / 0.98f; // Reverb algorithm requirment
				float damping = masterReverb->GetParameter(DSP::EReverbParameters::Damp);
				float width = masterReverb->GetParameter(DSP::EReverbParameters::Width);
				
				if (UI::Property(masterReverb->GetParameterName(DSP::EReverbParameters::PreDelay), preDelay, 1.0f, 0.0f, 1000.0f))
				{
					preDelay = std::min(preDelay, 1000.0f);
					masterReverb->SetParameter(DSP::EReverbParameters::PreDelay, preDelay);
				}
				
				if (UI::Property(masterReverb->GetParameterName(DSP::EReverbParameters::RoomSize), roomSize, 0.0f, 0.0f, 1.0f))
					masterReverb->SetParameter(DSP::EReverbParameters::RoomSize, roomSize * 0.98f);
				
				if (UI::Property(masterReverb->GetParameterName(DSP::EReverbParameters::Damp), damping, 0.0f, 0.0f, 2.0f))
				{
					damping = std::min(damping, 2.0f);  // Reverb algorithm requirment
					masterReverb->SetParameter(DSP::EReverbParameters::Damp, damping);
				}
				
				if (UI::Property(masterReverb->GetParameterName(DSP::EReverbParameters::Width), width, 0.0f, 0.0f, 1.0f))
					masterReverb->SetParameter(DSP::EReverbParameters::Width, width);

				UI::EndPropertyGrid();
			}

			UI::EndTreeNode();
		}
		else
			UI::ShiftCursorY(-(ImGui::GetStyle().ItemSpacing.y + 1.0f));

		ImGui::PopID();
	}

	void ProjectSettingsWindow::RenderScriptingSettings()
	{
		ImGui::PushID("ScriptingSettings");
		if (UI::PropertyGridHeader("Scripting", false))
		{
			UI::BeginPropertyGrid();
			s_SerializeProject |= UI::Property("Script Module Path", m_Project->m_Config.ScriptModulePath);
			s_SerializeProject |= UI::Property("Default Namespace", m_Project->m_Config.DefaultNamespace);
			s_SerializeProject |= UI::Property("Automatically Reload Assembly", m_Project->m_Config.AutomaticallyReloadAssembly);
			UI::EndPropertyGrid();

			ImGui::TreePop();
		}
		else
			UI::ShiftCursorY(-(ImGui::GetStyle().ItemSpacing.y + 1.0f));

		ImGui::PopID();
	}

	void ProjectSettingsWindow::RenderPhysicsSettings()
	{
		ImGui::PushID("PhysicsSettings");
		if (UI::PropertyGridHeader("Physics", false))
		{
			UI::BeginPropertyGrid();

			{
				auto& settings = PhysicsSystem::GetSettings();

				PhysicsAPIType physicsAPIType = Project::GetActive()->m_Config.CurrentPhysicsAPI;

				static const char* physicsAPINames[] = { "Jolt", "PhysX" };
				if (UI::PropertyDropdown<PhysicsAPIType, int32_t>("Physics API", physicsAPINames, 2, physicsAPIType))
				{
					PhysicsSystem::Shutdown();
					PhysicsAPI::SetCurrentAPI(physicsAPIType);
					Project::GetActive()->m_Config.CurrentPhysicsAPI = physicsAPIType;
					PhysicsSystem::Init();
				}

				UI::Property("Fixed Timestep (Default: 0.02)", settings.FixedTimestep);
				UI::Property("Gravity (Default: -9.81)", settings.Gravity.y);

				UI::PropertySlider("Position Solver Iterations", (int&)settings.PositionSolverIterations, 2, 20);
				UI::PropertySlider("Velocity Solver Iterations", (int&)settings.VelocitySolverIterations, 2, 20);

				UI::Property("Max Bodies (Hard Limit)", settings.MaxBodies, 1, 100000);

				UI::Property("Capture on Play", settings.CaptureOnPlay);

				ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Theme::backgroundDark);
				ImGui::PushStyleColor(ImGuiCol_Border, Colors::Theme::backgroundDark);

				auto singleColumnSeparator = []
				{
					ImDrawList* draw_list = ImGui::GetWindowDrawList();
					ImVec2 p = ImGui::GetCursorScreenPos();
					draw_list->AddLine(ImVec2(p.x - 9999, p.y), ImVec2(p.x + 9999, p.y), ImGui::GetColorU32(ImGuiCol_Border));
				};

				ImGui::Spacing();

				auto GetUniqueName = []()
				{
					int counter = 0;
					auto checkID = [&counter](auto checkID) -> std::string
					{
						++counter;
						const std::string counterStr = [&counter] {
							if (counter < 10)
								return "0" + std::to_string(counter);
							else
								return std::to_string(counter);
						}();

						std::string idstr = "NewLayer_" + counterStr;
						if (PhysicsLayerManager::GetLayer(idstr).IsValid())
							return checkID(checkID);
						else
							return idstr;
					};

					return checkID(checkID);
				};

				bool renameSelectedLayer = false;

				if (ImGui::Button("New Layer", { 80.0f, 28.0f }))
				{
					std::string name = GetUniqueName();
					m_SelectedLayer = PhysicsLayerManager::AddLayer(name);
					renameSelectedLayer = true;
					s_SerializeProject = true;
				}

				ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 100.0f), ImVec2(9999.0f, 9999.0f));
				ImGui::BeginChild("LayersList");
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8.0f, 0.0f });

				int32_t layerToDelete = -1;

				for (const auto& layer : PhysicsLayerManager::GetLayers())
				{
					ImGui::PushID(layer.Name.c_str());

					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf;
					bool selected = m_SelectedLayer == layer.LayerID;

					if (selected)
					{
						ImGui::PushStyleColor(ImGuiCol_Text, Colors::Theme::backgroundDark);
						flags |= ImGuiTreeNodeFlags_Selected;
					}

					if (ImGui::TreeNodeEx(layer.Name.c_str(), flags))
						ImGui::TreePop();

					bool itemClicked = ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::IsItemHovered(ImGuiHoveredFlags_None) &&
						ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 1.0f).x == 0.0f &&
						ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 1.0f).y == 0.0f;

					if (itemClicked)
						m_SelectedLayer = layer.LayerID;

					if (selected)
						ImGui::PopStyleColor();

					if (layer.LayerID > 0 && ImGui::BeginPopupContextItem())
					{
						if (ImGui::MenuItem("Rename", "F2"))
						{
							m_SelectedLayer = layer.LayerID;
							renameSelectedLayer = true;
						}

						if (ImGui::MenuItem("Delete", "Delete"))
						{
							if (selected)
								m_SelectedLayer = -1;

							layerToDelete = layer.LayerID;
							s_SerializeProject = true;
						}

						ImGui::EndPopup();
					}

					ImGui::PopID();
				}

				if (Input::IsKeyDown(KeyCode::F2) && m_SelectedLayer != -1)
					renameSelectedLayer = true;

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
					m_SelectedLayer = -1;

				ImGui::PopStyleVar();
				ImGui::EndChild();
				ImGui::PopStyleColor(2);

				ImGui::NextColumn();

				if (m_SelectedLayer != -1)
				{
					PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(m_SelectedLayer);

					auto propertyGridSpacing = []
					{
						ImGui::Spacing();
						ImGui::NextColumn();
						ImGui::NextColumn();
					};

					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::Spacing();
					
					singleColumnSeparator();

					ImGui::Spacing();
					ImGui::Spacing();

					static char buffer[256 + 1] {};
					strncpy(buffer, layerInfo.Name.c_str(), 256);
					ImGui::Text("Layer Name: ");

					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0.08f,0.08f,0.08f,1.0f });

					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3.0f);

					ImGuiInputTextFlags text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;

					if (layerInfo.Name == "Default")
						text_flags |= ImGuiInputTextFlags_ReadOnly;

					ImGui::PushID("selected_layer_name");
					if (renameSelectedLayer)
						ImGui::SetKeyboardFocusHere();
					float maxWidth = ImGui::GetContentRegionAvail().x;
					if (ImGui::InputText("##selected_layer_name", buffer, 256, text_flags))
					{
						PhysicsLayerManager::UpdateLayerName(layerInfo.LayerID, buffer);
						s_SerializeProject = true;
					}
					ImGui::PopID();

					ImGui::Text("Collides with Self: ");
					ImGui::SameLine();
					ImGui::Checkbox("##collides_with_self_ID", &layerInfo.CollidesWithSelf);

					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);

					ImGui::PopStyleColor();

					const float width = ImGui::GetColumnWidth();
					const float borderOffset = 7.0f;
					const float bottomAreaHeight = 50.0f;
					auto layersBounds = ImGui::GetContentRegionAvail();
					layersBounds.x = width - borderOffset;
					//layersBounds.y -= bottomAreaHeight;

					ImGui::BeginChild("Collides With", layersBounds, false);
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 1.0f });
					ImGui::Dummy({ width, 0.0f }); // 1px space offset

					UI::BeginPropertyGrid();
					for (const auto& layer : PhysicsLayerManager::GetLayers())
					{
						if (layer.LayerID == m_SelectedLayer)
							continue;

						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 1.0f); // adding 1px "border"

						bool shouldCollide = layerInfo.CollidesWith & layer.BitValue;
						if (UI::Property(layer.Name.c_str(), shouldCollide))
						{
							PhysicsLayerManager::SetLayerCollision(m_SelectedLayer, layer.LayerID, shouldCollide);
							s_SerializeProject = true;
						}
					}

					UI::EndPropertyGrid();
					ImGui::PopStyleVar();
					ImGui::EndChild();
				}

				if (layerToDelete > 0)
					PhysicsLayerManager::RemoveLayer(layerToDelete);
			}
			UI::EndPropertyGrid();

			UI::BeginPropertyGrid();

			if (ImGui::Button("Rebuild Collider Cache"))
				PhysicsSystem::GetMeshCache().Rebuild();

			UI::EndPropertyGrid();

			ImGui::TreePop();
		}
		else
			UI::ShiftCursorY(-(ImGui::GetStyle().ItemSpacing.y + 1.0f));

		ImGui::PopID();
	}

	void ProjectSettingsWindow::RenderLogSettings()
	{
		ImGui::PushID("LogSettings");
		if (UI::PropertyGridHeader("Log", false))
		{
			UI::BeginPropertyGrid(3);
			auto& tags = Log::EnabledTags();
			for (auto& [name, details] : tags)
			{
				// Don't show untagged log
				if (!name.empty())
				{
					ImGui::PushID(name.c_str());

					UI::Property(name.c_str(), details.Enabled);

					const char* levelStrings[] = { "Trace", "Info", "Warn", "Error", "Fatal"};
					int currentLevel = (int)details.LevelFilter;
					if (UI::PropertyDropdownNoLabel("LevelFilter", levelStrings, 5, &currentLevel))
						details.LevelFilter = (Log::Level)currentLevel;

					ImGui::PopID();
				}
			}
			UI::EndPropertyGrid();
			ImGui::TreePop();
		}
		ImGui::PopID();
	}
}


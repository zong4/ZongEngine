#include "MaterialsPanel.h"

#include "Hazel/ImGui/ImGui.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Editor/SelectionManager.h"

namespace Hazel {

	MaterialsPanel::MaterialsPanel()
	{
		m_CheckerBoardTexture = Texture2D::Create(TextureSpecification(), std::filesystem::path("Resources/Editor/Checkerboard.tga"));
	}

	MaterialsPanel::~MaterialsPanel(){}

	void MaterialsPanel::SetSceneContext(const Ref<Scene>& context)
	{
		m_Context = context;
	}

	void MaterialsPanel::OnImGuiRender(bool& isOpen)
	{
		if (SelectionManager::GetSelectionCount(SelectionContext::Scene) > 0)
		{
			m_SelectedEntity = m_Context->GetEntityWithUUID(SelectionManager::GetSelections(SelectionContext::Scene).front());
		}
		else
		{
			m_SelectedEntity = {};
		}

		const bool hasValidEntity = m_SelectedEntity && m_SelectedEntity.HasAny<MeshComponent, StaticMeshComponent>();

		ImGui::SetNextWindowSize(ImVec2(200.0f, 300.0f), ImGuiCond_Appearing);
		if (ImGui::Begin("Materials", &isOpen) && hasValidEntity)
		{
			const bool hasDynamicMesh = m_SelectedEntity.HasComponent<MeshComponent>() && AssetManager::IsAssetHandleValid(m_SelectedEntity.GetComponent<MeshComponent>().Mesh);
			const bool hasStaticMesh = m_SelectedEntity.HasComponent<StaticMeshComponent>() && AssetManager::IsAssetHandleValid(m_SelectedEntity.GetComponent<StaticMeshComponent>().StaticMesh);

			if (hasDynamicMesh || hasStaticMesh)
			{
				Ref<MaterialTable> meshMaterialTable, componentMaterialTable;

				if (m_SelectedEntity.HasComponent<MeshComponent>())
				{
					const auto& meshComponent = m_SelectedEntity.GetComponent<MeshComponent>();
					componentMaterialTable = meshComponent.MaterialTable;
					auto mesh = AssetManager::GetAsset<Mesh>(meshComponent.Mesh);
					if(mesh)
						meshMaterialTable = mesh->GetMaterials();
				}
				else if (m_SelectedEntity.HasComponent<StaticMeshComponent>())
				{
					const auto& staticMeshComponent = m_SelectedEntity.GetComponent<StaticMeshComponent>();
					componentMaterialTable = staticMeshComponent.MaterialTable;
					auto mesh = AssetManager::GetAsset<StaticMesh>(staticMeshComponent.StaticMesh);
					if (mesh)
						meshMaterialTable = mesh->GetMaterials();
				}

				//HZ_CORE_VERIFY(meshMaterialTable != nullptr && componentMaterialTable != nullptr);
				if (componentMaterialTable)
				{
					if (meshMaterialTable)
					{
						if (componentMaterialTable->GetMaterialCount() < meshMaterialTable->GetMaterialCount())
							componentMaterialTable->SetMaterialCount(meshMaterialTable->GetMaterialCount());
					}

					for (uint32_t i = 0; i < componentMaterialTable->GetMaterialCount(); i++)
					{
						bool hasComponentMaterial = componentMaterialTable->HasMaterial(i);
						bool hasMeshMaterial = meshMaterialTable && meshMaterialTable->HasMaterial(i);

						if (hasMeshMaterial && !hasComponentMaterial)
							RenderMaterial(i, meshMaterialTable->GetMaterial(i));
						else if (hasComponentMaterial)
							RenderMaterial(i, componentMaterialTable->GetMaterial(i));
					}
				}
			}
		}
		ImGui::End();
	}

	void MaterialsPanel::RenderMaterial(size_t materialIndex, AssetHandle materialAssetHandle)
	{
		Ref<MaterialAsset> materialAsset = AssetManager::GetAsset<MaterialAsset>(materialAssetHandle);
		auto material = materialAsset->GetMaterial();
		auto shader = material->GetShader();

		Ref<Shader> transparentShader = Renderer::GetShaderLibrary()->Get("HazelPBR_Transparent");
		bool transparent = shader == transparentShader;

		std::string name = material->GetName();
		if (name.empty())
			name = "Unnamed Material";

		name = fmt::format("{0} ({1}", name, material->GetShader()->GetName());

		if (!UI::PropertyGridHeader(name, materialIndex == 0))
			return;

		// Textures ------------------------------------------------------------------------------
		{
			// Albedo
			if (ImGui::CollapsingHeader("Albedo", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

				auto& albedoColor = material->GetVector3("u_MaterialUniforms.AlbedoColor");
				Ref<Texture2D> albedoMap = material->TryGetTexture2D("u_AlbedoTexture");
				bool hasAlbedoMap = albedoMap && !albedoMap.EqualsObject(Renderer::GetWhiteTexture()) && albedoMap->GetImage();
				Ref<Texture2D> albedoUITexture = hasAlbedoMap ? albedoMap : m_CheckerBoardTexture;

				ImVec2 textureCursorPos = ImGui::GetCursorPos();
				
				UI::Image(albedoUITexture, ImVec2(64, 64));

				if (ImGui::BeginDragDropTarget())
				{
					auto data = ImGui::AcceptDragDropPayload("asset_payload");
					if (data)
					{
						int count = data->DataSize / sizeof(AssetHandle);

						for (int i = 0; i < count; i++)
						{
							if (count > 1)
								break;

							AssetHandle assetHandle = *(((AssetHandle*)data->Data) + i);
							Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
							if (!asset || asset->GetAssetType() != AssetType::Texture)
								break;

							albedoMap = asset.As<Texture2D>();
							material->Set("u_AlbedoTexture", albedoMap);
							//needsSerialize = true;
						}
					}

					ImGui::EndDragDropTarget();
				}

				ImGui::PopStyleVar();

				if (ImGui::IsItemHovered())
				{
					if (hasAlbedoMap)
					{
						ImGui::BeginTooltip();
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						std::string pathString = albedoMap->GetPath().string();
						ImGui::TextUnformatted(pathString.c_str());
						ImGui::PopTextWrapPos();
						UI::Image(albedoUITexture, ImVec2(384, 384));
						ImGui::EndTooltip();
					}

					if (ImGui::IsItemClicked())
					{
						std::string filepath = FileSystem::OpenFileDialog().string();

						if (!filepath.empty())
						{
							TextureSpecification spec;
							spec.SRGB = true;
							albedoMap = Texture2D::Create(spec, filepath);
							material->Set("u_AlbedoTexture", albedoMap);
						}
					}
				}

				ImVec2 nextRowCursorPos = ImGui::GetCursorPos();
				ImGui::SameLine();
				ImVec2 properCursorPos = ImGui::GetCursorPos();
				ImGui::SetCursorPos(textureCursorPos);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
				if (hasAlbedoMap && ImGui::Button("X##AlbedoMap", ImVec2(18, 18)))
				{
					materialAsset->ClearAlbedoMap();
					//needsSerialize = true;
				}
				ImGui::PopStyleVar();
				ImGui::SetCursorPos(properCursorPos);

				if (UI::ColorEdit3("##Albedo", glm::value_ptr(albedoColor), ImGuiColorEditFlags_NoInputs))
					material->Set("u_MaterialUniforms.AlbedoColor", albedoColor);
				ImGui::SameLine();
				ImGui::TextUnformatted("Color");
				//if (ImGui::IsItemDeactivated())
				//	needsSerialize = true;

				float& emissive = material->GetFloat("u_MaterialUniforms.Emission");
				ImGui::SameLine(0, 20.0f);
				ImGui::SetNextItemWidth(100.0f);
				if (UI::DragFloat("##Emission", &emissive, 0.1f, 0.0f, 20.0f))
					material->Set("u_MaterialUniforms.Emission", emissive);
				ImGui::SameLine();
				ImGui::TextUnformatted("Emission");
				//if (ImGui::IsItemDeactivated())
				//	needsSerialize = true;

				ImGui::SetCursorPos(nextRowCursorPos);
			}
		}
		if (!transparent)
		{
			{
				// Normals
				if (ImGui::CollapsingHeader("Normals", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

					bool useNormalMap = material->GetFloat("u_MaterialUniforms.UseNormalMap");
					Ref<Texture2D> normalMap = material->TryGetTexture2D("u_NormalTexture");

					bool hasNormalMap = normalMap && !normalMap.EqualsObject(Renderer::GetWhiteTexture()) && normalMap->GetImage();
					Ref<Texture2D> normalUITexture = hasNormalMap ? normalMap : m_CheckerBoardTexture;

					ImVec2 textureCursorPos = ImGui::GetCursorPos();

					UI::Image(normalUITexture, ImVec2(64, 64));

					if (ImGui::BeginDragDropTarget())
					{
						auto data = ImGui::AcceptDragDropPayload("asset_payload");
						if (data)
						{
							int count = data->DataSize / sizeof(AssetHandle);

							for (int i = 0; i < count; i++)
							{
								if (count > 1)
									break;

								AssetHandle assetHandle = *(((AssetHandle*)data->Data) + i);
								Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
								if (!asset || asset->GetAssetType() != AssetType::Texture)
									break;

								normalMap = asset.As<Texture2D>();
								material->Set("u_NormalTexture", normalMap);
								material->Set("u_MaterialUniforms.UseNormalMap", true);
								//needsSerialize = true;
							}
						}

						ImGui::EndDragDropTarget();
					}

					ImGui::PopStyleVar();

					if (ImGui::IsItemHovered())
					{
						if (hasNormalMap)
						{
							ImGui::BeginTooltip();
							ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
							std::string pathString = normalMap->GetPath().string();
							ImGui::TextUnformatted(pathString.c_str());
							ImGui::PopTextWrapPos();
							UI::Image(normalUITexture, ImVec2(384, 384));
							ImGui::EndTooltip();
						}

						if (ImGui::IsItemClicked())
						{
							std::string filepath = FileSystem::OpenFileDialog().string();

							if (!filepath.empty())
							{
								normalMap = Texture2D::Create(TextureSpecification(), filepath);
								material->Set("u_NormalTexture", normalMap);
								material->Set("u_MaterialUniforms.UseNormalMap", true);
							}
						}
					}

					ImVec2 nextRowCursorPos = ImGui::GetCursorPos();
					ImGui::SameLine();
					ImVec2 properCursorPos = ImGui::GetCursorPos();
					ImGui::SetCursorPos(textureCursorPos);
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
					if (hasNormalMap && ImGui::Button("X##NormalMap", ImVec2(18, 18)))
					{
						materialAsset->ClearNormalMap();
						//needsSerialize = true;
					}
					ImGui::PopStyleVar();
					ImGui::SetCursorPos(properCursorPos);

					if (UI::Checkbox("Use", &useNormalMap))
						material->Set("u_MaterialUniforms.UseNormalMap", useNormalMap);
					//if (ImGui::IsItemDeactivated())
					//	needsSerialize = true;

					ImGui::SetCursorPos(nextRowCursorPos);
				}
			}
			{
				// Metalness
				if (ImGui::CollapsingHeader("Metalness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

					float& metalnessValue = material->GetFloat("u_MaterialUniforms.Metalness");
					Ref<Texture2D> metalnessMap = material->TryGetTexture2D("u_MetalnessTexture");

					bool hasMetalnessMap = metalnessMap && !metalnessMap.EqualsObject(Renderer::GetWhiteTexture()) && metalnessMap->GetImage();
					Ref<Texture2D> metalnessUITexture = hasMetalnessMap ? metalnessMap : m_CheckerBoardTexture;

					ImVec2 textureCursorPos = ImGui::GetCursorPos();

					UI::Image(metalnessUITexture, ImVec2(64, 64));

					if (ImGui::BeginDragDropTarget())
					{
						auto data = ImGui::AcceptDragDropPayload("asset_payload");
						if (data)
						{
							int count = data->DataSize / sizeof(AssetHandle);

							for (int i = 0; i < count; i++)
							{
								if (count > 1)
									break;

								AssetHandle assetHandle = *(((AssetHandle*)data->Data) + i);
								Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
								if (!asset || asset->GetAssetType() != AssetType::Texture)
									break;

								metalnessMap = asset.As<Texture2D>();
								material->Set("u_MetalnessTexture", metalnessMap);
								//needsSerialize = true;
							}
						}

						ImGui::EndDragDropTarget();
					}

					ImGui::PopStyleVar();

					if (ImGui::IsItemHovered())
					{
						if (hasMetalnessMap)
						{
							ImGui::BeginTooltip();
							ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
							std::string pathString = metalnessMap->GetPath().string();
							ImGui::TextUnformatted(pathString.c_str());
							ImGui::PopTextWrapPos();
							UI::Image(metalnessUITexture, ImVec2(384, 384));
							ImGui::EndTooltip();
						}

						if (ImGui::IsItemClicked())
						{
							std::string filepath = FileSystem::OpenFileDialog().string();

							if (!filepath.empty())
							{
								metalnessMap = Texture2D::Create(TextureSpecification(), filepath);
								material->Set("u_MetalnessTexture", metalnessMap);
							}
						}
					}

					ImVec2 nextRowCursorPos = ImGui::GetCursorPos();
					ImGui::SameLine();
					ImVec2 properCursorPos = ImGui::GetCursorPos();
					ImGui::SetCursorPos(textureCursorPos);
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
					if (hasMetalnessMap && ImGui::Button("X##MetalnessMap", ImVec2(18, 18)))
					{
						materialAsset->ClearMetalnessMap();
						//needsSerialize = true;
					}
					ImGui::PopStyleVar();
					ImGui::SetCursorPos(properCursorPos);
					ImGui::SetNextItemWidth(200.0f);
					if (UI::SliderFloat("##MetalnessInput", &metalnessValue, 0.0f, 1.0f))
						material->Set("u_MaterialUniforms.Metalness", metalnessValue);
					ImGui::SameLine();
					ImGui::TextUnformatted("Metalness Value");
					//if (ImGui::IsItemDeactivated())
					//	needsSerialize = true;
					ImGui::SetCursorPos(nextRowCursorPos);
				}
			}
			{
				// Roughness
				if (ImGui::CollapsingHeader("Roughness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

					float& roughnessValue = material->GetFloat("u_MaterialUniforms.Roughness");
					Ref<Texture2D> roughnessMap = material->TryGetTexture2D("u_RoughnessTexture");

					bool hasRoughnessMap = roughnessMap && !roughnessMap.EqualsObject(Renderer::GetWhiteTexture()) && roughnessMap->GetImage();
					Ref<Texture2D> roughnessUITexture = hasRoughnessMap ? roughnessMap : m_CheckerBoardTexture;

					ImVec2 textureCursorPos = ImGui::GetCursorPos();

					UI::Image(roughnessUITexture, ImVec2(64, 64));

					if (ImGui::BeginDragDropTarget())
					{
						auto data = ImGui::AcceptDragDropPayload("asset_payload");
						if (data)
						{
							int count = data->DataSize / sizeof(AssetHandle);

							for (int i = 0; i < count; i++)
							{
								if (count > 1)
									break;

								AssetHandle assetHandle = *(((AssetHandle*)data->Data) + i);
								Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
								if (!asset || asset->GetAssetType() != AssetType::Texture)
									break;

								roughnessMap = asset.As<Texture2D>();
								material->Set("u_RoughnessTexture", roughnessMap);
								//needsSerialize = true;
							}
						}

						ImGui::EndDragDropTarget();
					}

					ImGui::PopStyleVar();

					if (ImGui::IsItemHovered())
					{
						if (hasRoughnessMap)
						{
							ImGui::BeginTooltip();
							ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
							std::string pathString = roughnessMap->GetPath().string();
							ImGui::TextUnformatted(pathString.c_str());
							ImGui::PopTextWrapPos();
							UI::Image(roughnessUITexture, ImVec2(384, 384));
							ImGui::EndTooltip();
						}

						if (ImGui::IsItemClicked())
						{
							std::string filepath = FileSystem::OpenFileDialog().string();

							if (!filepath.empty())
							{
								roughnessMap = Texture2D::Create(TextureSpecification(), filepath);
								material->Set("u_RoughnessTexture", roughnessMap);
							}
						}
					}

					ImVec2 nextRowCursorPos = ImGui::GetCursorPos();
					ImGui::SameLine();
					ImVec2 properCursorPos = ImGui::GetCursorPos();
					ImGui::SetCursorPos(textureCursorPos);
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
					if (hasRoughnessMap && ImGui::Button("X##RoughnessMap", ImVec2(18, 18)))
					{
						materialAsset->ClearRoughnessMap();
						//needsSerialize = true;
					}
					ImGui::PopStyleVar();
					ImGui::SetCursorPos(properCursorPos);
					ImGui::SetNextItemWidth(200.0f);
					if (UI::SliderFloat("##RoughnessInput", &roughnessValue, 0.0f, 1.0f))
						material->Set("u_MaterialUniforms.Roughness", roughnessValue);
					ImGui::SameLine();
					ImGui::TextUnformatted("Roughness Value");
					//if (ImGui::IsItemDeactivated())
					//	needsSerialize = true;
					ImGui::SetCursorPos(nextRowCursorPos);
				}
			}
		}

		UI::EndTreeNode();
	}

}

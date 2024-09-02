#include "hzpch.h"
#include "DefaultAssetEditors.h"
#include "Hazel/Asset/AssetImporter.h"
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Audio/AudioFileUtils.h"
#include "Hazel/Audio/Sound.h"
#include "Hazel/Editor/NodeGraphEditor/SoundGraph/SoundGraphAsset.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Editor/SelectionManager.h"
#include "Hazel/Audio/AudioEngine.h"

#include "imgui_internal.h"

namespace Hazel {

	MaterialEditor::MaterialEditor()
		: AssetEditor("Material Editor")
	{
	}

	void MaterialEditor::OnOpen()
	{
		if (!m_MaterialAsset)
			SetOpen(false);
	}

	void MaterialEditor::OnClose()
	{
		m_MaterialAsset = nullptr;
	}

	void MaterialEditor::Render()
	{
		auto material = m_MaterialAsset->GetMaterial();

		Ref<Shader> shader = material->GetShader();
		bool needsSerialize = false;

		Ref<Shader> transparentShader = Renderer::GetShaderLibrary()->Get("HazelPBR_Transparent");
		bool transparent = shader == transparentShader;
		UI::BeginPropertyGrid();
		UI::PushID();
		if (UI::Property("Transparent", transparent))
		{
			if (transparent)
				m_MaterialAsset->SetMaterial(Material::Create(transparentShader));
			else
				m_MaterialAsset->SetMaterial(Material::Create(Renderer::GetShaderLibrary()->Get("HazelPBR_Static")));

			m_MaterialAsset->m_Transparent = transparent;
			m_MaterialAsset->SetDefaults();

			material = m_MaterialAsset->GetMaterial();
			shader = material->GetShader();
		}
		UI::PopID();
		UI::EndPropertyGrid();

		ImGui::Text("Shader: %s", material->GetShader()->GetName().c_str());
		
		auto checkAndSetTexture = [&material, &needsSerialize](Ref<Texture2D>& texture2D) {
			auto data = ImGui::AcceptDragDropPayload("asset_payload"); 
			if (data && data->DataSize / sizeof(AssetHandle) == 1)
			{
				AssetHandle assetHandle = *(((AssetHandle*)data->Data));
				Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
				if (asset && asset->GetAssetType() == AssetType::Texture)
				{
					texture2D = asset.As<Texture2D>();
					needsSerialize = true;
				}
				return true;
			}
			return false;
		};

		// Albedo
		if (ImGui::CollapsingHeader("Albedo", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

			auto& albedoColor = material->GetVector3("u_MaterialUniforms.AlbedoColor");
			Ref<Texture2D> albedoMap = material->TryGetTexture2D("u_AlbedoTexture");
			bool hasAlbedoMap = albedoMap ? !albedoMap.EqualsObject(Renderer::GetWhiteTexture()) && albedoMap->GetImage() : false;
			Ref<Texture2D> albedoUITexture = hasAlbedoMap ? albedoMap : EditorResources::CheckerboardTexture;

			ImVec2 textureCursorPos = ImGui::GetCursorPos();
			UI::Image(albedoUITexture, ImVec2(64, 64));
			if (ImGui::BeginDragDropTarget())
			{
				if (checkAndSetTexture(albedoMap))
				{
					material->Set("u_AlbedoTexture", albedoMap);
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
					std::string filepath = albedoMap->GetPath().string();
					ImGui::TextUnformatted(filepath.c_str());
					ImGui::PopTextWrapPos();
					UI::Image(albedoUITexture, ImVec2(384, 384));
					ImGui::EndTooltip();
				}
				if (ImGui::IsItemClicked())
				{
				}
			}

			ImVec2 nextRowCursorPos = ImGui::GetCursorPos();
			ImGui::SameLine();
			ImVec2 properCursorPos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(textureCursorPos);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
			if (hasAlbedoMap && ImGui::Button("X##AlbedoMap", ImVec2(18, 18)))
			{
				m_MaterialAsset->ClearAlbedoMap();
				needsSerialize = true;
			}
			ImGui::PopStyleVar();
			ImGui::SetCursorPos(properCursorPos);

			UI::ColorEdit3("##Albedo", glm::value_ptr(albedoColor), ImGuiColorEditFlags_NoInputs);
			if (ImGui::IsItemDeactivated())
				needsSerialize = true;
			ImGui::SameLine();
			ImGui::TextUnformatted("Color");

			float& emissive = material->GetFloat("u_MaterialUniforms.Emission");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(100.0f);
			UI::DragFloat("##Emission", &emissive, 0.1f, 0.0f, 20.0f);
			if (ImGui::IsItemDeactivated())
				needsSerialize = true;
			ImGui::SameLine();
			ImGui::TextUnformatted("Emission");

			ImGui::SetCursorPos(nextRowCursorPos);
		}

		if (transparent)
		{
			float& transparency = material->GetFloat("u_MaterialUniforms.Transparency");

			UI::BeginPropertyGrid();
			UI::Property("Transparency", transparency, 0.01f, 0.0f, 1.0f);
			if (ImGui::IsItemDeactivated())
				needsSerialize = true;
			UI::EndPropertyGrid();
		}
		else
		{
			// Textures ------------------------------------------------------------------------------
			{
				// Normals
				if (ImGui::CollapsingHeader("Normals", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
					bool useNormalMap = material->GetFloat("u_MaterialUniforms.UseNormalMap");
					Ref<Texture2D> normalMap = material->TryGetTexture2D("u_NormalTexture");

					bool hasNormalMap = normalMap ? !normalMap.EqualsObject(Renderer::GetWhiteTexture()) && normalMap->GetImage() : false;
					ImVec2 textureCursorPos = ImGui::GetCursorPos();
					UI::Image(hasNormalMap ? normalMap : EditorResources::CheckerboardTexture, ImVec2(64, 64));

					if (ImGui::BeginDragDropTarget())
					{
						if (checkAndSetTexture(normalMap))
						{
							material->Set("u_NormalTexture", normalMap);
							material->Set("u_MaterialUniforms.UseNormalMap", true);
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
							std::string filepath = normalMap->GetPath().string();
							ImGui::TextUnformatted(filepath.c_str());
							ImGui::PopTextWrapPos();
							UI::Image(normalMap, ImVec2(384, 384));
							ImGui::EndTooltip();
						}
						if (ImGui::IsItemClicked())
						{
						}
					}
					ImVec2 nextRowCursorPos = ImGui::GetCursorPos();
					ImGui::SameLine();
					ImVec2 properCursorPos = ImGui::GetCursorPos();
					ImGui::SetCursorPos(textureCursorPos);
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
					if (hasNormalMap && ImGui::Button("X##NormalMap", ImVec2(18, 18)))
					{
						m_MaterialAsset->ClearNormalMap();
						needsSerialize = true;
					}
					ImGui::PopStyleVar();
					ImGui::SetCursorPos(properCursorPos);

					if (UI::Checkbox("##Use", &useNormalMap))
						material->Set("u_MaterialUniforms.UseNormalMap", useNormalMap);
					if (ImGui::IsItemDeactivated())
						needsSerialize = true;
					ImGui::SameLine();
					ImGui::TextUnformatted("Use");

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

					bool hasMetalnessMap = metalnessMap ? !metalnessMap.EqualsObject(Renderer::GetWhiteTexture()) && metalnessMap->GetImage() : false;
					ImVec2 textureCursorPos = ImGui::GetCursorPos();
					UI::Image(hasMetalnessMap ? metalnessMap : EditorResources::CheckerboardTexture, ImVec2(64, 64));

					if (ImGui::BeginDragDropTarget())
					{
						if (checkAndSetTexture(metalnessMap))
						{
							material->Set("u_MetalnessTexture", metalnessMap);
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
							std::string filepath = metalnessMap->GetPath().string();
							ImGui::TextUnformatted(filepath.c_str());
							ImGui::PopTextWrapPos();
							UI::Image(metalnessMap, ImVec2(384, 384));
							ImGui::EndTooltip();
						}
						if (ImGui::IsItemClicked())
						{
						}
					}
					ImVec2 nextRowCursorPos = ImGui::GetCursorPos();
					ImGui::SameLine();
					ImVec2 properCursorPos = ImGui::GetCursorPos();
					ImGui::SetCursorPos(textureCursorPos);
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
					if (hasMetalnessMap && ImGui::Button("X##MetalnessMap", ImVec2(18, 18)))
					{
						m_MaterialAsset->ClearMetalnessMap();
						needsSerialize = true;
					}
					ImGui::PopStyleVar();
					ImGui::SetCursorPos(properCursorPos);
					ImGui::SetNextItemWidth(200.0f);
					UI::SliderFloat("##MetalnessInput", &metalnessValue, 0.0f, 1.0f);
					if (ImGui::IsItemDeactivated())
						needsSerialize = true;
					ImGui::SameLine();
					ImGui::TextUnformatted("Metalness Value");
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
					bool hasRoughnessMap = roughnessMap ? !roughnessMap.EqualsObject(Renderer::GetWhiteTexture()) && roughnessMap->GetImage() : false;
					ImVec2 textureCursorPos = ImGui::GetCursorPos();
					UI::Image(hasRoughnessMap ? roughnessMap : EditorResources::CheckerboardTexture, ImVec2(64, 64));

					if (ImGui::BeginDragDropTarget())
					{
						if (checkAndSetTexture(roughnessMap))
						{
							material->Set("u_RoughnessTexture", roughnessMap);
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
							std::string filepath = roughnessMap->GetPath().string();
							ImGui::TextUnformatted(filepath.c_str());
							ImGui::PopTextWrapPos();
							UI::Image(roughnessMap, ImVec2(384, 384));
							ImGui::EndTooltip();
						}
						if (ImGui::IsItemClicked())
						{

						}
					}
					ImVec2 nextRowCursorPos = ImGui::GetCursorPos();
					ImGui::SameLine();
					ImVec2 properCursorPos = ImGui::GetCursorPos();
					ImGui::SetCursorPos(textureCursorPos);
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
					if (hasRoughnessMap && ImGui::Button("X##RoughnessMap", ImVec2(18, 18)))
					{
						m_MaterialAsset->ClearRoughnessMap();
						needsSerialize = true;
					}
					ImGui::PopStyleVar();
					ImGui::SetCursorPos(properCursorPos);
					ImGui::SetNextItemWidth(200.0f);
					UI::SliderFloat("##RoughnessInput", &roughnessValue, 0.0f, 1.0f);
					if (ImGui::IsItemDeactivated())
						needsSerialize = true;
					ImGui::SameLine();
					ImGui::TextUnformatted("Roughness Value");
					ImGui::SetCursorPos(nextRowCursorPos);
				}
			}

			UI::BeginPropertyGrid();

			bool castsShadows = m_MaterialAsset->IsShadowCasting();
			if (UI::Property("Casts shadows", castsShadows))
				m_MaterialAsset->SetShadowCasting(castsShadows);

			UI::EndPropertyGrid();
		}

		if (needsSerialize)
			AssetImporter::Serialize(m_MaterialAsset);
	}

	TextureViewer::TextureViewer()
		: AssetEditor("Edit Texture")
	{
		SetMinSize(200, 600);
		SetMaxSize(500, 1000);
	}

	void TextureViewer::OnOpen()
	{
		if (!m_Asset)
			SetOpen(false);
	}

	void TextureViewer::OnClose()
	{
		m_Asset = nullptr;
	}

	void TextureViewer::Render()
	{
		float textureWidth = (float)m_Asset->GetWidth();
		float textureHeight = (float)m_Asset->GetHeight();
		//float bitsPerPixel = Texture::GetBPP(m_Asset->GetFormat());
		float imageSize = ImGui::GetWindowWidth() - 40;
		imageSize = glm::min(imageSize, 500.0f);

		ImGui::SetCursorPosX(20);
		//ImGui::Image((ImTextureID)m_Asset->GetRendererID(), { imageSize, imageSize });

		UI::BeginPropertyGrid();
		UI::BeginDisabled();
		UI::Property("Width", textureWidth);
		UI::Property("Height", textureHeight);
		// UI::Property("Bits", bitsPerPixel, 0.1f, 0.0f, 0.0f, true); // TODO: Format
		UI::EndDisabled();
		UI::EndPropertyGrid();
	}

	AudioFileViewer::AudioFileViewer()
		: AssetEditor("Audio File")
	{
		SetMinSize(340, 340);
		SetMaxSize(500, 500);
	}

	void AudioFileViewer::SetAsset(const Ref<Asset>& asset)
	{
		m_Asset = (Ref<AudioFile>)asset;

		const AssetMetadata& metadata = Project::GetEditorAssetManager()->GetMetadata(m_Asset.As<AudioFile>()->Handle);
		SetTitle(metadata.FilePath.stem().string());
	}

	void AudioFileViewer::OnOpen()
	{
		if (!m_Asset)
			SetOpen(false);
	}

	void AudioFileViewer::OnClose()
	{
		m_Asset = nullptr;
	}

	void AudioFileViewer::Render()
	{
		std::chrono::duration<double> seconds{ m_Asset->Duration };
		auto duration = Utils::DurationToString(seconds);

		auto samplingRate = std::to_string(m_Asset->SamplingRate) + " Hz";
		auto bitDepth = std::to_string(m_Asset->BitDepth) + "-bit";
		auto channels = AudioFileUtils::ChannelsToLayoutString(m_Asset->NumChannels);
		auto fileSize = Utils::BytesToString(m_Asset->FileSize);
		auto filePath = Project::GetEditorAssetManager()->GetMetadata(m_Asset->Handle).FilePath.string();

		auto localBounds = ImGui::GetContentRegionAvail();
		localBounds.y -= 14.0f; // making sure to not overlap resize corner

		// Hacky way to hide Property widget background, while making overall text contrast more pleasant
		auto& colors = ImGui::GetStyle().Colors;
		ImGui::PushStyleColor(ImGuiCol_ChildBg, colors[ImGuiCol_FrameBg]);

		ImGui::BeginChild("AudioFileProps", localBounds, false);
		ImGui::Spacing();

		UI::BeginPropertyGrid();
		UI::Property("Duration", duration.c_str());
		UI::Property("Sampling rate", samplingRate.c_str());
		UI::Property("Bit depth", bitDepth.c_str());
		UI::Property("Channels", channels.c_str());
		UI::Property("File size", fileSize.c_str());
		UI::Property("File path", filePath.c_str());
		UI::EndPropertyGrid();

		ImGui::EndChild();

		ImGui::PopStyleColor(); // ImGuiCol_ChildBg
	}

	SoundConfigEditor::SoundConfigEditor()
		: AssetEditor("Sound Configuration")
	{
		SetMinSize(340, 340);
		SetMaxSize(700, 700);
	}

	void SoundConfigEditor::SetAsset(const Ref<Asset>& asset)
	{
		MiniAudioEngine::Get().StopActiveAudioFilePreview();

		m_Asset = (Ref<SoundConfig>)asset;

		const AssetMetadata& metadata = Project::GetEditorAssetManager()->GetMetadata(m_Asset.As<SoundConfig>()->Handle);
		SetTitle(metadata.FilePath.stem().string());
	}

	void SoundConfigEditor::OnOpen()
	{
		MiniAudioEngine::Get().StopActiveAudioFilePreview();

		if (!m_Asset)
			SetOpen(false);
	}

	void SoundConfigEditor::OnClose()
	{
		MiniAudioEngine::Get().StopActiveAudioFilePreview();

		if (m_Asset)
			AssetImporter::Serialize(m_Asset);

		m_Asset = nullptr;
	}

	void SoundConfigEditor::Render()
	{
		auto localBounds = ImGui::GetContentRegionAvail();
		localBounds.y -= 14.0f; // making sure to not overlap resize corner

		ImGui::BeginChild("SoundConfigPanel", localBounds, false);
		ImGui::Spacing();

		{
			// PropertyGrid consists out of 2 columns, so need to move cursor accordingly
			auto propertyGridSpacing = []
			{
				ImGui::Spacing();
				ImGui::NextColumn();
				ImGui::NextColumn();
			};

			//=======================================================

			SoundConfig& soundConfig = *m_Asset;

			// Adding space after header
			ImGui::Spacing();

			//--- Sound Assets and Looping
			//----------------------------
			UI::PushID();
			UI::BeginPropertyGrid();
			// Need to wrap this first Property Grid into another ID,
			// otherwise there's a conflict with the next Property Grid.

			{
				UI::PropertyAssetReferenceError error;
				UI::PropertyAssetReferenceSettings settings;
				settings.ShowFullFilePath = false;
				UI::PropertyMultiAssetReference<AssetType::Audio, AssetType::SoundGraphSound>("Source", soundConfig.DataSourceAsset, "", &error, settings);
			}

			if (soundConfig.DataSourceAsset)
			{
				const auto path = Project::GetEditorAssetManager()->GetFileSystemPath(soundConfig.DataSourceAsset);
				if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path))
				{
					const bool isValidAudioFile = AudioFileUtils::IsValidAudioFile(path.string());

					UI::BeginDisabled(!isValidAudioFile);

					ImGui::NextColumn();

					if (ImGui::Button("Preview"))
					{
						MiniAudioEngine::Get().PreviewAudioFile(soundConfig.DataSourceAsset);
					}

					ImGui::SameLine();
					
					if (ImGui::Button("Stop"))
					{
						MiniAudioEngine::Get().StopActiveAudioFilePreview();
					}

					ImGui::NextColumn();

					UI::EndDisabled();
				}
			}

			propertyGridSpacing();

			UI::Property("Volume Multiplier", soundConfig.VolumeMultiplier, 0.01f, 0.0f, 1.0f); //TODO: switch to dBs in the future ?
			UI::Property("Pitch Multiplier", soundConfig.PitchMultiplier, 0.01f, 0.0f, 24.0f); // max pitch 24 is just an arbitrary number here

			propertyGridSpacing();
			propertyGridSpacing();

			UI::Property("Looping", soundConfig.bLooping);

			propertyGridSpacing();
			propertyGridSpacing();

			// Currently we use normalized value for the filter properties,
			// which are then translated to frequency scale

			//? was trying to make a more usable logarithmic scale for the slider value,
			//? so that 0.5 represents 1.2 kHz, or 0.0625 of frequency range normalized
			auto logFrequencyToNormScale = [](float frequencyN)
			{
				return (log2(frequencyN) + 8.0f) / 8.0f;
			};
			auto sliderScaleToLogFreq = [](float sliderValue)
			{
				return pow(2.0f, 8.0f * sliderValue - 8.0f);
			};

			//float lpfFreq = logFrequencyToNormScale(1.0f - soundConfig.LPFilterValue * 0.99609375f);
			float lpfFreq = /*1.0f -*/ soundConfig.LPFilterValue;
			if (UI::Property("Low-Pass Filter", lpfFreq, 0.0f, 0.0f, 1.0f))
			{
				lpfFreq = std::clamp(lpfFreq, 0.0f, 1.0f);
				//soundConfig.LPFilterValue = sliderScaleToLogFreq(lpfFreq);
				soundConfig.LPFilterValue = /*1.0f - */lpfFreq;
			}

			//float hpfFreq = logFrequencyToNormScale(soundConfig.HPFilterValue * 0.99609375f + 0.00390625f);
			float hpfFreq = soundConfig.HPFilterValue;
			if (UI::Property("High-Pass Filter", hpfFreq, 0.0f, 0.0f, 1.0f))
			{
				hpfFreq = std::clamp(hpfFreq, 0.0f, 1.0f);
				//soundConfig.HPFilterValue = sliderScaleToLogFreq(1.0f - hpfFreq);
				soundConfig.HPFilterValue = hpfFreq;
			}

			propertyGridSpacing();
			propertyGridSpacing();

			UI::Property("Master Reverb send", soundConfig.MasterReverbSend, 0.01f, 0.0f, 1.0f);

			UI::EndPropertyGrid();
			UI::PopID();

			ImGui::Spacing();
			ImGui::Spacing();

			auto contentRegionAvailable = ImGui::GetContentRegionAvail();

			//--- Enable Spatialization
			//-------------------------
			ImGui::Spacing();
			ImGui::Spacing();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4.0f, 6.0f });
			bool spatializationOpen = UI::PropertyGridHeader("Spatialization", true);
			ImGui::PopStyleVar(); // ImGuiStyleVar_FramePadding
			ImGui::SameLine(contentRegionAvailable.x - (ImGui::GetFrameHeight() + GImGui->Style.FramePadding.y * 2.0f));
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
			UI::Checkbox("##enabled", &soundConfig.bSpatializationEnabled);

			ImGui::Indent(12.0f);

			//--- Spatialization Settings
			//---------------------------
			if (spatializationOpen)
			{
				ImGui::Spacing();

				using AttModel = AttenuationModel;

				auto& spatialConfig = soundConfig.Spatialization;

				auto getTextForModel = [&](AttModel model)
				{
					switch (model)
					{
						case AttModel::None:
							return "None";
						case AttModel::Inverse:
							return "Inverse";
						case AttModel::Linear:
							return "Linear";
						case AttModel::Exponential:
							return "Exponential";
						default:
							return "None";
					}
				};

				const auto& attenModStr = std::vector<std::string>{ getTextForModel(AttModel::None),
																	getTextForModel(AttModel::Inverse),
																	getTextForModel(AttModel::Linear),
																	getTextForModel(AttModel::Exponential) };

				UI::BeginPropertyGrid();

				int32_t selectedModel = static_cast<int32_t>(spatialConfig->AttenuationMod);
				if (UI::PropertyDropdown("Attenuaion Model", attenModStr, (int32_t)attenModStr.size(), &selectedModel))
				{
					spatialConfig->AttenuationMod = static_cast<AttModel>(selectedModel);
				}

				propertyGridSpacing();
				propertyGridSpacing();
				UI::Property("Min Gain", spatialConfig->MinGain, 0.01f, 0.0f, 1.0f);
				UI::Property("Max Gain", spatialConfig->MaxGain, 0.01f, 0.0f, 1.0f);
				UI::Property("Min Distance", spatialConfig->MinDistance, 1.00f, 0.0f, FLT_MAX);
				UI::Property("Max Distance", spatialConfig->MaxDistance, 1.00f, 0.0f, FLT_MAX);

				propertyGridSpacing();
				propertyGridSpacing();

				float inAngle = glm::degrees(spatialConfig->ConeInnerAngleInRadians);
				float outAngle = glm::degrees(spatialConfig->ConeOuterAngleInRadians);
				float outGain = spatialConfig->ConeOuterGain;

				//? Have to manually clamp here because UI::Property doesn't take flags to pass in ImGuiSliderFlags_ClampOnInput
				if (UI::Property("Cone Inner Angle", inAngle, 1.0f, 0.0f, 360.0f))
				{
					if (inAngle > 360.0f) inAngle = 360.0f;
					spatialConfig->ConeInnerAngleInRadians = glm::radians(inAngle);
				}
				if (UI::Property("Cone Outer Angle", outAngle, 1.0f, 0.0f, 360.0f))
				{
					if (outAngle > 360.0f) outAngle = 360.0f;
					spatialConfig->ConeOuterAngleInRadians = glm::radians(outAngle);
				}
				if (UI::Property("Cone Outer Gain", outGain, 0.01f, 0.0f, 1.0f))
				{
					if (outGain > 1.0f) outGain = 1.0f;
					spatialConfig->ConeOuterGain = outGain;
				}

				propertyGridSpacing();
				propertyGridSpacing();
				if (UI::Property("Doppler Factor", spatialConfig->DopplerFactor, 0.01f, 0.0f, 1.0f)) {}
				//if (UI::Property("Rolloff", spatialConfig->Rolloff, 0.01f, 0.0f, 1.0f)) {  }

				propertyGridSpacing();
				propertyGridSpacing();
				// TODO: air absorption filter is not hooked up yet
				//if (UI::Property("Air Absorption", spatialConfig->bAirAbsorptionEnabled)) {  }

				UI::Property("Spread from Source Size", spatialConfig->bSpreadFromSourceSize);
				UI::Property("Source Size", spatialConfig->SourceSize, 0.01f, 0.01f, FLT_MAX);
				UI::Property("Spread", spatialConfig->Spread, 0.01f, 0.0f, 1.0f);
				UI::Property("Focus", spatialConfig->Focus, 0.01f, 0.0f, 1.0f);

				UI::EndPropertyGrid();
				ImGui::TreePop();
			}

			ImGui::Unindent();
		}

		ImGui::EndChild(); // SoundConfigPanel
	}

	PrefabEditor::PrefabEditor()
		: AssetEditor("Prefab Editor"), m_SceneHierarchyPanel(nullptr, SelectionContext::PrefabEditor, false)
	{
	}

	void PrefabEditor::OnOpen()
	{
		SelectionManager::DeselectAll();
	}

	void PrefabEditor::OnClose()
	{
		SelectionManager::DeselectAll(SelectionContext::PrefabEditor);
	}

	void PrefabEditor::Render()
	{
		// Need to do this in order to ensure that the scene hierarchy panel doesn't close immediately.
		// There's been some structural changes since the addition of the PanelManager.
		bool isOpen = true;
		m_SceneHierarchyPanel.OnImGuiRender(isOpen);
	}

}

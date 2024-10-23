#include "pch.h"
#include "AssetSerializer.h"

#include "AssetManager.h"

#include "Engine/Audio/AudioFileUtils.h"
#include "Engine/Audio/SoundBank.h"
#include "Engine/Audio/ResourceManager.h"

#include "Engine/Scene/Prefab.h"
#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Asset/MeshColliderAsset.h"

#include "Engine/Renderer/MaterialAsset.h"
#include "Engine/Renderer/Mesh.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/UI/Font.h"

#include "Engine/Utilities/FileSystem.h"
#include "Engine/Utilities/SerializationMacros.h"
#include "Engine/Utilities/StringUtils.h"
#include "Engine/Utilities/YAMLSerializationHelpers.h"

#include "Engine/Serialization/TextureRuntimeSerializer.h"

#include "yaml-cpp/yaml.h"

namespace Hazel {

	//////////////////////////////////////////////////////////////////////////////////
	// TextureSerializer
	//////////////////////////////////////////////////////////////////////////////////

	bool TextureSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		asset = Texture2D::Create(TextureSpecification(), Project::GetEditorAssetManager()->GetFileSystemPathString(metadata));
		asset->Handle = metadata.Handle;

		bool result = asset.As<Texture2D>()->Loaded();

		if (!result)
			asset->SetFlag(AssetFlag::Invalid, true);

		return result;
	}

	bool TextureSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		outInfo.Offset = stream.GetStreamPosition();

		auto& metadata = Project::GetEditorAssetManager()->GetMetadata(handle);
		Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);
		outInfo.Size = TextureRuntimeSerializer::SerializeTexture2DToFile(texture, stream);
		return true;
	}

	Ref<Asset> TextureSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		ZONG_CORE_WARN("TextureSerializer::DeserializeFromAssetPack");

		stream.SetStreamPosition(assetInfo.PackedOffset);
		return TextureRuntimeSerializer::DeserializeTexture2D(stream);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// FontSerializer
	//////////////////////////////////////////////////////////////////////////////////

	bool FontSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		asset = Ref<Font>::Create(Project::GetEditorAssetManager()->GetFileSystemPathString(metadata));
		asset->Handle = metadata.Handle;

#if 0
		// TODO(Yan): we should probably handle fonts not loading correctly
		bool result = asset.As<Font>()->Loaded();
		if (!result)
			asset->SetFlag(AssetFlag::Invalid, true);
#endif

		return true;
	}

	bool FontSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		outInfo.Offset = stream.GetStreamPosition();

		Ref<Font> font = AssetManager::GetAsset<Font>(handle);
		auto path = Project::GetEditorAssetManager()->GetFileSystemPath(handle);
		stream.WriteString(font->GetName());
		Buffer fontData = FileSystem::ReadBytes(path);
		stream.WriteBuffer(fontData);

		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> FontSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);

		std::string name;
		stream.ReadString(name);
		Buffer fontData;
		stream.ReadBuffer(fontData);

		return Ref<Font>::Create(name, fontData);;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// MaterialAssetSerializer
	//////////////////////////////////////////////////////////////////////////////////

	void MaterialAssetSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<MaterialAsset> materialAsset = asset.As<MaterialAsset>();

		std::string yamlString = SerializeToYAML(materialAsset);

		std::ofstream fout(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		fout << yamlString;
	}

	bool MaterialAssetSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		std::ifstream stream(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		if (!stream.is_open())
			return false;

		std::stringstream strStream;
		strStream << stream.rdbuf();

		Ref<MaterialAsset> materialAsset;
		bool success = DeserializeFromYAML(strStream.str(), materialAsset);
		if (!success)
			return false;

		asset = materialAsset;
		asset->Handle = metadata.Handle;
		return true;
	}

	bool MaterialAssetSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<MaterialAsset> materialAsset = AssetManager::GetAsset<MaterialAsset>(handle);

		std::string yamlString = SerializeToYAML(materialAsset);
		outInfo.Offset = stream.GetStreamPosition();
		stream.WriteString(yamlString);
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> MaterialAssetSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		std::string yamlString;
		stream.ReadString(yamlString);

		Ref<MaterialAsset> materialAsset;
		bool result = DeserializeFromYAML(yamlString, materialAsset);
		if (!result)
			return nullptr;

		return materialAsset;
	}

	std::string MaterialAssetSerializer::SerializeToYAML(Ref<MaterialAsset> materialAsset) const
	{
		YAML::Emitter out;
		out << YAML::BeginMap; // Material
		out << YAML::Key << "Material" << YAML::Value;
		{
			out << YAML::BeginMap;

			// TODO(Yan): this should have shader UUID when that's a thing
			//            right now only supports PBR or Transparent shaders
			Ref<Shader> transparentShader = Renderer::GetShaderLibrary()->Get("HazelPBR_Transparent");
			bool transparent = materialAsset->GetMaterial()->GetShader() == transparentShader;
			ZONG_SERIALIZE_PROPERTY(Transparent, transparent, out);

			ZONG_SERIALIZE_PROPERTY(AlbedoColor, materialAsset->GetAlbedoColor(), out);
			ZONG_SERIALIZE_PROPERTY(Emission, materialAsset->GetEmission(), out);
			if (!transparent)
			{
				ZONG_SERIALIZE_PROPERTY(UseNormalMap, materialAsset->IsUsingNormalMap(), out);
				ZONG_SERIALIZE_PROPERTY(Metalness, materialAsset->GetMetalness(), out);
				ZONG_SERIALIZE_PROPERTY(Roughness, materialAsset->GetRoughness(), out);
			}
			else
			{
				ZONG_SERIALIZE_PROPERTY(Transparency, materialAsset->GetTransparency(), out);
			}

			{
				Ref<Texture2D> albedoMap = materialAsset->GetAlbedoMap();
				bool hasAlbedoMap = albedoMap ? !albedoMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
				AssetHandle albedoMapHandle = hasAlbedoMap ? albedoMap->Handle : UUID(0);
				ZONG_SERIALIZE_PROPERTY(AlbedoMap, albedoMapHandle, out);
			}
			if (!transparent)
			{
				{
					Ref<Texture2D> normalMap = materialAsset->GetNormalMap();
					bool hasNormalMap = normalMap ? !normalMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
					AssetHandle normalMapHandle = hasNormalMap ? normalMap->Handle : UUID(0);
					ZONG_SERIALIZE_PROPERTY(NormalMap, normalMapHandle, out);
				}
				{
					Ref<Texture2D> metalnessMap = materialAsset->GetMetalnessMap();
					bool hasMetalnessMap = metalnessMap ? !metalnessMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
					AssetHandle metalnessMapHandle = hasMetalnessMap ? metalnessMap->Handle : UUID(0);
					ZONG_SERIALIZE_PROPERTY(MetalnessMap, metalnessMapHandle, out);
				}
				{
					Ref<Texture2D> roughnessMap = materialAsset->GetRoughnessMap();
					bool hasRoughnessMap = roughnessMap ? !roughnessMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
					AssetHandle roughnessMapHandle = hasRoughnessMap ? roughnessMap->Handle : UUID(0);
					ZONG_SERIALIZE_PROPERTY(RoughnessMap, roughnessMapHandle, out);
				}
			}

			ZONG_SERIALIZE_PROPERTY(MaterialFlags, materialAsset->GetMaterial()->GetFlags(), out);

			out << YAML::EndMap;
		}
		out << YAML::EndMap; // Material

		return std::string(out.c_str());
	}

	bool MaterialAssetSerializer::DeserializeFromYAML(const std::string& yamlString, Ref<MaterialAsset>& targetMaterialAsset) const
	{
		YAML::Node root = YAML::Load(yamlString);
		YAML::Node materialNode = root["Material"];

		bool transparent = false;
		ZONG_DESERIALIZE_PROPERTY(Transparent, transparent, materialNode, false);

		targetMaterialAsset = Ref<MaterialAsset>::Create(transparent);

		ZONG_DESERIALIZE_PROPERTY(AlbedoColor, targetMaterialAsset->GetAlbedoColor(), materialNode, glm::vec3(0.8f));
		ZONG_DESERIALIZE_PROPERTY(Emission, targetMaterialAsset->GetEmission(), materialNode, 0.0f);

		if (!transparent)
		{
			targetMaterialAsset->SetUseNormalMap(materialNode["UseNormalMap"] ? materialNode["UseNormalMap"].as<bool>() : false);
			ZONG_DESERIALIZE_PROPERTY(Metalness, targetMaterialAsset->GetMetalness(), materialNode, 0.0f);
			ZONG_DESERIALIZE_PROPERTY(Roughness, targetMaterialAsset->GetRoughness(), materialNode, 0.5f);
		}
		else
		{
			ZONG_DESERIALIZE_PROPERTY(Transparency, targetMaterialAsset->GetTransparency(), materialNode, 1.0f);
		}

		AssetHandle albedoMap, normalMap, metalnessMap, roughnessMap;
		ZONG_DESERIALIZE_PROPERTY(AlbedoMap, albedoMap, materialNode, (AssetHandle)0);
		if (!transparent)
		{
			ZONG_DESERIALIZE_PROPERTY(NormalMap, normalMap, materialNode, (AssetHandle)0);
			ZONG_DESERIALIZE_PROPERTY(MetalnessMap, metalnessMap, materialNode, (AssetHandle)0);
			ZONG_DESERIALIZE_PROPERTY(RoughnessMap, roughnessMap, materialNode, (AssetHandle)0);
		}
		if (albedoMap)
		{
			if (AssetManager::IsAssetHandleValid(albedoMap))
				targetMaterialAsset->SetAlbedoMap(AssetManager::GetAsset<Texture2D>(albedoMap));
		}
		if (normalMap)
		{
			if (AssetManager::IsAssetHandleValid(normalMap))
				targetMaterialAsset->SetNormalMap(AssetManager::GetAsset<Texture2D>(normalMap));
		}
		if (metalnessMap)
		{
			if (AssetManager::IsAssetHandleValid(metalnessMap))
				targetMaterialAsset->SetMetalnessMap(AssetManager::GetAsset<Texture2D>(metalnessMap));
		}
		if (roughnessMap)
		{
			if (AssetManager::IsAssetHandleValid(roughnessMap))
				targetMaterialAsset->SetRoughnessMap(AssetManager::GetAsset<Texture2D>(roughnessMap));
		}

		ZONG_DESERIALIZE_PROPERTY(MaterialFlags, roughnessMap, materialNode, (AssetHandle)0);
		if (materialNode["MaterialFlags"])
			targetMaterialAsset->GetMaterial()->SetFlags(materialNode["MaterialFlags"].as<uint32_t>());

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// EnvironmentSerializer
	//////////////////////////////////////////////////////////////////////////////////

	bool EnvironmentSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		auto [radiance, irradiance] = Renderer::CreateEnvironmentMap(Project::GetEditorAssetManager()->GetFileSystemPathString(metadata));

		if (!radiance || !irradiance)
			return false;

		asset = Ref<Environment>::Create(radiance, irradiance);
		asset->Handle = metadata.Handle;
		return true;
	}

	bool EnvironmentSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		outInfo.Offset = stream.GetStreamPosition();

		Ref<Environment> environment = AssetManager::GetAsset<Environment>(handle);
		uint64_t size = TextureRuntimeSerializer::SerializeToFile(environment->RadianceMap, stream);
		size = TextureRuntimeSerializer::SerializeToFile(environment->IrradianceMap, stream);

		// Serialize as just generic TextureCube maybe?
		struct EnvironmentMapMetadata
		{
			uint64_t RadianceMapOffset, RadianceMapSize;
			uint64_t IrradianceMapOffset, IrradianceMapSize;
		};

		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> EnvironmentSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		Ref<TextureCube> radianceMap = TextureRuntimeSerializer::DeserializeTextureCube(stream);
		Ref<TextureCube> irradianceMap = TextureRuntimeSerializer::DeserializeTextureCube(stream);
		return Ref<Environment>::Create(radianceMap, irradianceMap);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// AudioFileSourceSerializer
	//////////////////////////////////////////////////////////////////////////////////

	void AudioFileSourceSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{

	}

	bool AudioFileSourceSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		if (auto opt = AudioFileUtils::GetFileInfo(metadata))
		{
			AudioFileUtils::AudioFileInfo info = opt.value();
			asset = Ref<AudioFile>::Create(info.Duration, info.SamplingRate, info.BitDepth, info.NumChannels, info.FileSize);
		}
		else
			asset = Ref<AudioFile>::Create();

		asset->Handle = metadata.Handle;
		return true;
	}

	bool AudioFileSourceSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		//? JP. Disabled as we don't use filepaths for lookup anymore. No idea if this serialization is till needed for the runtime, maybe in the future.
#if 0 
		outInfo.Offset = stream.GetStreamPosition();

		Ref<AudioFile> audioFile = AssetManager::GetAsset<AudioFile>(handle);
		auto path = Project::GetEditorAssetManager()->GetFileSystemPath(handle);
		auto relativePath = std::filesystem::relative(path, Project::GetAssetDirectory());
		if (relativePath.empty())
			audioFile->FilePath = path.string();
		else
			audioFile->FilePath = relativePath.string();

		stream.WriteString(audioFile->FilePath);

		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
#endif
		return true;
	}

	Ref<Asset> AudioFileSourceSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		//? JP. Disabled as we don't use filepaths for lookup anymore. No idea if this serialization is till needed for the runtime, maybe in the future.
#if 0
		stream.SetStreamPosition(assetInfo.PackedOffset);
		Ref<AudioFile> audioFile = Ref<AudioFile>::Create();
		stream.ReadString(audioFile->FilePath);
#endif
		return Ref<AudioFile>::Create();
	}

	//////////////////////////////////////////////////////////////////////////////////
	// SoundConfigSerializer
	//////////////////////////////////////////////////////////////////////////////////

	void SoundConfigSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<SoundConfig> soundConfig = asset.As<SoundConfig>();

		std::string yamlString = SerializeToYAML(soundConfig);

		std::ofstream fout(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		fout << yamlString;
	}

	bool SoundConfigSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		std::ifstream stream(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		if (!stream.is_open())
			return false;

		std::stringstream strStream;
		strStream << stream.rdbuf();

		Ref<SoundConfig> soundConfig = nullptr;

		// TODO: JP to Yan: Maybe we should make a cleaner intefrace to reuse with other asset types as well?
		// Reference to existing asset may be in use somewhere,
		// we don't want to invalidate it every time we make a change to the asset.
		const bool assetLoaded = Project::GetAssetManager()->IsAssetLoaded(metadata.Handle); //? this check may not be equally correct for Editor and Runtime, or if we change some stuff with asset handling
		if (assetLoaded)
		{
			auto existingAsset = Project::GetAssetManager()->GetAsset(metadata.Handle);
			if (existingAsset && existingAsset->GetAssetType() == AssetType::SoundConfig)
				soundConfig = existingAsset.As<SoundConfig>();
		}
		else
		{
			soundConfig = Ref<SoundConfig>::Create();
		}

		bool success = DeserializeFromYAML(strStream.str(), soundConfig);
		if (!success)
			return false;

		asset = soundConfig;
		asset->Handle = metadata.Handle;
		return true;
	}

	bool SoundConfigSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<SoundConfig> soundConfig = AssetManager::GetAsset<SoundConfig>(handle);

		std::string yamlString = SerializeToYAML(soundConfig);
		outInfo.Offset = stream.GetStreamPosition();
		stream.WriteString(yamlString);
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> SoundConfigSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		std::string yamlString;
		stream.ReadString(yamlString);

		Ref<SoundConfig> soundConfig = Ref<SoundConfig>::Create();
		bool result = DeserializeFromYAML(yamlString, soundConfig);
		if (!result)
			return nullptr;

		return soundConfig;
	}

	std::string SoundConfigSerializer::SerializeToYAML(Ref<SoundConfig> soundConfig) const
	{
		YAML::Emitter out;

		out << YAML::BeginMap; // SoundConfig
		if (soundConfig->DataSourceAsset)
		{
			ZONG_SERIALIZE_PROPERTY(AssetID, soundConfig->DataSourceAsset, out);
		}

		ZONG_SERIALIZE_PROPERTY(IsLooping, (bool)soundConfig->bLooping, out);
		ZONG_SERIALIZE_PROPERTY(VolumeMultiplier, soundConfig->VolumeMultiplier, out);
		ZONG_SERIALIZE_PROPERTY(PitchMultiplier, soundConfig->PitchMultiplier, out);
		ZONG_SERIALIZE_PROPERTY(MasterReverbSend, soundConfig->MasterReverbSend, out);
		ZONG_SERIALIZE_PROPERTY(LPFilterValue, soundConfig->LPFilterValue, out);
		ZONG_SERIALIZE_PROPERTY(HPFilterValue, soundConfig->HPFilterValue, out);

		// TODO: move Spatialization to its own asset type
		out << YAML::Key << "Spatialization";
		out << YAML::BeginMap; // Spatialization

		auto& spatialConfig = soundConfig->Spatialization;
		ZONG_SERIALIZE_PROPERTY(Enabled, soundConfig->bSpatializationEnabled, out);
		ZONG_SERIALIZE_PROPERTY(AttenuationModel, (int)spatialConfig->AttenuationMod, out);
		ZONG_SERIALIZE_PROPERTY(MinGain, spatialConfig->MinGain, out);
		ZONG_SERIALIZE_PROPERTY(MaxGain, spatialConfig->MaxGain, out);
		ZONG_SERIALIZE_PROPERTY(MinDistance, spatialConfig->MinDistance, out);
		ZONG_SERIALIZE_PROPERTY(MaxDistance, spatialConfig->MaxDistance, out);
		ZONG_SERIALIZE_PROPERTY(ConeInnerAngle, spatialConfig->ConeInnerAngleInRadians, out);
		ZONG_SERIALIZE_PROPERTY(ConeOuterAngle, spatialConfig->ConeOuterAngleInRadians, out);
		ZONG_SERIALIZE_PROPERTY(ConeOuterGain, spatialConfig->ConeOuterGain, out);
		ZONG_SERIALIZE_PROPERTY(DopplerFactor, spatialConfig->DopplerFactor, out);
		ZONG_SERIALIZE_PROPERTY(Rollor, spatialConfig->Rolloff, out);
		ZONG_SERIALIZE_PROPERTY(AirAbsorptionEnabled, spatialConfig->bAirAbsorptionEnabled, out);
		ZONG_SERIALIZE_PROPERTY(SpreadFromSourceSize, spatialConfig->bSpreadFromSourceSize, out);
		ZONG_SERIALIZE_PROPERTY(SourceSize, spatialConfig->SourceSize, out);
		ZONG_SERIALIZE_PROPERTY(Spread, spatialConfig->Spread, out);
		ZONG_SERIALIZE_PROPERTY(Focus, spatialConfig->Focus, out);

		out << YAML::EndMap; // Spatialization
		out << YAML::EndMap; // SoundConfig

		return std::string(out.c_str());
	}

	bool SoundConfigSerializer::DeserializeFromYAML(const std::string& yamlString, Ref<SoundConfig> targetSoundConfig) const
	{
		YAML::Node data = YAML::Load(yamlString);

		AssetHandle assetHandle = data["AssetID"] ? data["AssetID"].as<uint64_t>() : 0;

		bool valid = true;
		if (Application::IsRuntime())
		{
			AssetType type = AssetManager::GetAssetType(assetHandle);

			if (type == AssetType::Audio)
			{
				// If actual audio file, check if it exists in SoundBank
				Ref<SoundBank> soundBank = MiniAudioEngine::Get().GetResourceManager()->GetSoundBank();
				ZONG_CORE_VERIFY(soundBank, "SoundBank is not loaded!");

				valid = soundBank->Contains(assetHandle);
			}
		}
		else
		{
			valid = AssetManager::IsAssetHandleValid(assetHandle);
		}

		if (valid)
		{
			targetSoundConfig->DataSourceAsset = assetHandle;
		}
		else
		{
			if (Application::IsRuntime())
			{
				ZONG_CORE_ERROR("Tried to load invalid audio source asset (UUID {0}) in SoundConfig: {1}",
					assetHandle, targetSoundConfig->Handle);
			}
			else
			{
				ZONG_CORE_ERROR("Tried to load invalid audio source asset (UUID {0}) in SoundConfig: {1}",
					assetHandle, Project::GetEditorAssetManager()->GetFileSystemPath(targetSoundConfig->Handle).string());
			}
		}

		ZONG_DESERIALIZE_PROPERTY(IsLooping, targetSoundConfig->bLooping, data, false);
		ZONG_DESERIALIZE_PROPERTY(VolumeMultiplier, targetSoundConfig->VolumeMultiplier, data, 1.0f);
		ZONG_DESERIALIZE_PROPERTY(PitchMultiplier, targetSoundConfig->PitchMultiplier, data, 1.0f);
		ZONG_DESERIALIZE_PROPERTY(MasterReverbSend, targetSoundConfig->MasterReverbSend, data, 0.0f);
		ZONG_DESERIALIZE_PROPERTY(LPFilterValue, targetSoundConfig->LPFilterValue, data, 20000.0f);
		ZONG_DESERIALIZE_PROPERTY(HPFilterValue, targetSoundConfig->HPFilterValue, data, 0.0f);

		auto spConfigData = data["Spatialization"];
		if (spConfigData)
		{
			targetSoundConfig->bSpatializationEnabled = spConfigData["Enabled"] ? spConfigData["Enabled"].as<bool>() : false;

			auto& spatialConfig = targetSoundConfig->Spatialization;

			ZONG_DESERIALIZE_PROPERTY(Enabled, targetSoundConfig->bSpatializationEnabled, spConfigData, false);
			spatialConfig->AttenuationMod = spConfigData["AttenuationModel"] ? static_cast<AttenuationModel>(spConfigData["AttenuationModel"].as<int>())
				: AttenuationModel::Inverse;

			ZONG_DESERIALIZE_PROPERTY(MinGain, spatialConfig->MinGain, spConfigData, 0.0f);
			ZONG_DESERIALIZE_PROPERTY(MaxGain, spatialConfig->MaxGain, spConfigData, 1.0f);
			ZONG_DESERIALIZE_PROPERTY(MinDistance, spatialConfig->MinDistance, spConfigData, 1.0f);
			ZONG_DESERIALIZE_PROPERTY(MaxDistance, spatialConfig->MaxDistance, spConfigData, 1000.0f);
			ZONG_DESERIALIZE_PROPERTY(ConeInnerAngle, spatialConfig->ConeInnerAngleInRadians, spConfigData, 6.283185f);
			ZONG_DESERIALIZE_PROPERTY(ConeOuterAngle, spatialConfig->ConeOuterAngleInRadians, spConfigData, 6.283185f);
			ZONG_DESERIALIZE_PROPERTY(ConeOuterGain, spatialConfig->ConeOuterGain, spConfigData, 0.0f);
			ZONG_DESERIALIZE_PROPERTY(DopplerFactor, spatialConfig->DopplerFactor, spConfigData, 1.0f);
			ZONG_DESERIALIZE_PROPERTY(Rollor, spatialConfig->Rolloff, spConfigData, 1.0f);
			ZONG_DESERIALIZE_PROPERTY(AirAbsorptionEnabled, spatialConfig->bAirAbsorptionEnabled, spConfigData, true);
			ZONG_DESERIALIZE_PROPERTY(SpreadFromSourceSize, spatialConfig->bSpreadFromSourceSize, spConfigData, true);
			ZONG_DESERIALIZE_PROPERTY(SourceSize, spatialConfig->SourceSize, spConfigData, 1.0f);
			ZONG_DESERIALIZE_PROPERTY(Spread, spatialConfig->Spread, spConfigData, 1.0f);
			ZONG_DESERIALIZE_PROPERTY(Focus, spatialConfig->Focus, spConfigData, 1.0f);
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// PrefabSerializer
	//////////////////////////////////////////////////////////////////////////////////

	void PrefabSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<Prefab> prefab = asset.As<Prefab>();

		std::string yamlString = SerializeToYAML(prefab);

		std::ofstream fout(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		fout << yamlString;
	}

	bool PrefabSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		std::ifstream stream(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		if (!stream.is_open())
			return false;

		std::stringstream strStream;
		strStream << stream.rdbuf();

		Ref<Prefab> prefab = Ref<Prefab>::Create();
		bool success = DeserializeFromYAML(strStream.str(), prefab);
		if (!success)
			return false;

		asset = prefab;
		asset->Handle = metadata.Handle;
		return true;
	}

	bool PrefabSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(handle);

		std::string yamlString = SerializeToYAML(prefab);
		outInfo.Offset = stream.GetStreamPosition();
		stream.WriteString(yamlString);
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> PrefabSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		std::string yamlString;
		stream.ReadString(yamlString);

		Ref<Prefab> prefab = Ref<Prefab>::Create();
		bool result = DeserializeFromYAML(yamlString, prefab);
		if (!result)
			return nullptr;

		return prefab;
	}

	std::string PrefabSerializer::SerializeToYAML(Ref<Prefab> prefab) const
	{
		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "Prefab";
		out << YAML::Value << YAML::BeginSeq;

		prefab->m_Scene->m_Registry.each([&](auto entityID)
		{
			Entity entity = { entityID, prefab->m_Scene.Raw() };
			if (!entity || !entity.HasComponent<IDComponent>())
				return;

			SceneSerializer::SerializeEntity(out, entity);
		});

		out << YAML::EndSeq;
		out << YAML::EndMap;

		return std::string(out.c_str());
	}

	bool PrefabSerializer::DeserializeFromYAML(const std::string& yamlString, Ref<Prefab> prefab) const
	{
		YAML::Node data = YAML::Load(yamlString);
		if (!data["Prefab"])
			return false;

		YAML::Node prefabNode = data["Prefab"];
		SceneSerializer::DeserializeEntities(prefabNode, prefab->m_Scene);
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// SceneAssetSerializer
	//////////////////////////////////////////////////////////////////////////////////

	void SceneAssetSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		SceneSerializer serializer(asset.As<Scene>());
		serializer.Serialize(Project::GetEditorAssetManager()->GetFileSystemPath(metadata).string());
	}

	bool SceneAssetSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		asset = Ref<Scene>::Create("SceneAsset", false, false);
		asset->Handle = metadata.Handle;
		return true;
	}

	bool SceneAssetSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<Scene> scene = Ref<Scene>::Create("AssetPackTemp", true, false);
		const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(handle);
		SceneSerializer serializer(scene);
		if (serializer.Deserialize(Project::GetAssetDirectory() / metadata.FilePath))
		{
			return serializer.SerializeToAssetPack(stream, outInfo);
		}
		return false;
	}

	Ref<Asset> SceneAssetSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		ZONG_CORE_VERIFY(false); // Not implemented
		return nullptr;
	}

	Ref<Scene> SceneAssetSerializer::DeserializeSceneFromAssetPack(FileStreamReader& stream, const AssetPackFile::SceneInfo& sceneInfo) const
	{
		Ref<Scene> scene = Ref<Scene>::Create();
		SceneSerializer serializer(scene);
		if (serializer.DeserializeFromAssetPack(stream, sceneInfo))
			return scene;

		return nullptr;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// MeshColliderSerializer
	//////////////////////////////////////////////////////////////////////////////////

	void MeshColliderSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<MeshColliderAsset> meshCollider = asset.As<MeshColliderAsset>();

		std::string yamlString = SerializeToYAML(meshCollider);

		std::ofstream fout(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		fout << yamlString;
	}

	bool MeshColliderSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		std::ifstream stream(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		if (!stream.is_open())
			return false;

		std::stringstream strStream;
		strStream << stream.rdbuf();

		if (strStream.rdbuf()->in_avail() == 0)
			return false;

		Ref<MeshColliderAsset> meshCollider = Ref<MeshColliderAsset>::Create();
		bool result = DeserializeFromYAML(strStream.str(), meshCollider);
		if (!result)
			return false;

		asset = meshCollider;
		asset->Handle = metadata.Handle;
		return true;
	}

	bool MeshColliderSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<MeshColliderAsset> meshCollider = AssetManager::GetAsset<MeshColliderAsset>(handle);

		std::string yamlString = SerializeToYAML(meshCollider);
		outInfo.Offset = stream.GetStreamPosition();
		stream.WriteString(yamlString);
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> MeshColliderSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		std::string yamlString;
		stream.ReadString(yamlString);

		Ref<MeshColliderAsset> meshCollider = Ref<MeshColliderAsset>::Create();
		bool result = DeserializeFromYAML(yamlString, meshCollider);
		if (!result)
			return nullptr;

		return meshCollider;
	}

	std::string MeshColliderSerializer::SerializeToYAML(Ref<MeshColliderAsset> meshCollider) const
	{
		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "ColliderMesh" << YAML::Value << meshCollider->ColliderMesh;
		out << YAML::Key << "EnableVertexWelding" << YAML::Value << meshCollider->EnableVertexWelding;
		out << YAML::Key << "VertexWeldTolerance" << YAML::Value << meshCollider->VertexWeldTolerance;
		out << YAML::Key << "FlipNormals" << YAML::Value << meshCollider->FlipNormals;
		out << YAML::Key << "CheckZeroAreaTriangles" << YAML::Value << meshCollider->CheckZeroAreaTriangles;
		out << YAML::Key << "AreaTestEpsilon" << YAML::Value << meshCollider->AreaTestEpsilon;
		out << YAML::Key << "ShiftVerticesToOrigin" << YAML::Value << meshCollider->ShiftVerticesToOrigin;
		out << YAML::Key << "AlwaysShareShape" << YAML::Value << meshCollider->AlwaysShareShape;
		out << YAML::Key << "CollisionComplexity" << YAML::Value << (uint8_t)meshCollider->CollisionComplexity;
		out << YAML::Key << "ColliderScale" << YAML::Value << meshCollider->ColliderScale;
		out << YAML::Key << "PreviewScale" << YAML::Value << meshCollider->PreviewScale;

		out << YAML::Key << "ColliderMaterial" << YAML::BeginMap;
		out << YAML::Key << "Friction" << YAML::Value << meshCollider->Material.Friction;
		out << YAML::Key << "Restitution" << YAML::Value << meshCollider->Material.Restitution;
		out << YAML::EndMap;

		out << YAML::EndMap;

		return std::string(out.c_str());
	}

	bool MeshColliderSerializer::DeserializeFromYAML(const std::string& yamlString, Ref<MeshColliderAsset> targetMeshCollider) const
	{
		YAML::Node data = YAML::Load(yamlString);

		targetMeshCollider->ColliderMesh = data["ColliderMesh"].as<AssetHandle>(0);
		targetMeshCollider->Material.Friction = data["ColliderMaterial"]["Friction"].as<float>(0.1f);
		targetMeshCollider->Material.Restitution = data["ColliderMaterial"]["Restitution"].as<float>(0.05f);
		targetMeshCollider->EnableVertexWelding = data["EnableVertexWelding"].as<bool>(true);
		targetMeshCollider->VertexWeldTolerance = glm::clamp<float>(data["VertexWeldTolerance"].as<float>(0.1f), 0.05f, 1.0f);
		targetMeshCollider->FlipNormals = data["FlipNormals"].as<bool>(false);
		targetMeshCollider->CheckZeroAreaTriangles = data["CheckZeroAreaTriangles"].as<bool>(false);
		targetMeshCollider->AreaTestEpsilon = glm::max(0.06f, data["AreaTestEpsilon"].as<float>(0.06f));
		targetMeshCollider->ShiftVerticesToOrigin = data["ShiftVerticesToOrigin"].as<bool>(false);
		targetMeshCollider->AlwaysShareShape = data["AlwaysShareShape"].as<bool>(false);
		targetMeshCollider->CollisionComplexity = (ECollisionComplexity)data["CollisionComplexity"].as<uint8_t>(0);
		targetMeshCollider->ColliderScale = data["ColliderScale"].as<glm::vec3>(glm::vec3(1.0f));
		targetMeshCollider->PreviewScale = data["PreviewScale"].as<glm::vec3>(glm::vec3(1.0f));

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// ScriptFileSerializer
	//////////////////////////////////////////////////////////////////////////////////

	void ScriptFileSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		std::ofstream stream(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		ZONG_CORE_VERIFY(stream.is_open());

		std::ifstream templateStream("Resources/Templates/NewClassTemplate.cs");
		ZONG_CORE_VERIFY(templateStream.is_open());

		std::stringstream templateStrStream;
		templateStrStream << templateStream.rdbuf();
		std::string templateString = templateStrStream.str();

		templateStream.close();

		auto replaceTemplateToken = [&templateString](const char* token, const std::string& value)
		{
			size_t pos = 0;
			while ((pos = templateString.find(token, pos)) != std::string::npos)
			{
				templateString.replace(pos, strlen(token), value);
				pos += strlen(token);
			}
		};

		auto scriptFileAsset = asset.As<ScriptFileAsset>();
		replaceTemplateToken("$NAMESPACE_NAME$", scriptFileAsset->GetClassNamespace());
		replaceTemplateToken("$CLASS_NAME$", scriptFileAsset->GetClassName());

		stream << templateString;
		stream.close();
	}

	bool ScriptFileSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		asset = Ref<ScriptFileAsset>::Create();
		asset->Handle = metadata.Handle;
		return true;
	}

	bool ScriptFileSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		ZONG_CORE_VERIFY(false); // Not implemented

		outInfo.Offset = stream.GetStreamPosition();

		// Write 64 FFs (dummy data)
		for (uint32_t i = 0; i < 16; i++)
			stream.WriteRaw<uint32_t>(0xffffffff);

		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> ScriptFileSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		ZONG_CORE_VERIFY(false); // Not implemented
		return nullptr;
	}

}

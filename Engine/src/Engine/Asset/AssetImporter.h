#pragma once

#include "AssetSerializer.h"

#include "Engine/Serialization/FileStream.h"
#include "Engine/Scene/Scene.h"

namespace Engine {

	class AssetImporter
	{
	public:
		static void Init();
		static void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset);
		static void Serialize(const Ref<Asset>& asset);
		static bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset);
		
		static bool SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo);
		static Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo);
		static Ref<Scene> DeserializeSceneFromAssetPack(FileStreamReader& stream, const AssetPackFile::SceneInfo& assetInfo);
	private:
		static std::unordered_map<AssetType, Scope<AssetSerializer>> s_Serializers;
	};

}

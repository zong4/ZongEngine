#pragma once

#include "Engine/Asset/Asset.h"
#include "Engine/Asset/AssetSerializer.h"

#include "Engine/Serialization/AssetPack.h"
#include "Engine/Serialization/AssetPackFile.h"
#include "Engine/Serialization/FileStream.h"

namespace Engine {

	class MeshRuntimeSerializer
	{
	public:
		bool SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo);
		Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo);
	};

}

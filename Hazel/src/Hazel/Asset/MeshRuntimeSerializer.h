#pragma once

#include "Hazel/Asset/Asset.h"
#include "Hazel/Asset/AssetSerializer.h"

#include "Hazel/Serialization/AssetPack.h"
#include "Hazel/Serialization/AssetPackFile.h"
#include "Hazel/Serialization/FileStream.h"

namespace Hazel {

	class MeshRuntimeSerializer
	{
	public:
		bool SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo);
		Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo);
	};

}

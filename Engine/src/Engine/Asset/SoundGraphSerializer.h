#pragma once

#include "Engine/Asset/AssetSerializer.h"

namespace Hazel {

	class SoundGraphAsset;
	class SoundGraphCache;

	class SoundGraphGraphSerializer : public AssetSerializer
	{
	public:
		SoundGraphGraphSerializer();
		~SoundGraphGraphSerializer();

		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;

		virtual bool SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const;
		virtual Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const;
	private:
		std::string SerializeToYAML(Ref<SoundGraphAsset> soundGraphAsset) const;
		bool DeserializeFromYAML(const std::string& yamlString, Ref<SoundGraphAsset>& soundGraphAsset) const;
	private:
		Scope<SoundGraphCache> m_Cache;
	};

} // namespace Hazel

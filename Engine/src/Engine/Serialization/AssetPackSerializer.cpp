#include "hzpch.h"
#include "AssetPackSerializer.h"
#include "Engine/Asset/AssetImporter.h"

#include "Engine/Serialization/FileStream.h"

#include <filesystem>
#include <fstream>
#include <unordered_set>

namespace Hazel {

	static void CreateDirectoriesIfNeeded(const std::filesystem::path& path)
	{
		std::filesystem::path directory = path.parent_path();
		if (!std::filesystem::exists(directory))
			std::filesystem::create_directories(directory);
	}

	void AssetPackSerializer::Serialize(const std::filesystem::path& path, AssetPackFile& file, Buffer appBinary, std::atomic<float>& progress)
	{
		// Print Info
		HZ_CORE_TRACE("Serializing AssetPack to {}", path.string());
		HZ_CORE_TRACE("  {} scenes", file.Index.Scenes.size());
		uint32_t assetCount = 0;
		for (const auto& [sceneHandle, sceneInfo] : file.Index.Scenes)
			assetCount += uint32_t(sceneInfo.Assets.size());
		HZ_CORE_TRACE("  {} assets (including duplicates)", assetCount);

		FileStreamWriter serializer(path);

		serializer.WriteRaw<AssetPackFile::FileHeader>(file.Header);

		// ===============
		// Write index
		// ===============
		// Write dummy data for index (come back later to fill in)
		uint64_t indexPos = serializer.GetStreamPosition();
		uint64_t indexTableSize = CalculateIndexTableSize(file);
		serializer.WriteZero(indexTableSize);

		std::unordered_map<AssetHandle, AssetSerializationInfo> serializedAssets;

		float progressIncrement = 0.4f / (float)file.Index.Scenes.size();

		// Write app binary data
		file.Index.PackedAppBinaryOffset = serializer.GetStreamPosition();
		serializer.WriteBuffer(appBinary);
		file.Index.PackedAppBinarySize = serializer.GetStreamPosition() - file.Index.PackedAppBinaryOffset;
		appBinary.Release();

		// Write asset data + fill in offset + size
		for (auto& [sceneHandle, sceneInfo] : file.Index.Scenes)
		{
			// Serialize Scene
			AssetSerializationInfo serializationInfo;
			AssetImporter::SerializeToAssetPack(sceneHandle, serializer, serializationInfo);
			file.Index.Scenes[sceneHandle].PackedOffset = serializationInfo.Offset;
			file.Index.Scenes[sceneHandle].PackedSize = serializationInfo.Size;

			// Serialize Assets
			for (auto& [assetHandle, assetInfo] : sceneInfo.Assets)
			{
				if (serializedAssets.find(assetHandle) != serializedAssets.end())
				{
					// Has already been serialized
					serializationInfo = serializedAssets.at(assetHandle);
					file.Index.Scenes[sceneHandle].Assets[assetHandle].PackedOffset = serializationInfo.Offset;
					file.Index.Scenes[sceneHandle].Assets[assetHandle].PackedSize = serializationInfo.Size;
				}
				else
				{
					// Serialize asset
					AssetImporter::SerializeToAssetPack(assetHandle, serializer, serializationInfo);
					file.Index.Scenes[sceneHandle].Assets[assetHandle].PackedOffset = serializationInfo.Offset;
					file.Index.Scenes[sceneHandle].Assets[assetHandle].PackedSize = serializationInfo.Size;
					serializedAssets[assetHandle] = serializationInfo;
				}
			}

			progress = progress + progressIncrement;
		}

		HZ_CORE_TRACE("Serialized {} assets into AssetPack", serializedAssets.size());

		serializer.SetStreamPosition(indexPos);
		serializer.WriteRaw<uint64_t>(file.Index.PackedAppBinaryOffset);
		serializer.WriteRaw<uint64_t>(file.Index.PackedAppBinarySize);

		uint64_t begin = indexPos;
		serializer.WriteRaw<uint32_t>((uint32_t)file.Index.Scenes.size()); // Write scene map size
		for (auto& [sceneHandle, sceneInfo] : file.Index.Scenes)
		{
			serializer.WriteRaw<uint64_t>(sceneHandle);
			serializer.WriteRaw<uint64_t>(sceneInfo.PackedOffset);
			serializer.WriteRaw<uint64_t>(sceneInfo.PackedSize);
			serializer.WriteRaw<uint16_t>(sceneInfo.Flags);

			serializer.WriteMap(file.Index.Scenes[sceneHandle].Assets);
		}

		progress = progress + 0.1f;
	}

	bool AssetPackSerializer::DeserializeIndex(const std::filesystem::path& path, AssetPackFile& file)
	{
		// Print Info
		HZ_CORE_TRACE("Deserializing AssetPack from {}", path.string());

		FileStreamReader stream(path);
		if (!stream.IsStreamGood())
			return false;

		stream.ReadRaw<AssetPackFile::FileHeader>(file.Header);
		bool validHeader = memcmp(file.Header.HEADER, "HZAP", 4) == 0;
		HZ_CORE_ASSERT(validHeader);
		if (!validHeader)
			return false;

		// Read app binary info
		stream.ReadRaw<uint64_t>(file.Index.PackedAppBinaryOffset);
		stream.ReadRaw<uint64_t>(file.Index.PackedAppBinarySize);

		uint32_t sceneCount = 0;
		stream.ReadRaw<uint32_t>(sceneCount); // Read scene map size
		for (uint32_t i = 0; i < sceneCount; i++)
		{
			uint64_t sceneHandle = 0;
			stream.ReadRaw<uint64_t>(sceneHandle);

			AssetPackFile::SceneInfo& sceneInfo = file.Index.Scenes[sceneHandle];
			stream.ReadRaw<uint64_t>(sceneInfo.PackedOffset);
			stream.ReadRaw<uint64_t>(sceneInfo.PackedSize);
			stream.ReadRaw<uint16_t>(sceneInfo.Flags);

			stream.ReadMap(sceneInfo.Assets);
		}

		HZ_CORE_TRACE("Deserialized index with {} scenes from AssetPack", sceneCount);
		return true;
	}

	uint64_t AssetPackSerializer::CalculateIndexTableSize(const AssetPackFile& file)
	{
		uint64_t appInfoSize = sizeof(uint64_t) * 2;
		uint64_t sceneMapSize = sizeof(uint32_t) + (sizeof(AssetHandle) + sizeof(uint64_t) * 2 + sizeof(uint16_t)) * file.Index.Scenes.size();
		uint64_t assetMapSize = 0;
		for (const auto& [sceneHandle, sceneInfo] : file.Index.Scenes)
			assetMapSize += sizeof(uint32_t) + (sizeof(AssetHandle) + sizeof(AssetPackFile::AssetInfo)) * sceneInfo.Assets.size();

		return appInfoSize + sceneMapSize + assetMapSize;
	}

}

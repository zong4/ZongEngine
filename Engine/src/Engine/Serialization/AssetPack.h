#pragma once

#include <filesystem>
#include <map>

#include "Engine/Core/UUID.h"
#include "Engine/Asset/Asset.h"
#include "Engine/Scene/Scene.h"

#include "AssetPackSerializer.h"
#include "AssetPackFile.h"

namespace Engine {

	class AssetPack : public RefCounted
	{		
	public:
		AssetPack() = default;
		AssetPack(const std::filesystem::path& path);

		void AddAsset(Ref<Asset> asset);

		void Serialize();

		Ref<Scene> LoadScene(AssetHandle sceneHandle);
		Ref<Asset> LoadAsset(AssetHandle sceneHandle, AssetHandle assetHandle);
		
		bool IsAssetHandleValid(AssetHandle assetHandle) const;
		bool IsAssetHandleValid(AssetHandle sceneHandle, AssetHandle assetHandle) const;

		Buffer ReadAppBinary();
		uint64_t GetBuildVersion();

		// This will create a complete asset pack from ALL referenced assets
		// in currently active project. This should change in the future to
		// take in a Ref<Project> or something when the AssetManager becomes
		// non-static, but since it is static at the moment this is what we get
		static Ref<AssetPack> CreateFromActiveProject(std::atomic<float>& progress);
		static Ref<AssetPack> Load(const std::filesystem::path& path);
		static Ref<AssetPack> LoadActiveProject();
	private:
		std::filesystem::path m_Path;
		AssetPackFile m_File;

		AssetPackSerializer m_Serializer;

		std::unordered_set<AssetHandle> m_AssetHandleIndex;
	};

}

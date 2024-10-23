#pragma once

#include "Engine/Core/Assert.h"

namespace Hazel {

	enum class AssetFlag : uint16_t
	{
		None = 0,
		Missing = BIT(0),
		Invalid = BIT(1)
	};

	enum class AssetType : uint16_t
	{
		None = 0,
		Scene,
		Prefab,
		Mesh,
		StaticMesh,
		MeshSource,
		Material,
		Texture,
		EnvMap,
		Audio,
		SoundConfig,
		SpatializationConfig,
		Font,
		Script,
		ScriptFile,
		MeshCollider,
		SoundGraphSound,
		Skeleton,
		Animation,
		AnimationGraph
	};

	namespace Utils {

		inline AssetType AssetTypeFromString(std::string_view assetType)
		{
			if (assetType == "None")                return AssetType::None;
			if (assetType == "Scene")               return AssetType::Scene;
			if (assetType == "Prefab")              return AssetType::Prefab;
			if (assetType == "Mesh")                return AssetType::Mesh;
			if (assetType == "StaticMesh")          return AssetType::StaticMesh;
			if (assetType == "MeshAsset")           return AssetType::MeshSource; // DEPRECATED
			if (assetType == "MeshSource")          return AssetType::MeshSource;
			if (assetType == "Material")            return AssetType::Material;
			if (assetType == "Texture")             return AssetType::Texture;
			if (assetType == "EnvMap")              return AssetType::EnvMap;
			if (assetType == "Audio")               return AssetType::Audio;
			if (assetType == "SoundConfig")         return AssetType::SoundConfig;
			if (assetType == "Font")                return AssetType::Font;
			if (assetType == "Script")              return AssetType::Script;
			if (assetType == "ScriptFile")          return AssetType::ScriptFile;
			if (assetType == "MeshCollider")        return AssetType::MeshCollider;
			if (assetType == "SoundGraphSound")     return AssetType::SoundGraphSound;
			if (assetType == "Skeleton")            return AssetType::Skeleton;
			if (assetType == "Animation")           return AssetType::Animation;
			//if (assetType == "AnimationController") return AssetType::AnimationController; // OBSOLETE. You need to re-import animated asset
			if (assetType == "AnimationGraph")      return AssetType::AnimationGraph;

			return AssetType::None;
		}

		inline const char* AssetTypeToString(AssetType assetType)
		{
			switch (assetType)
			{
				case AssetType::None:                return "None";
				case AssetType::Scene:               return "Scene";
				case AssetType::Prefab:              return "Prefab";
				case AssetType::Mesh:                return "Mesh";
				case AssetType::StaticMesh:          return "StaticMesh";
				case AssetType::MeshSource:          return "MeshSource";
				case AssetType::Material:            return "Material";
				case AssetType::Texture:             return "Texture";
				case AssetType::EnvMap:              return "EnvMap";
				case AssetType::Audio:               return "Audio";
				case AssetType::SoundConfig:         return "SoundConfig";
				case AssetType::Font:                return "Font";
				case AssetType::Script:              return "Script";
				case AssetType::ScriptFile:          return "ScriptFile";
				case AssetType::MeshCollider:        return "MeshCollider";
				case AssetType::SoundGraphSound:     return "SoundGraphSound";
				case AssetType::Skeleton:            return "Skeleton";
				case AssetType::Animation:           return "Animation";
				case AssetType::AnimationGraph:      return "AnimationGraph";
			}

			HZ_CORE_ASSERT(false, "Unknown Asset Type");
			return "None";
		}

	}
}

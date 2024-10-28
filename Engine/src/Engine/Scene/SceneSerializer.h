#pragma once

#include "Scene.h"

#include "Engine/Serialization/FileStream.h"
#include "Engine/Asset/AssetSerializer.h"

namespace YAML {
	class Emitter;
	class Node;
}

namespace Engine {

	class SceneSerializer
	{
	public:
		SceneSerializer(const Ref<Scene>& scene);

		void Serialize(const std::filesystem::path& filepath);
		void SerializeToYAML(YAML::Emitter& out);
		bool DeserializeFromYAML(const std::string& yamlString);
		void SerializeRuntime(AssetHandle scene);

		bool Deserialize(const std::filesystem::path& filepath);
		bool DeserializeRuntime(AssetHandle scene);

		bool SerializeToAssetPack(FileStreamWriter& stream, AssetSerializationInfo& outInfo);
		bool DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::SceneInfo& sceneInfo);

		bool DeserializeReferencedPrefabs(const std::filesystem::path& filepath, std::unordered_set<AssetHandle>& outPrefabs);
	public:
		static void SerializeEntity(YAML::Emitter& out, Entity entity);
		static void DeserializeEntities(YAML::Node& entitiesNode, Ref<Scene> scene);
	public:
		inline static std::string_view FileFilter = "Engine Scene (*.hscene)\0*.hscene\0";
		inline static std::string_view DefaultExtension = ".hscene";

	private:
		Ref<Scene> m_Scene;
	};

}

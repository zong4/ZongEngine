#include "hzpch.h"
#include "MeshSerializer.h"

#include "yaml-cpp/yaml.h"

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Project/Project.h"

#include "AssimpMeshImporter.h"
#include "MeshRuntimeSerializer.h"

#include "Hazel/Debug/Profiler.h"

namespace YAML {

	template<>
	struct convert<std::vector<uint32_t>>
	{
		static Node encode(const std::vector<uint32_t>& value)
		{
			Node node;
			for (uint32_t element : value)
				node.push_back(element);
			return node;
		}

		static bool decode(const Node& node, std::vector<uint32_t>& result)
		{
			if (!node.IsSequence())
				return false;

			result.resize(node.size());
			for (size_t i = 0; i < node.size(); i++)
				result[i] = node[i].as<uint32_t>();

			return true;
		}
	};

}

namespace Hazel {

	YAML::Emitter& operator<<(YAML::Emitter& out, const std::vector<uint32_t>& value)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq;
		for (uint32_t element : value)
			out << element;
		out << YAML::EndSeq;
		return out;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// MeshSourceSerializer
	//////////////////////////////////////////////////////////////////////////////////

	bool MeshSourceSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		HZ_PROFILE_FUNC("MeshSourceSerializer::TryLoadData");

		AssimpMeshImporter importer(Project::GetEditorAssetManager()->GetFileSystemPathString(metadata));
		Ref<MeshSource> meshSource = importer.ImportToMeshSource();
		if (!meshSource)
			return false;

		asset = meshSource;
		asset->Handle = metadata.Handle;
		return true;
	}

	bool MeshSourceSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		MeshRuntimeSerializer serializer;
		serializer.SerializeToAssetPack(handle, stream, outInfo);
		return true;
	}

	Ref<Asset> MeshSourceSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		MeshRuntimeSerializer serializer;
		return serializer.DeserializeFromAssetPack(stream, assetInfo);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// MeshSerializer
	//////////////////////////////////////////////////////////////////////////////////

	void MeshSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<Mesh> mesh = asset.As<Mesh>();

		std::string yamlString = SerializeToYAML(mesh);

		std::filesystem::path serializePath = Project::GetActive()->GetAssetDirectory() / metadata.FilePath;
		HZ_CORE_WARN("Serializing to {0}", serializePath.string());
		std::ofstream fout(serializePath);

		if (!fout.is_open())
		{
			HZ_CORE_ERROR("Failed to serialize mesh to file '{0}'", serializePath);
//			HZ_CORE_ASSERT(false, "GetLastError() = {0}", GetLastError());
			return;
		}

		fout << yamlString;
		fout.flush();
		fout.close();
	}

	bool MeshSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		auto filepath = Project::GetActiveAssetDirectory() / metadata.FilePath;
		std::ifstream stream(filepath);
		HZ_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		Ref<Mesh> mesh;
		bool success = DeserializeFromYAML(strStream.str(), mesh);
		if (!success)
			return false;

		mesh->Handle = metadata.Handle;
		AssetManager::RegisterDependency(mesh->GetMeshSource(), mesh->Handle);
		asset = mesh;
		return true;
	}

	bool MeshSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(handle);

		std::string yamlString = SerializeToYAML(mesh);
		outInfo.Offset = stream.GetStreamPosition();
		stream.WriteString(yamlString);
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> MeshSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		std::string yamlString;
		stream.ReadString(yamlString);

		Ref<Mesh> mesh;
		bool result = DeserializeFromYAML(yamlString, mesh);
		if (!result)
			return nullptr;

		return mesh;
	}

	std::string MeshSerializer::SerializeToYAML(Ref<Mesh> mesh) const
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Mesh";
		{
			out << YAML::BeginMap;
			out << YAML::Key << "MeshSource";
			out << YAML::Value << mesh->GetMeshSource();
			out << YAML::Key << "SubmeshIndices";
			out << YAML::Flow;
			if (auto meshSource = AssetManager::GetAsset<MeshSource>(mesh->GetMeshSource()); meshSource && meshSource->GetSubmeshes().size() == mesh->GetSubmeshes().size())
				out << YAML::Value << std::vector<uint32_t>();
			else
				out << YAML::Value << mesh->GetSubmeshes();
			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		return std::string(out.c_str());
	}

	bool MeshSerializer::DeserializeFromYAML(const std::string& yamlString, Ref<Mesh>& targetMesh) const
	{
		YAML::Node data = YAML::Load(yamlString);
		if (!data["Mesh"])
			return false;

		YAML::Node rootNode = data["Mesh"];
		if (!rootNode["MeshAsset"] && !rootNode["MeshSource"])
			return false;

		AssetHandle meshSource = 0;
		if (rootNode["MeshAsset"]) // DEPRECATED
			meshSource = rootNode["MeshAsset"].as<uint64_t>();
		else
			meshSource = rootNode["MeshSource"].as<uint64_t>();

		if(!AssetManager::GetAsset<MeshSource>(meshSource))
			return false; // TODO(Yan): feedback to the user

		auto submeshIndices = rootNode["SubmeshIndices"].as<std::vector<uint32_t>>();
		auto generateColliders = rootNode["GenerateColliders"].as<bool>(false);

		targetMesh = Ref<Mesh>::Create(meshSource, submeshIndices, generateColliders);
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// StaticMeshSerializer
	//////////////////////////////////////////////////////////////////////////////////

	void StaticMeshSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<StaticMesh> staticMesh = asset.As<StaticMesh>();

		std::string yamlString = SerializeToYAML(staticMesh);

		auto serializePath = Project::GetActive()->GetAssetDirectory() / metadata.FilePath;
		std::ofstream fout(serializePath);
		HZ_CORE_ASSERT(fout.good());
		if (fout.good())
			fout << yamlString;
	}

	bool StaticMeshSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		auto filepath = Project::GetActiveAssetDirectory() / metadata.FilePath;
		std::ifstream stream(filepath);
		HZ_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		Ref<StaticMesh> staticMesh;
		bool success = DeserializeFromYAML(strStream.str(), staticMesh);
		if (!success)
			return false;

		staticMesh->Handle = metadata.Handle;
		AssetManager::RegisterDependency(staticMesh->GetMeshSource(), staticMesh->Handle);
		asset = staticMesh;
		return true;
	}

	bool StaticMeshSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<StaticMesh> staticMesh = AssetManager::GetAsset<StaticMesh>(handle);

		std::string yamlString = SerializeToYAML(staticMesh);
		outInfo.Offset = stream.GetStreamPosition();
		stream.WriteString(yamlString);
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> StaticMeshSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		std::string yamlString;
		stream.ReadString(yamlString);

		Ref<StaticMesh> staticMesh;
		bool result = DeserializeFromYAML(yamlString, staticMesh);
		if (!result)
			return nullptr;

		return staticMesh;
	}

	std::string StaticMeshSerializer::SerializeToYAML(Ref<StaticMesh> staticMesh) const
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Mesh";
		{
			out << YAML::BeginMap;
			out << YAML::Key << "MeshSource";
			out << YAML::Value << staticMesh->GetMeshSource();
			out << YAML::Key << "SubmeshIndices";
			out << YAML::Flow << YAML::Value << staticMesh->GetSubmeshes();
			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		return std::string(out.c_str());
	}

	bool StaticMeshSerializer::DeserializeFromYAML(const std::string& yamlString, Ref<StaticMesh>& targetStaticMesh) const
	{
		YAML::Node data = YAML::Load(yamlString);
		if (!data["Mesh"])
			return false;

		YAML::Node rootNode = data["Mesh"];
		if (!rootNode["MeshSource"] && !rootNode["MeshAsset"])
			return false;

		AssetHandle meshSource = rootNode["MeshSource"].as<uint64_t>();
		auto submeshIndices = rootNode["SubmeshIndices"].as<std::vector<uint32_t>>();
		auto generateColliders = rootNode["GenerateColliders"].as<bool>(true);

		targetStaticMesh = Ref<StaticMesh>::Create(meshSource, submeshIndices, generateColliders);
		return true;
	}

}

#include "pch.h"
#include "MeshCookingFactory.h"
#include "PhysicsAPI.h"
#include "Engine/Asset/AssetManager.h"

#include <glm/gtc/type_ptr.hpp>

namespace Engine {

	std::pair<ECookingResult, ECookingResult> MeshCookingFactory::CookMesh(AssetHandle colliderHandle, bool invalidateOld /*= false*/)
	{
		return CookMesh(AssetManager::GetAsset<MeshColliderAsset>(colliderHandle), invalidateOld);
	}

	bool MeshCookingFactory::SerializeMeshCollider(const std::filesystem::path& filepath, MeshColliderData& meshData)
	{
		EnginePhysicsMesh hmc;
		hmc.Type = meshData.Type;
		hmc.SubmeshCount = (uint32_t)meshData.Submeshes.size();

		std::ofstream stream(filepath, std::ios::binary | std::ios::trunc);
		if (!stream)
		{
			stream.close();
			ZONG_CORE_ERROR("Failed to write collider to {0}", filepath.string());
			for (auto& submesh : meshData.Submeshes)
				submesh.ColliderData.Release();
			meshData.Submeshes.clear();
			return false;
		}

		stream.write((char*)&hmc, sizeof(EnginePhysicsMesh));
		for (auto& submesh : meshData.Submeshes)
		{
			stream.write((char*)glm::value_ptr(submesh.Transform), sizeof(submesh.Transform));
			stream.write((char*)&submesh.ColliderData.Size, sizeof(submesh.ColliderData.Size));
			stream.write((char*)submesh.ColliderData.Data, submesh.ColliderData.Size);
		}

		return true;
	}

	MeshColliderData MeshCookingFactory::DeserializeMeshCollider(const std::filesystem::path& filepath)
	{
		// Deserialize
		Buffer colliderBuffer = FileSystem::ReadBytes(filepath);
		EnginePhysicsMesh& hmc = *(EnginePhysicsMesh*)colliderBuffer.Data;
		ZONG_CORE_VERIFY(strcmp(hmc.Header, EnginePhysicsMesh().Header) == 0);

		MeshColliderData meshData;
		meshData.Type = hmc.Type;
		meshData.Submeshes.resize(hmc.SubmeshCount);

		uint8_t* buffer = colliderBuffer.As<uint8_t>() + sizeof(EnginePhysicsMesh);
		for (uint32_t i = 0; i < hmc.SubmeshCount; i++)
		{
			SubmeshColliderData& submeshData = meshData.Submeshes[i];

			// Transform
			memcpy(glm::value_ptr(submeshData.Transform), buffer, sizeof(glm::mat4));
			buffer += sizeof(glm::mat4);

			// Data
			auto size = submeshData.ColliderData.Size = *(uint64_t*)buffer;
			buffer += sizeof(size);
			submeshData.ColliderData = Buffer::Copy(buffer, size);
			buffer += size;
		}

		colliderBuffer.Release();
		return meshData;
	}

}

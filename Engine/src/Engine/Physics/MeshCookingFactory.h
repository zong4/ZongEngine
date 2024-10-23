#pragma once

#include "Engine/Core/Base.h"
#include "Engine/Renderer/Mesh.h"
#include "Engine/Asset/MeshColliderAsset.h"
#include "PhysicsTypes.h"

namespace Hazel {

	enum class MeshColliderType : uint8_t { Triangle = 0, Convex = 1, None = 3 };

	struct SubmeshColliderData
	{
		Buffer ColliderData;
		glm::mat4 Transform;
	};

	struct MeshColliderData
	{
		std::vector<SubmeshColliderData> Submeshes;
		MeshColliderType Type;
	};

	struct CachedColliderData
	{
		MeshColliderData SimpleColliderData;
		MeshColliderData ComplexColliderData;
	};

	// .hmc file format
	struct HazelPhysicsMesh
	{
		const char Header[8] = "HazelMC";
		MeshColliderType Type;
		uint32_t SubmeshCount;
	};

	class MeshCookingFactory : public RefCounted
	{
	public:
		virtual ~MeshCookingFactory() = default;

		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual std::pair<ECookingResult, ECookingResult> CookMesh(Ref<MeshColliderAsset> colliderAsset, bool invalidateOld = false) = 0;
		std::pair<ECookingResult, ECookingResult> CookMesh(AssetHandle colliderHandle, bool invalidateOld = false);

	protected:
		bool SerializeMeshCollider(const std::filesystem::path& filepath, MeshColliderData& meshData);
		MeshColliderData DeserializeMeshCollider(const std::filesystem::path& filepath);
	};

}

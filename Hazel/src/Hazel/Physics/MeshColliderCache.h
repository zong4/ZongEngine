#pragma once

#include "Hazel/Asset/Asset.h"
#include "MeshCookingFactory.h"

#include <unordered_map>

namespace Hazel {

	class MeshColliderCache
	{
	public:
		void Init();

		const CachedColliderData& GetMeshData(const Ref<MeshColliderAsset>& colliderAsset);
		Ref<Mesh> GetDebugMesh(const Ref<MeshColliderAsset>& colliderAsset);
		Ref<StaticMesh> GetDebugStaticMesh(const Ref<MeshColliderAsset>& colliderAsset);

		AssetHandle GetBoxDebugMesh() const { return m_BoxMesh; }
		AssetHandle GetSphereDebugMesh() const { return m_SphereMesh; }
		AssetHandle GetCapsuleDebugMesh() const { return m_CapsuleMesh; }

		bool Exists(const Ref<MeshColliderAsset>& colliderAsset) const;
		void Rebuild();
		void Clear();

	private:
		void AddSimpleDebugMesh(const Ref<MeshColliderAsset>& colliderAsset, const Ref<StaticMesh>& debugMesh);
		void AddSimpleDebugMesh(const Ref<MeshColliderAsset>& colliderAsset, const Ref<Mesh>& debugMesh);
		void AddComplexDebugMesh(const Ref<MeshColliderAsset>& colliderAsset, const Ref<StaticMesh>& debugMesh);
		void AddComplexDebugMesh(const Ref<MeshColliderAsset>& colliderAsset, const Ref<Mesh>& debugMesh);

	private:
		std::unordered_map<AssetHandle, std::map<AssetHandle, CachedColliderData>> m_MeshData;

		// Editor-only
		std::unordered_map<AssetHandle, std::map<AssetHandle, Ref<StaticMesh>>> m_DebugSimpleStaticMeshes;
		std::unordered_map<AssetHandle, std::map<AssetHandle, Ref<Mesh>>> m_DebugSimpleMeshes;
		std::unordered_map<AssetHandle, std::map<AssetHandle, Ref<StaticMesh>>> m_DebugComplexStaticMeshes;
		std::unordered_map<AssetHandle, std::map<AssetHandle, Ref<Mesh>>> m_DebugComplexMeshes;

		AssetHandle m_BoxMesh = 0, m_SphereMesh = 0, m_CapsuleMesh = 0;

		friend class PhysXCookingFactory;
		friend class JoltCookingFactory;
	};

}

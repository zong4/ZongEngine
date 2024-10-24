#pragma once

#include "Engine/Physics/MeshCookingFactory.h"

namespace Engine {

	class JoltCookingFactory : public MeshCookingFactory
	{
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

		virtual std::pair<ECookingResult, ECookingResult> CookMesh(Ref<MeshColliderAsset> colliderAsset, bool invalidateOld = false) override;

	private:
		static ECookingResult CookConvexMesh(const Ref<MeshColliderAsset>& colliderAsset, const Ref<MeshSource>& meshSource, const std::vector<uint32_t>& submeshIndices, MeshColliderData& outData);
		static ECookingResult CookTriangleMesh(const Ref<MeshColliderAsset>& colliderAsset, const Ref<MeshSource>& meshSource, const std::vector<uint32_t>& submeshIndices, MeshColliderData& outData);
		static void GenerateDebugMesh(const Ref<MeshColliderAsset>& colliderAsset, const MeshColliderData& colliderData);

	};

}

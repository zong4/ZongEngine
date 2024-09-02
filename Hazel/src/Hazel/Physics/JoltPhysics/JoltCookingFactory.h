#pragma once

#include "Hazel/Physics/MeshCookingFactory.h"

namespace Hazel {

	class JoltCookingFactory : public MeshCookingFactory
	{
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

		virtual std::pair<ECookingResult, ECookingResult> CookMesh(Ref<MeshColliderAsset> colliderAsset, bool invalidateOld = false) override;

	private:
		static ECookingResult CookConvexMesh(const Ref<MeshColliderAsset>& colliderAsset, const Ref<MeshSource>& meshSource, const std::vector<uint32_t>& submeshIndices, MeshColliderData& outData);
		static ECookingResult CookTriangleMesh(const Ref<MeshColliderAsset>& colliderAsset, const Ref<MeshSource>& meshSource, const std::vector<uint32_t>& submeshIndices, MeshColliderData& outData);
#ifndef HZ_DIST
		static void GenerateDebugMesh(const Ref<MeshColliderAsset>& colliderAsset, const bool isStaticMesh, const std::vector<uint32_t>& submeshIndices, const MeshColliderData& colliderData);
#endif
	};

}

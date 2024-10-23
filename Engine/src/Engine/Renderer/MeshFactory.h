#pragma once

#include "Mesh.h"

namespace Hazel {

	class MeshFactory
	{
	public:
		static AssetHandle CreateBox(const glm::vec3& size);
		static AssetHandle CreateSphere(float radius);
		static AssetHandle CreateCapsule(float radius, float height);
	};

}

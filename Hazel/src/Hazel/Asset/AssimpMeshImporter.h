#pragma once

#include "Hazel/Renderer/Mesh.h"

#include <filesystem>

namespace Hazel {

#ifdef HZ_DIST
	class AssimpMeshImporter
	{
	public:
		AssimpMeshImporter(const std::filesystem::path& path) {}

		Ref<MeshSource> ImportToMeshSource() { return nullptr; }
		bool ImportSkeleton(Scope<Skeleton>& skeleton) { return false; }
		bool ImportAnimation(const uint32_t animationIndex, const Skeleton& skeleton, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask, Scope<Animation>& animation) { return false; }
		bool IsCompatibleSkeleton(const uint32_t animationIndex, const Skeleton& skeleton) { return false; }
		uint32_t GetAnimationCount() { return 0; }
	private:
		void TraverseNodes(Ref<MeshSource> meshSource, void* assimpNode, uint32_t nodeIndex, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0) {}
	private:
		const std::filesystem::path m_Path;
	};
#else
	class AssimpMeshImporter
	{
	public:
		AssimpMeshImporter(const std::filesystem::path& path);

		Ref<MeshSource> ImportToMeshSource();
		bool ImportSkeleton(Scope<Skeleton>& skeleton);
		bool ImportAnimation(const uint32_t animationIndex, const Skeleton& skeleton, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask, Scope<Animation>& animation);
		bool IsCompatibleSkeleton(const uint32_t animationIndex, const Skeleton& skeleton);
		uint32_t GetAnimationCount();
	private:
		void TraverseNodes(Ref<MeshSource> meshSource, void* assimpNode, uint32_t nodeIndex, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);
	private:
		const std::filesystem::path m_Path;
	};
#endif
}

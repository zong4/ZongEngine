#pragma once

#include "Engine/Animation/Animation.h"
#include "Engine/Animation/Skeleton.h"
#include "Engine/Core/Base.h"

#include <assimp/scene.h>

#include <string_view>
#include <vector>

namespace Hazel::AssimpAnimationImporter {

#ifdef HZ_DIST
	Scope<Skeleton> ImportSkeleton(const std::string_view filename) { return nullptr; }
	Scope<Skeleton> ImportSkeleton(const aiScene* scene) { return nullptr; }

	std::vector<std::string> GetAnimationNames(const aiScene* scene) { return std::vector<std::string>(); }
	Scope<Animation> ImportAnimation(const aiScene* scene, const std::string_view animationName, const Skeleton& skeleton, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask) { return nullptr; }
#else
	Scope<Skeleton> ImportSkeleton(const std::string_view filename);
	Scope<Skeleton> ImportSkeleton(const aiScene* scene);

	std::vector<std::string> GetAnimationNames(const aiScene* scene);
	Scope<Animation> ImportAnimation(const aiScene* scene, const uint32_t animationIndex, const Skeleton& skeleton, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask);
#endif
}

#pragma once

#include "Engine/Asset/Asset.h"
#include "Engine/Core/Base.h"

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace Engine {

	class MeshSource;

	// A skeleton is a hierarchy of bones (technically they're actually "joints",
	// but the misnomer "bone" is very common, so that's what I've gone with).
	// Each bone has a transform that describes its position relative to its parent.
	// The bones arranged thus give the "rest pose" of the skeleton.
	class Skeleton
	{
	public:
		static const uint32_t NullIndex = ~0;

	public:
		Skeleton() = default;
		Skeleton(uint32_t size);

		uint32_t AddBone(std::string name, uint32_t parentIndex, const glm::mat4& transform);
		uint32_t GetBoneIndex(const std::string_view name) const;

		uint32_t GetParentBoneIndex(const uint32_t boneIndex) const { ZONG_CORE_ASSERT(boneIndex < m_ParentBoneIndices.size(), "bone index out of range in Skeleton::GetParentIndex()!"); return m_ParentBoneIndices[boneIndex]; }
		const auto& GetParentBoneIndices() const { return m_ParentBoneIndices; }

		uint32_t GetNumBones() const { return static_cast<uint32_t>(m_BoneNames.size()); }
		const std::string& GetBoneName(const uint32_t boneIndex) const { ZONG_CORE_ASSERT(boneIndex < m_BoneNames.size(), "bone index out of range in Skeleton::GetBoneName()!"); return m_BoneNames[boneIndex]; }
		const auto& GetBoneNames() const { return m_BoneNames; }

		const std::vector<glm::vec3> GetBoneTranslations() const { return m_BoneTranslations; }
		const std::vector<glm::quat> GetBoneRotations() const { return m_BoneRotations; }
		const std::vector<glm::vec3> GetBoneScales() const { return m_BoneScales; }

		void SetBones(std::vector<std::string> boneNames, std::vector<uint32_t> parentBoneIndices, std::vector<glm::vec3> boneTranslations, std::vector<glm::quat> boneRotations, std::vector<glm::vec3> boneScales);
	private:
		std::vector<std::string> m_BoneNames;
		std::vector<uint32_t> m_ParentBoneIndices;
		
		// rest pose of skeleton. All in bone-local space (i.e. translation/rotation/scale relative to parent)
		std::vector<glm::vec3> m_BoneTranslations;
		std::vector<glm::quat> m_BoneRotations;
		std::vector<glm::vec3> m_BoneScales;
	};

	// Wraps a Skeleton as an "asset"
	// A skeleton ultimately comes from a mesh source (so named because it just so happened that meshes were
	// the first thing that "sources" were used for).   A source is an externally authored digital content file)
	class SkeletonAsset : public Asset
	{
	public:
		SkeletonAsset(const AssetHandle meshSource);
		SkeletonAsset(const Ref<MeshSource> meshSource);

		static AssetType GetStaticType() { return AssetType::Skeleton; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		Ref<MeshSource> GetMeshSource() const;
		const Skeleton& GetSkeleton() const;

	private:
		Ref<MeshSource> m_MeshSource;
	};

}

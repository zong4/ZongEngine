#pragma once

#include "Hazel/Asset/Asset.h"

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace Hazel {

	class MeshSource;

	// A skeleton is a hierarchy of bones (technically they're actually "joints",
	// but the term "bone" is very common, so that's what I've gone with).
	// Each bone has a transform that describes its position relative to its parent.
	// The bones arranged thus give the "rest pose" of the skeleton.
	class Skeleton
	{
	public:
		static const uint32_t NullIndex = ~0;

	public:
		Skeleton() = default;
		Skeleton(uint32_t size);

		const auto& GetTransform() const { return m_Transform; }
		void SetTransform(const glm::mat4& transform) { m_Transform = transform; }

		uint32_t AddBone(std::string name, uint32_t parentIndex, const glm::mat4& transform);
		uint32_t GetBoneIndex(const std::string_view name) const;

		const auto& GetParentBoneIndices() const { return m_ParentBoneIndices; }
		uint32_t GetParentBoneIndex(const uint32_t boneIndex) const { HZ_CORE_ASSERT(boneIndex < m_ParentBoneIndices.size(), "bone index out of range in Skeleton::GetParentIndex()!"); return m_ParentBoneIndices[boneIndex]; }
		std::vector<uint32_t> GetChildBoneIndexes(const uint32_t boneIndex) const;

		uint32_t GetNumBones() const { return static_cast<uint32_t>(m_BoneNames.size()); }
		const std::string& GetBoneName(const uint32_t boneIndex) const { HZ_CORE_ASSERT(boneIndex < m_BoneNames.size(), "bone index out of range in Skeleton::GetBoneName()!"); return m_BoneNames[boneIndex]; }
		const auto& GetBoneNames() const { return m_BoneNames; }

		const std::vector<glm::vec3>& GetBoneTranslations() const { return m_BoneTranslations; }
		const std::vector<glm::quat>& GetBoneRotations() const { return m_BoneRotations; }
		const std::vector<glm::vec3>& GetBoneScales() const { return m_BoneScales; }

		void SetBones(std::vector<std::string> boneNames, std::vector<uint32_t> parentBoneIndices, std::vector<glm::vec3> boneTranslations, std::vector<glm::quat> boneRotations, std::vector<glm::vec3> boneScales);

	private:
		std::vector<std::string> m_BoneNames;
		std::vector<uint32_t> m_ParentBoneIndices;
		
		// rest pose of skeleton. All in bone-local space (i.e. translation/rotation/scale relative to parent)
		std::vector<glm::vec3> m_BoneTranslations;
		std::vector<glm::quat> m_BoneRotations;
		std::vector<glm::vec3> m_BoneScales;

		// The skeleton itself can have a transform
		// Notably this happens if the whole "armature" is rotated or scaled in DCC tool
		glm::mat4 m_Transform;
	};

	// Wraps a Skeleton as an "asset"
	// A skeleton ultimately comes from a mesh source (so named because it just so happened that meshes were
	// the first thing that "sources" were used for).   A source is an externally authored digital content file)
	class SkeletonAsset : public Asset
	{
	public:
		SkeletonAsset(const AssetHandle meshSource);

		static AssetType GetStaticType() { return AssetType::Skeleton; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		AssetHandle GetMeshSource() const;
		const Skeleton& GetSkeleton() const;

		void OnDependencyUpdated(AssetHandle handle) override;

	private:
		AssetHandle m_MeshSource;
	};

}

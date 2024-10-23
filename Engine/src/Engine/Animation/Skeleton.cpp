#include "hzpch.h"
#include "Skeleton.h"

#include "Engine/Asset/AssetManager.h"
#include "Engine/Math/Math.h"
#include "Engine/Renderer/Mesh.h"

namespace Hazel {

	Skeleton::Skeleton(uint32_t size)
	{
		m_BoneNames.reserve(size);
		m_ParentBoneIndices.reserve(size);
		m_BoneTranslations.reserve(size);
		m_BoneRotations.reserve(size);
		m_BoneScales.reserve(size);
	}


	uint32_t Skeleton::AddBone(std::string name, uint32_t parentIndex, const glm::mat4& transform)
	{
		uint32_t index = static_cast<uint32_t>(m_BoneNames.size());
		m_BoneNames.emplace_back(name);
		m_ParentBoneIndices.emplace_back(parentIndex);
		m_BoneTranslations.emplace_back();
		m_BoneRotations.emplace_back();
		m_BoneScales.emplace_back();
		Math::DecomposeTransform(transform, m_BoneTranslations.back(), m_BoneRotations.back(), m_BoneScales.back());

		return index;
	}


	uint32_t Skeleton::GetBoneIndex(const std::string_view name) const
	{
		for (size_t i = 0; i < m_BoneNames.size(); ++i)
		{
			if (m_BoneNames[i] == name)
			{
				return static_cast<uint32_t>(i);
			}
		}
		return Skeleton::NullIndex;
	}


	void Skeleton::SetBones(std::vector<std::string> boneNames, std::vector<uint32_t> parentBoneIndices, std::vector<glm::vec3> boneTranslations, std::vector<glm::quat> boneRotations, std::vector<glm::vec3> boneScales)
	{
		HZ_CORE_ASSERT(parentBoneIndices.size() == boneNames.size());
		HZ_CORE_ASSERT(boneTranslations.size()  == boneNames.size());
		HZ_CORE_ASSERT(boneRotations.size()     == boneNames.size());
		HZ_CORE_ASSERT(boneScales.size()        == boneNames.size());
		m_BoneNames = std::move(boneNames);
		m_ParentBoneIndices = std::move(parentBoneIndices);
		m_BoneTranslations = std::move(boneTranslations);
		m_BoneRotations = std::move(boneRotations);
		m_BoneScales = std::move(boneScales);
	}


	SkeletonAsset::SkeletonAsset(const AssetHandle meshSource) : m_MeshSource(AssetManager::GetAsset<MeshSource>(meshSource))
	{
	}

	SkeletonAsset::SkeletonAsset(const Ref<MeshSource> meshSource) : m_MeshSource(meshSource)
	{
	}

	Hazel::Ref<Hazel::MeshSource> SkeletonAsset::GetMeshSource() const
	{
		return m_MeshSource;
	}

	const Hazel::Skeleton& SkeletonAsset::GetSkeleton() const
	{
		HZ_CORE_ASSERT(m_MeshSource && m_MeshSource->HasSkeleton());
		return m_MeshSource->GetSkeleton();
	}

}


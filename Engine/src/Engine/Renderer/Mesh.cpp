#include "pch.h" 
#include "Mesh.h"

#include "Engine/Debug/Profiler.h"
#include "Engine/Math/Math.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Project/Project.h"
#include "Engine/Asset/AssimpMeshImporter.h"
#include "Engine/Asset/AssetManager.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "imgui/imgui.h"

#include <filesystem>

namespace Engine
{

#define MESH_DEBUG_LOG 0
#if MESH_DEBUG_LOG
#define ZONG_MESH_LOG(...) ZONG_CORE_TRACE_TAG("Mesh", __VA_ARGS__)
#define ZONG_MESH_ERROR(...) ZONG_CORE_ERROR_TAG("Mesh", __VA_ARGS__)
#else
#define ZONG_MESH_LOG(...)
#define ZONG_MESH_ERROR(...)
#endif

	////////////////////////////////////////////////////////
	// MeshSource //////////////////////////////////////////
	////////////////////////////////////////////////////////
	MeshSource::MeshSource(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const glm::mat4& transform)
		: m_Vertices(vertices), m_Indices(indices)
	{
		// Generate a new asset handle
		Handle = {};

		Submesh submesh;
		submesh.BaseVertex = 0;
		submesh.BaseIndex = 0;
		submesh.IndexCount = (uint32_t)indices.size() * 3u;
		submesh.Transform = transform;
		m_Submeshes.push_back(submesh);

		m_VertexBuffer = VertexBuffer::Create(m_Vertices.data(), (uint32_t)(m_Vertices.size() * sizeof(Vertex)));
		m_IndexBuffer = IndexBuffer::Create(m_Indices.data(), (uint32_t)(m_Indices.size() * sizeof(Index)));
	}

	MeshSource::MeshSource(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const std::vector<Submesh>& submeshes)
		: m_Vertices(vertices), m_Indices(indices), m_Submeshes(submeshes)
	{
		// Generate a new asset handle
		Handle = {};

		m_VertexBuffer = VertexBuffer::Create(m_Vertices.data(), (uint32_t)(m_Vertices.size() * sizeof(Vertex)));
		m_IndexBuffer = IndexBuffer::Create(m_Indices.data(), (uint32_t)(m_Indices.size() * sizeof(Index)));

		// TODO: generate bounding box for submeshes, etc.
	}

	MeshSource::~MeshSource()
	{
	}

	static std::string LevelToSpaces(uint32_t level)
	{
		std::string result = "";
		for (uint32_t i = 0; i < level; i++)
			result += "--";
		return result;
	}

	void MeshSource::DumpVertexBuffer()
	{
		// TODO: Convert to ImGui
		ZONG_MESH_LOG("------------------------------------------------------");
		ZONG_MESH_LOG("Vertex Buffer Dump");
		ZONG_MESH_LOG("Mesh: {0}", m_FilePath);
		for (size_t i = 0; i < m_Vertices.size(); i++)
		{
			auto& vertex = m_Vertices[i];
			ZONG_MESH_LOG("Vertex: {0}", i);
			ZONG_MESH_LOG("Position: {0}, {1}, {2}", vertex.Position.x, vertex.Position.y, vertex.Position.z);
			ZONG_MESH_LOG("Normal: {0}, {1}, {2}", vertex.Normal.x, vertex.Normal.y, vertex.Normal.z);
			ZONG_MESH_LOG("Binormal: {0}, {1}, {2}", vertex.Binormal.x, vertex.Binormal.y, vertex.Binormal.z);
			ZONG_MESH_LOG("Tangent: {0}, {1}, {2}", vertex.Tangent.x, vertex.Tangent.y, vertex.Tangent.z);
			ZONG_MESH_LOG("TexCoord: {0}, {1}", vertex.Texcoord.x, vertex.Texcoord.y);
			ZONG_MESH_LOG("--");
		}
		ZONG_MESH_LOG("------------------------------------------------------");
	}


	// TODO (0x): this is temporary.. and will eventually be replaced with some kind of skeleton retargeting
	bool MeshSource::IsCompatibleSkeleton(const uint32_t animationIndex, const Skeleton& skeleton) const
	{
		if (!m_Skeleton)
		{
			ZONG_CORE_VERIFY(!m_Runtime);
			auto path = Project::GetEditorAssetManager()->GetFileSystemPath(Handle);
			AssimpMeshImporter importer(path);
			return importer.IsCompatibleSkeleton(animationIndex, skeleton);
		}
		
		return m_Skeleton->GetBoneNames() == skeleton.GetBoneNames();
	}

	std::vector<std::string> MeshSource::GetAnimationNames() const
	{
		return m_AnimationNames;
	}

	const Animation* MeshSource::GetAnimation(const uint32_t animationIndex, const Skeleton& skeleton, bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask) const
	{
		// Note: It's possible that the same animation index could be requested but with different root motion parameters.
		//       This is pretty edge-case, and not currently supported!
		if (!m_Animations[animationIndex])
		{
			// Deferred load of animations.
			// We cannot load them earlier (e.g. in MeshSource constructor) for two reasons:
			// 1) Assimp does not import bones (and hence no skeleton) if the mesh source file contains only animations (and no skin)
			//    This means we need to wait until we know what the skeleton is before we can load the animations.
			// 2) We don't have any way to pass the root motion parameters to the mesh source constructor

			ZONG_CORE_VERIFY(!m_Runtime);
			auto path = Project::GetEditorAssetManager()->GetFileSystemPath(Handle);
			AssimpMeshImporter importer(path);
			importer.ImportAnimation(animationIndex, skeleton, isMaskedRootMotion, rootTranslationMask, rootRotationMask, m_Animations[animationIndex]);
		}

		ZONG_CORE_ASSERT(animationIndex < m_Animations.size(), "Animation index out of range!");
		ZONG_CORE_ASSERT(m_Animations[animationIndex], "Attempted to access null animation!");
		if (animationIndex < m_Animations.size())
		{
			return m_Animations[animationIndex].get();
		}
		return nullptr;
	}


	Mesh::Mesh(Ref<MeshSource> meshSource)
		: m_MeshSource(meshSource)
	{
		// Generate a new asset handle
		Handle = {};

		SetSubmeshes({});

		const auto& meshMaterials = meshSource->GetMaterials();
		m_Materials = Ref<MaterialTable>::Create((uint32_t)meshMaterials.size());
		for (size_t i = 0; i < meshMaterials.size(); i++)
			m_Materials->SetMaterial((uint32_t)i, AssetManager::CreateMemoryOnlyAsset<MaterialAsset>(meshMaterials[i]));
	}

	Mesh::Mesh(Ref<MeshSource> meshSource, const std::vector<uint32_t>& submeshes)
		: m_MeshSource(meshSource)
	{
		// Generate a new asset handle
		Handle = {};

		SetSubmeshes(submeshes);

		const auto& meshMaterials = meshSource->GetMaterials();
		m_Materials = Ref<MaterialTable>::Create((uint32_t)meshMaterials.size());
		for (size_t i = 0; i < meshMaterials.size(); i++)
			m_Materials->SetMaterial((uint32_t)i, AssetManager::CreateMemoryOnlyAsset<MaterialAsset>(meshMaterials[i]));
	}

	Mesh::Mesh(const Ref<Mesh>& other)
		: m_MeshSource(other->m_MeshSource), m_Materials(other->m_Materials)
	{
		SetSubmeshes(other->m_Submeshes);
	}

	Mesh::~Mesh()
	{
	}

	void Mesh::SetSubmeshes(const std::vector<uint32_t>& submeshes)
	{
		if (!submeshes.empty())
		{
			m_Submeshes = submeshes;
		}
		else
		{
			const auto& submeshes = m_MeshSource->GetSubmeshes();
			m_Submeshes.resize(submeshes.size());
			for (uint32_t i = 0; i < submeshes.size(); i++)
				m_Submeshes[i] = i;
		}
	}

	////////////////////////////////////////////////////////
	// StaticMesh //////////////////////////////////////////
	////////////////////////////////////////////////////////

	StaticMesh::StaticMesh(Ref<MeshSource> meshSource)
		: m_MeshSource(meshSource)
	{
		// Generate a new asset handle
		Handle = {};

		SetSubmeshes({});

		const auto& meshMaterials = meshSource->GetMaterials();
		uint32_t numMaterials = static_cast<uint32_t>(meshMaterials.size());
		m_Materials = Ref<MaterialTable>::Create(numMaterials);
		for (uint32_t i = 0; i < numMaterials; i++)
			m_Materials->SetMaterial(i, AssetManager::CreateMemoryOnlyAsset<MaterialAsset>(meshMaterials[i]));
	}

	StaticMesh::StaticMesh(Ref<MeshSource> meshSource, const std::vector<uint32_t>& submeshes)
		: m_MeshSource(meshSource)
	{
		// Generate a new asset handle
		Handle = {};

		SetSubmeshes(submeshes);

		const auto& meshMaterials = meshSource->GetMaterials();
		uint32_t numMaterials = static_cast<uint32_t>(meshMaterials.size());
		m_Materials = Ref<MaterialTable>::Create(numMaterials);
		for (uint32_t i = 0; i < numMaterials; i++)
			m_Materials->SetMaterial(i, AssetManager::CreateMemoryOnlyAsset<MaterialAsset>(meshMaterials[i]));
	}

	StaticMesh::StaticMesh(const Ref<StaticMesh>& other)
		: m_MeshSource(other->m_MeshSource), m_Materials(other->m_Materials)
	{
		SetSubmeshes(other->m_Submeshes);
	}

	StaticMesh::~StaticMesh()
	{
	}

	void StaticMesh::SetSubmeshes(const std::vector<uint32_t>& submeshes)
	{
		if (!submeshes.empty())
		{
			m_Submeshes = submeshes;
		}
		else
		{
			const auto& submeshes = m_MeshSource->GetSubmeshes();
			m_Submeshes.resize(submeshes.size());
			for (uint32_t i = 0; i < submeshes.size(); i++)
				m_Submeshes[i] = i;
		}
	}
}

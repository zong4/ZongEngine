#include "pch.h"
#include "JoltCookingFactory.h"
#include "JoltBinaryStream.h"

#include "Engine/Physics/PhysicsSystem.h"

#include "Engine/Asset/AssetManager.h"
#include "Engine/Project/Project.h"
#include "Engine/Math/Math.h"
#include "Engine/Core/Timer.h"

#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>

namespace Hazel {

	namespace Utils {

		static std::filesystem::path GetCacheDirectory()
		{
			return Project::GetCacheDirectory() / "Colliders" / "Jolt";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::filesystem::path cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);
		}
	}

	void JoltCookingFactory::Init()
	{
	}

	void JoltCookingFactory::Shutdown()
	{
	}

	std::pair<ECookingResult, ECookingResult> JoltCookingFactory::CookMesh(Ref<MeshColliderAsset> colliderAsset, bool invalidateOld)
	{
		ZONG_SCOPE_TIMER("CookingFactory::CookMesh");

		Utils::CreateCacheDirectoryIfNeeded();

		AssetHandle colliderHandle = colliderAsset->Handle;

		if (!AssetManager::IsAssetHandleValid(colliderHandle))
		{
			ZONG_CORE_ERROR_TAG("Physics", "Tried to cook mesh collider using an invalid mesh!");
			return { ECookingResult::InvalidMesh, ECookingResult::InvalidMesh };
		}

		Ref<Asset> mesh = AssetManager::GetAsset<Asset>(colliderAsset->ColliderMesh);

		if (!mesh)
		{
			ZONG_CORE_ERROR_TAG("Physics", "Tried to cook mesh collider using an invalid mesh!");
			return { ECookingResult::InvalidMesh, ECookingResult::InvalidMesh };
		}

		ZONG_CORE_ASSERT(mesh->GetAssetType() == AssetType::StaticMesh || mesh->GetAssetType() == AssetType::Mesh);
		bool isStaticMesh = mesh->GetAssetType() == AssetType::StaticMesh;

		// name-handle.hmc
		std::string baseFileName = fmt::format("{0}-{1}", "Mesh", colliderHandle);

		const bool isPhysicalAsset = !AssetManager::IsMemoryAsset(colliderHandle);

		std::filesystem::path simpleColliderFilePath = Utils::GetCacheDirectory() / fmt::format("{0}-Simple.hmc", baseFileName);
		std::filesystem::path complexColliderFilePath = Utils::GetCacheDirectory() / fmt::format("{0}-Complex.hmc", baseFileName);

		CachedColliderData colliderData;
		ECookingResult simpleMeshResult = ECookingResult::Failure;
		ECookingResult complexMeshResult = ECookingResult::Failure;

		Ref<MeshSource> meshSource = isStaticMesh ? mesh.As<StaticMesh>()->GetMeshSource() : mesh.As<Mesh>()->GetMeshSource();
		const auto& submeshIndices = isStaticMesh ? mesh.As<StaticMesh>()->GetSubmeshes() : mesh.As<Mesh>()->GetSubmeshes();

		// Cook or load the simple collider
		{
			if (invalidateOld || !std::filesystem::exists(simpleColliderFilePath))
			{
				simpleMeshResult = CookConvexMesh(colliderAsset, meshSource, submeshIndices, colliderData.SimpleColliderData);

				if (simpleMeshResult == ECookingResult::Success && !SerializeMeshCollider(simpleColliderFilePath, colliderData.SimpleColliderData))
				{
					ZONG_CORE_ERROR_TAG("Physics", "Failed to cook simple collider mesh, aborting...");
					simpleMeshResult = ECookingResult::Failure;
				}
			}
			else
			{
				colliderData.SimpleColliderData = DeserializeMeshCollider(simpleColliderFilePath);
				simpleMeshResult = ECookingResult::Success;
			}

#ifndef ZONG_DIST
			if (simpleMeshResult == ECookingResult::Success)
				GenerateDebugMesh(colliderAsset, colliderData.SimpleColliderData);
#endif
		}

		// Cook or load the complex collider
		{
			if (invalidateOld || !std::filesystem::exists(complexColliderFilePath))
			{
				complexMeshResult = CookTriangleMesh(colliderAsset, meshSource, submeshIndices, colliderData.ComplexColliderData);

				if (complexMeshResult == ECookingResult::Success && !SerializeMeshCollider(complexColliderFilePath, colliderData.ComplexColliderData))
				{
					ZONG_CORE_ERROR_TAG("Physics", "Failed to cook complex collider mesh, aborting...");
					complexMeshResult = ECookingResult::Failure;
				}
			}
			else
			{
				colliderData.ComplexColliderData = DeserializeMeshCollider(complexColliderFilePath);
				complexMeshResult = ECookingResult::Success;
			}

#ifndef ZONG_DIST
			if (complexMeshResult == ECookingResult::Success)
				GenerateDebugMesh(colliderAsset, colliderData.ComplexColliderData);
#endif
		}

		if (simpleMeshResult == ECookingResult::Success || complexMeshResult == ECookingResult::Success)
		{
			// Add to cache
			auto& meshCache = PhysicsSystem::GetMeshCache();
			if (isPhysicalAsset)
				meshCache.m_MeshData[colliderAsset->ColliderMesh][colliderHandle] = colliderData;
			else
				meshCache.m_MeshData[colliderAsset->ColliderMesh][0] = colliderData;
		}

		return { simpleMeshResult, complexMeshResult };
	}

	ECookingResult JoltCookingFactory::CookConvexMesh(const Ref<MeshColliderAsset>& colliderAsset, const Ref<MeshSource>& meshSource, const std::vector<uint32_t>& submeshIndices, MeshColliderData& outData)
	{
		ECookingResult cookingResult = ECookingResult::Failure;

		const auto& vertices = meshSource->GetVertices();
		const auto& indices = meshSource->GetIndices();
		const auto& submeshes = meshSource->GetSubmeshes();

		for (auto submeshIndex : submeshIndices)
		{
			const auto& submesh = submeshes[submeshIndex];
			JPH::Array<JPH::Vec3> positions;

			for (uint32_t i = submesh.BaseIndex / 3; i < (submesh.BaseIndex / 3) + (submesh.IndexCount / 3); i++)
			{
				const Index& vertexIndex = indices[i];
				const Vertex& v0 = vertices[vertexIndex.V1];
				positions.push_back(JPH::Vec3(v0.Position.x, v0.Position.y, v0.Position.z));

				const Vertex& v1 = vertices[vertexIndex.V2];
				positions.push_back(JPH::Vec3(v1.Position.x, v1.Position.y, v1.Position.z));

				const Vertex& v2 = vertices[vertexIndex.V3];
				positions.push_back(JPH::Vec3(v2.Position.x, v2.Position.y, v2.Position.z));
			}

			JPH::RefConst<JPH::ConvexHullShapeSettings> meshSettings = new JPH::ConvexHullShapeSettings(positions);
			JPH::Shape::ShapeResult result = meshSettings->Create();

			if (result.HasError())
			{
				ZONG_CORE_ERROR_TAG("Physics", "Failed to cook convex mesh {}. Error: {}", submesh.MeshName, result.GetError());

				for (auto& existingSubmesh : outData.Submeshes)
					existingSubmesh.ColliderData.Release();
				outData.Submeshes.clear();

				cookingResult = ECookingResult::Failure;
				break;
			}

			JPH::RefConst<JPH::Shape> shape = result.Get();

			JoltBinaryStreamWriter bufferWriter;
			shape->SaveBinaryState(bufferWriter);

			SubmeshColliderData& data = outData.Submeshes.emplace_back();
			data.ColliderData = bufferWriter.ToBuffer();
			data.Transform = submesh.Transform * glm::scale(glm::mat4(1.0f), colliderAsset->ColliderScale);
			cookingResult = ECookingResult::Success;
		}

		outData.Type = MeshColliderType::Convex;
		return cookingResult;
	}

	ECookingResult JoltCookingFactory::CookTriangleMesh(const Ref<MeshColliderAsset>& colliderAsset, const Ref<MeshSource>& meshSource, const std::vector<uint32_t>& submeshIndices, MeshColliderData& outData)
	{
		ECookingResult cookingResult = ECookingResult::Failure;

		const auto& vertices = meshSource->GetVertices();
		const auto& indices = meshSource->GetIndices();
		const auto& submeshes = meshSource->GetSubmeshes();

		for (auto submeshIndex : submeshIndices)
		{
			const auto& submesh = submeshes[submeshIndex];

			JPH::VertexList vertexList;
			JPH::IndexedTriangleList triangleList;

			for (uint32_t vertexIndex = submesh.BaseVertex; vertexIndex < submesh.BaseVertex + submesh.VertexCount; vertexIndex++)
			{
				const Vertex& v = vertices[vertexIndex];
				vertexList.push_back(JPH::Float3(v.Position.x, v.Position.y, v.Position.z));
			}

			for (uint32_t triangleIndex = submesh.BaseIndex / 3; triangleIndex < (submesh.BaseIndex / 3) + (submesh.IndexCount / 3); triangleIndex++)
			{
				const Index& i = indices[triangleIndex];
				triangleList.push_back(JPH::IndexedTriangle(i.V1, i.V2, i.V3, 0));
			}

			JPH::RefConst<JPH::MeshShapeSettings> meshSettings = new JPH::MeshShapeSettings(vertexList, triangleList);

			JPH::Shape::ShapeResult result = meshSettings->Create();

			if (result.HasError())
			{
				ZONG_CORE_ERROR_TAG("Physics", "Failed to cook triangle mesh {}. Error: {}", submesh.MeshName, result.GetError());

				for (auto& existingSubmesh : outData.Submeshes)
					existingSubmesh.ColliderData.Release();
				outData.Submeshes.clear();

				cookingResult = ECookingResult::Failure;
				break;
			}

			JPH::RefConst<JPH::Shape> shape = result.Get();

			JoltBinaryStreamWriter bufferWriter;
			shape->SaveBinaryState(bufferWriter);

			SubmeshColliderData& data = outData.Submeshes.emplace_back();
			data.ColliderData = bufferWriter.ToBuffer();
			data.Transform = submesh.Transform * glm::scale(glm::mat4(1.0f), colliderAsset->ColliderScale);
			cookingResult = ECookingResult::Success;
		}

		outData.Type = MeshColliderType::Triangle;
		return cookingResult;
	}

	void JoltCookingFactory::GenerateDebugMesh(const Ref<MeshColliderAsset>& colliderAsset, const MeshColliderData& colliderData)
	{
	}

}

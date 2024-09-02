#include "hzpch.h"
#include "JoltCookingFactory.h"

#include "JoltBinaryStream.h"

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Core/Timer.h"
#include "Hazel/Math/Math.h"
#include "Hazel/Physics/PhysicsSystem.h"
#include "Hazel/Project/Project.h"

#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

#include <filesystem>
#include <format>

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
		HZ_SCOPE_TIMER("CookingFactory::CookMesh");

		Utils::CreateCacheDirectoryIfNeeded();

		AssetHandle colliderHandle = colliderAsset->Handle;

		if (!AssetManager::IsAssetHandleValid(colliderHandle))
		{
			HZ_CORE_ERROR_TAG("Physics", "Tried to cook mesh collider using an invalid mesh!");
			return { ECookingResult::InvalidMesh, ECookingResult::InvalidMesh };
		}

		Ref<Asset> mesh = AssetManager::GetAsset<Asset>(colliderAsset->ColliderMesh);

		if (!mesh)
		{
			HZ_CORE_ERROR_TAG("Physics", "Tried to cook mesh collider using an invalid mesh!");
			return { ECookingResult::InvalidMesh, ECookingResult::InvalidMesh };
		}

		HZ_CORE_ASSERT(mesh->GetAssetType() == AssetType::StaticMesh || mesh->GetAssetType() == AssetType::Mesh);
		bool isStaticMesh = mesh->GetAssetType() == AssetType::StaticMesh;

		// name-handle.hmc
		std::string baseFileName = std::format("{0}-{1}", "Mesh", colliderHandle);

		const bool isPhysicalAsset = !AssetManager::GetMemoryAsset(colliderHandle);

		std::filesystem::path simpleColliderFilePath = Utils::GetCacheDirectory() / std::format("{0}-Simple.hmc", baseFileName);
		std::filesystem::path complexColliderFilePath = Utils::GetCacheDirectory() / std::format("{0}-Complex.hmc", baseFileName);

		CachedColliderData colliderData;
		ECookingResult simpleMeshResult = ECookingResult::Failure;
		ECookingResult complexMeshResult = ECookingResult::Failure;

		if (auto meshSource = AssetManager::GetAsset<MeshSource>(isStaticMesh ? mesh.As<StaticMesh>()->GetMeshSource() : mesh.As<Mesh>()->GetMeshSource()); meshSource)
		{
			const auto& submeshIndices = isStaticMesh ? mesh.As<StaticMesh>()->GetSubmeshes() : mesh.As<Mesh>()->GetSubmeshes();

			// Cook or load the simple collider
			if (invalidateOld || !std::filesystem::exists(simpleColliderFilePath))
			{
				simpleMeshResult = CookConvexMesh(colliderAsset, meshSource, submeshIndices, colliderData.SimpleColliderData);

				if (simpleMeshResult == ECookingResult::Success && !SerializeMeshCollider(simpleColliderFilePath, colliderData.SimpleColliderData))
				{
					HZ_CORE_ERROR_TAG("Physics", "Failed to cook simple collider mesh, aborting...");
					simpleMeshResult = ECookingResult::Failure;
				}
			}
			else
			{
				colliderData.SimpleColliderData = DeserializeMeshCollider(simpleColliderFilePath);
				simpleMeshResult = ECookingResult::Success;
			}

#ifndef HZ_DIST
			if (simpleMeshResult == ECookingResult::Success)
				GenerateDebugMesh(colliderAsset, isStaticMesh, submeshIndices, colliderData.SimpleColliderData);
#endif

			// Cook or load the complex collider
			if (invalidateOld || !std::filesystem::exists(complexColliderFilePath))
			{
				complexMeshResult = CookTriangleMesh(colliderAsset, meshSource, submeshIndices, colliderData.ComplexColliderData);

				if (complexMeshResult == ECookingResult::Success && !SerializeMeshCollider(complexColliderFilePath, colliderData.ComplexColliderData))
				{
					HZ_CORE_ERROR_TAG("Physics", "Failed to cook complex collider mesh, aborting...");
					complexMeshResult = ECookingResult::Failure;
				}
			}
			else
			{
				colliderData.ComplexColliderData = DeserializeMeshCollider(complexColliderFilePath);
				complexMeshResult = ECookingResult::Success;
			}

#ifndef HZ_DIST
			if (complexMeshResult == ECookingResult::Success)
				GenerateDebugMesh(colliderAsset, isStaticMesh, submeshIndices, colliderData.ComplexColliderData);
#endif

			if (simpleMeshResult == ECookingResult::Success || complexMeshResult == ECookingResult::Success)
			{
				// Add to cache
				auto& meshCache = PhysicsSystem::GetMeshCache();
				if (isPhysicalAsset)
					meshCache.m_MeshData[colliderAsset->ColliderMesh][colliderHandle] = colliderData;
				else
					meshCache.m_MeshData[colliderAsset->ColliderMesh][0] = colliderData;
			}
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

			if (submesh.VertexCount < 3)
			{
				outData.Submeshes.emplace_back();
				continue;
			}

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
				HZ_CORE_ERROR_TAG("Physics", "Failed to cook convex mesh {}. Error: {}", submesh.MeshName, result.GetError());

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

			if (submesh.VertexCount < 3)
			{
				outData.Submeshes.emplace_back();
				continue;
			}

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
				HZ_CORE_ERROR_TAG("Physics", "Failed to cook triangle mesh {}. Error: {}", submesh.MeshName, result.GetError());

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

#ifndef HZ_DIST
	void JoltCookingFactory::GenerateDebugMesh(const Ref<MeshColliderAsset>& colliderAsset, const bool isStaticMesh, const std::vector<uint32_t>& submeshIndices, const MeshColliderData& colliderData)
	{
		std::vector<Vertex> vertices;
		std::vector<Index> indices;
		std::vector<Submesh> submeshes;
		for (size_t i = 0; i < colliderData.Submeshes.size(); i++)
		{
			const SubmeshColliderData& submeshData = colliderData.Submeshes[i];

			// retrieve the shape by restoring from the binary data
			JoltBinaryStreamReader bufferReader(submeshData.ColliderData);
			auto shapeResult = JPH::Shape::sRestoreFromBinaryState(bufferReader);
			if (!shapeResult.HasError())
			{
				auto shape = shapeResult.Get();
				auto jphCom = shape->GetCenterOfMass();
				glm::vec3 com = { jphCom.GetX(), jphCom.GetY(), jphCom.GetZ() };
				JPH::Shape::VisitedShapes ioVisitedShapes;
				JPH::Shape::Stats stats = shape->GetStatsRecursive(ioVisitedShapes);

				if (stats.mNumTriangles > 0)
				{
					Submesh& submesh = submeshes.emplace_back();
					submesh.BaseVertex = static_cast<uint32_t>(vertices.size());
					submesh.VertexCount = stats.mNumTriangles * 3;
					submesh.BaseIndex = static_cast<uint32_t>(indices.size()) * 3;
					submesh.IndexCount = stats.mNumTriangles * 3; // Watch out. There are 3 "indexes" per Hazel::Index
					submesh.MaterialIndex = 0;
					submesh.Transform = submeshData.Transform;
					vertices.reserve(vertices.size() + stats.mNumTriangles * 3);
					indices.reserve(indices.size() + stats.mNumTriangles);

					JPH::Shape::GetTrianglesContext ioContext;
					JPH::AABox inBox;
					JPH::Vec3 inPositionCOM = JPH::Vec3::sZero();
					JPH::Quat inRotation = JPH::Quat::sIdentity();
					JPH::Vec3 inScale = JPH::Vec3::sReplicate(1.0f);
					shape->GetTrianglesStart(ioContext, inBox, inPositionCOM, inRotation, inScale);

					JPH::Float3 jphVertices[100 * 3];
					int count = 0;
					while ((count = shape->GetTrianglesNext(ioContext, 100, jphVertices)) > 0)
					{
						uint32_t baseIndex = static_cast<uint32_t>(vertices.size());
						for (int i = 0; i < count; ++i)
						{
							Vertex& v1 = vertices.emplace_back();
							v1.Position = { jphVertices[3 * i].x, jphVertices[3 * i].y, jphVertices[3 * i].z };
							v1.Position += com;

							Vertex& v2 = vertices.emplace_back();
							v2.Position = { jphVertices[3 * i + 1].x, jphVertices[3 * i + 1].y, jphVertices[3 * i + 1].z };
							v2.Position += com;

							Vertex& v3 = vertices.emplace_back();
							v3.Position = { jphVertices[3 * i + 2].x, jphVertices[3 * i + 2].y, jphVertices[3 * i + 2].z };
							v3.Position += com;

							Index& index = indices.emplace_back();
							index.V1 = static_cast<uint32_t>(baseIndex + 3 * i);
							index.V2 = static_cast<uint32_t>(baseIndex + 3 * i + 1);
							index.V3 = static_cast<uint32_t>(baseIndex + 3 * i + 2);
						}
					}
				}
			}
		}
		if (vertices.size() > 0)
		{
			Ref<MeshSource> meshSource = Ref<MeshSource>::Create(vertices, indices, submeshes);
			AssetManager::AddMemoryOnlyAsset(meshSource);
			if (colliderData.Type == MeshColliderType::Convex)
			{
				if (isStaticMesh)
				{
					PhysicsSystem::GetMeshCache().AddSimpleDebugMesh(colliderAsset, Ref<StaticMesh>::Create(meshSource->Handle, submeshIndices, /*generateColliders=*/false));
				}
				else
				{
					PhysicsSystem::GetMeshCache().AddSimpleDebugMesh(colliderAsset, Ref<Mesh>::Create(meshSource->Handle, submeshIndices, /*generateColliders=*/false));
				}
			}
			else
			{
				if (isStaticMesh)
				{
					PhysicsSystem::GetMeshCache().AddComplexDebugMesh(colliderAsset, Ref<StaticMesh>::Create(meshSource->Handle, submeshIndices, /*generateColliders=*/false));
				}
				else
				{
					PhysicsSystem::GetMeshCache().AddSimpleDebugMesh(colliderAsset, Ref<Mesh>::Create(meshSource->Handle, submeshIndices, /*generateColliders=*/false));
				}
			}
		}
	}
#endif

}

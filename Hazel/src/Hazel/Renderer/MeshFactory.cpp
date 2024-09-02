#include "hzpch.h"
#include "MeshFactory.h"

#include "Hazel/Asset/AssetManager.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace Hazel {

	AssetHandle MeshFactory::CreateBox(const glm::vec3& size)
	{
		std::vector<Vertex> vertices;
		vertices.resize(8);
		vertices[0].Position = { -size.x / 2.0f, -size.y / 2.0f,  size.z / 2.0f };
		vertices[1].Position = {  size.x / 2.0f, -size.y / 2.0f,  size.z / 2.0f };
		vertices[2].Position = {  size.x / 2.0f,  size.y / 2.0f,  size.z / 2.0f };
		vertices[3].Position = { -size.x / 2.0f,  size.y / 2.0f,  size.z / 2.0f };
		vertices[4].Position = { -size.x / 2.0f, -size.y / 2.0f, -size.z / 2.0f };
		vertices[5].Position = {  size.x / 2.0f, -size.y / 2.0f, -size.z / 2.0f };
		vertices[6].Position = {  size.x / 2.0f,  size.y / 2.0f, -size.z / 2.0f };
		vertices[7].Position = { -size.x / 2.0f,  size.y / 2.0f, -size.z / 2.0f };

		vertices[0].Normal = { -1.0f, -1.0f,  1.0f };
		vertices[1].Normal = {  1.0f, -1.0f,  1.0f };
		vertices[2].Normal = {  1.0f,  1.0f,  1.0f };
		vertices[3].Normal = { -1.0f,  1.0f,  1.0f };
		vertices[4].Normal = { -1.0f, -1.0f, -1.0f };
		vertices[5].Normal = {  1.0f, -1.0f, -1.0f };
		vertices[6].Normal = {  1.0f,  1.0f, -1.0f };
		vertices[7].Normal = { -1.0f,  1.0f, -1.0f };

		std::vector<Index> indices;
		indices.resize(12);
		indices[0] =  { 0, 1, 2 };
		indices[1] =  { 2, 3, 0 };
		indices[2] =  { 1, 5, 6 };
		indices[3] =  { 6, 2, 1 };
		indices[4] =  { 7, 6, 5 };
		indices[5] =  { 5, 4, 7 };
		indices[6] =  { 4, 0, 3 };
		indices[7] =  { 3, 7, 4 };
		indices[8] =  { 4, 5, 1 };
		indices[9] =  { 1, 0, 4 };
		indices[10] = { 3, 2, 6 };
		indices[11] = { 6, 7, 3 };

		AssetHandle meshSource = AssetManager::AddMemoryOnlyAsset(Ref<MeshSource>::Create(vertices, indices, glm::mat4(1.0f)));
		return AssetManager::AddMemoryOnlyAsset(Ref<StaticMesh>::Create(meshSource, /*generateColliders=*/false));
	}

	AssetHandle MeshFactory::CreateSphere(float radius)
	{
		std::vector<Vertex> vertices;
		std::vector<Index> indices;

		constexpr float latitudeBands = 30;
		constexpr float longitudeBands = 30;

		for (float latitude = 0.0f; latitude <= latitudeBands; latitude++)
		{
			const float theta = latitude * (float)M_PI / latitudeBands;
			const float sinTheta = glm::sin(theta);
			const float cosTheta = glm::cos(theta);

			for (float longitude = 0.0f; longitude <= longitudeBands; longitude++)
			{
				const float phi = longitude * 2.f * (float)M_PI / longitudeBands;
				const float sinPhi = glm::sin(phi);
				const float cosPhi = glm::cos(phi);

				Vertex vertex;
				vertex.Normal = { cosPhi * sinTheta, cosTheta, sinPhi * sinTheta };
				vertex.Position = { radius * vertex.Normal.x, radius * vertex.Normal.y, radius * vertex.Normal.z };
				vertices.push_back(vertex);
			}
		}

		for (uint32_t latitude = 0; latitude < (uint32_t)latitudeBands; latitude++)
		{
			for (uint32_t longitude = 0; longitude < (uint32_t)longitudeBands; longitude++)
			{
				const uint32_t first = (latitude * ((uint32_t)longitudeBands + 1)) + longitude;
				const uint32_t second = first + (uint32_t)longitudeBands + 1;

				indices.push_back({ first, second, first + 1 });
				indices.push_back({ second, second + 1, first + 1 });
			}
		}

		AssetHandle meshSource = AssetManager::AddMemoryOnlyAsset(Ref<MeshSource>::Create(vertices, indices, glm::mat4(1.0f)));
		return AssetManager::AddMemoryOnlyAsset(Ref<StaticMesh>::Create(meshSource, /*generateColliders=*/false));
	}

	static void CalculateRing(size_t segments, float radius, float y, float dy, float height, float actualRadius, std::vector<Vertex>& vertices)
	{
		float segIncr = 1.0f / (float)(segments - 1);
		for (size_t s = 0; s < segments; s++)
		{
			float x = glm::cos(float(M_PI * 2) * s * segIncr) * radius;
			float z = glm::sin(float(M_PI * 2) * s * segIncr) * radius;

			Vertex& vertex = vertices.emplace_back();
			vertex.Position = glm::vec3(actualRadius * x, actualRadius * y + height * dy, actualRadius * z);
		}
	}

	AssetHandle MeshFactory::CreateCapsule(float radius, float height)
	{
		constexpr size_t subdivisionsHeight = 8;
		constexpr size_t ringsBody = subdivisionsHeight + 1;
		constexpr size_t ringsTotal = subdivisionsHeight + ringsBody;
		constexpr size_t numSegments = 12;
		constexpr float radiusModifier = 0.021f; // Needed to ensure that the wireframe is always visible

		std::vector<Vertex> vertices;
		std::vector<Index> indices;
		
		vertices.reserve(numSegments * ringsTotal);
		indices.reserve((numSegments - 1) * (ringsTotal - 1) * 2);

		float bodyIncr = 1.0f / (float)(ringsBody - 1);
		float ringIncr = 1.0f / (float)(subdivisionsHeight - 1);

		for (int r = 0; r < subdivisionsHeight / 2; r++)
			CalculateRing(numSegments, glm::sin(float(M_PI) * r * ringIncr), glm::sin(float(M_PI) * (r * ringIncr - 0.5f)), -0.5f, height, radius + radiusModifier, vertices);

		for (int r = 0; r < ringsBody; r++)
			CalculateRing(numSegments, 1.0f, 0.0f, r * bodyIncr - 0.5f, height, radius + radiusModifier, vertices);

		for (int r = subdivisionsHeight / 2; r < subdivisionsHeight; r++)
			CalculateRing(numSegments, glm::sin(float(M_PI) * r * ringIncr), glm::sin(float(M_PI) * (r * ringIncr - 0.5f)), 0.5f, height, radius + radiusModifier, vertices);

		for (int r = 0; r < ringsTotal - 1; r++)
		{
			for (int s = 0; s < numSegments - 1; s++)
			{
				Index& index1 = indices.emplace_back();
				index1.V1 = (uint32_t)(r * numSegments + s + 1);
				index1.V2 = (uint32_t)(r * numSegments + s + 0);
				index1.V3 = (uint32_t)((r + 1) * numSegments + s + 1);

				Index& index2 = indices.emplace_back();
				index2.V1 = (uint32_t)((r + 1) * numSegments + s + 0);
				index2.V2 = (uint32_t)((r + 1) * numSegments + s + 1);
				index2.V3 = (uint32_t)(r * numSegments + s);
			}
		}

		AssetHandle meshSource = AssetManager::AddMemoryOnlyAsset(Ref<MeshSource>::Create(vertices, indices, glm::mat4(1.0f)));
		return AssetManager::AddMemoryOnlyAsset(Ref<StaticMesh>::Create(meshSource, /*generateColliders=*/false));
	}
}

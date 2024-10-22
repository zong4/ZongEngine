#include "hzpch.h"
#include "AssimpMeshImporter.h"

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Asset/AssimpAnimationImporter.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Utilities/AssimpLogStream.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

namespace Hazel {

#define MESH_DEBUG_LOG 0
#if MESH_DEBUG_LOG
#define HZ_MESH_LOG(...) HZ_CORE_TRACE_TAG("Mesh", __VA_ARGS__)
#define HZ_MESH_ERROR(...) HZ_CORE_ERROR_TAG("Mesh", __VA_ARGS__)
#else
#define HZ_MESH_LOG(...)
#define HZ_MESH_ERROR(...)
#endif

	static const uint32_t s_MeshImportFlags =
		aiProcess_CalcTangentSpace |        // Create binormals/tangents just in case
		aiProcess_Triangulate |             // Make sure we're triangles
		aiProcess_SortByPType |             // Split meshes by primitive type
		aiProcess_GenNormals |              // Make sure we have legit normals
		aiProcess_GenUVCoords |             // Convert UVs if required 
//		aiProcess_OptimizeGraph |
		aiProcess_OptimizeMeshes |          // Batch draws where possible
		aiProcess_JoinIdenticalVertices |
		aiProcess_LimitBoneWeights |        // If more than N (=4) bone weights, discard least influencing bones and renormalise sum to 1
		aiProcess_ValidateDataStructure |   // Validation
		aiProcess_GlobalScale;              // e.g. convert cm to m for fbx import (and other formats where cm is native)

	namespace Utils {

		glm::mat4 Mat4FromAIMatrix4x4(const aiMatrix4x4& matrix)
		{
			glm::mat4 result;
			//the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
			result[0][0] = matrix.a1; result[1][0] = matrix.a2; result[2][0] = matrix.a3; result[3][0] = matrix.a4;
			result[0][1] = matrix.b1; result[1][1] = matrix.b2; result[2][1] = matrix.b3; result[3][1] = matrix.b4;
			result[0][2] = matrix.c1; result[1][2] = matrix.c2; result[2][2] = matrix.c3; result[3][2] = matrix.c4;
			result[0][3] = matrix.d1; result[1][3] = matrix.d2; result[2][3] = matrix.d3; result[3][3] = matrix.d4;
			return result;
		}

#if MESH_DEBUG_LOG
		void PrintNode(aiNode* node, size_t depth)
		{
			HZ_MESH_LOG("{0:^{1}}{2} {{", "", depth * 3, node->mName.C_Str());
			++depth;
			glm::vec3 translation;
			glm::vec3 rotation;
			glm::vec3 scale;
			glm::mat4 transform = Mat4FromAIMatrix4x4(node->mTransformation);
			Math::DecomposeTransform(transform, translation, rotation, scale);
			rotation = glm::degrees(rotation);

			HZ_MESH_LOG("{0:^{1}}translation: ({2:6.2f}, {3:6.2f}, {4:6.2f})", "", depth * 3, translation.x, translation.y, translation.z);
			HZ_MESH_LOG("{0:^{1}}rotation:    ({2:6.2f}, {3:6.2f}, {4:6.2f})", "", depth * 3, rotation.x, rotation.y, rotation.z);
			HZ_MESH_LOG("{0:^{1}}scale:       ({2:6.2f}, {3:6.2f}, {4:6.2f})", "", depth * 3, scale.x, scale.y, scale.z);
			for (uint32_t i = 0; i < node->mNumChildren; ++i)
			{
				PrintNode(node->mChildren[i], depth);
			}
			--depth;
			HZ_MESH_LOG("{0:^{1}}}}", "", depth * 3);
		}
#endif

	}

	AssimpMeshImporter::AssimpMeshImporter(const std::filesystem::path& path)
		: m_Path(path)
	{
		AssimpLogStream::Initialize();
	}

	Ref<MeshSource> AssimpMeshImporter::ImportToMeshSource()
	{
		Ref<MeshSource> meshSource = Ref<MeshSource>::Create();

		HZ_CORE_INFO_TAG("Mesh", "Loading mesh: {0}", m_Path.string());

		Assimp::Importer importer;
		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

		const aiScene* scene = importer.ReadFile(m_Path.string(), s_MeshImportFlags);
		if (!scene /* || !scene->HasMeshes()*/)  // note: scene can legit contain no meshes (e.g. it could contain an armature, an animation, and no skin (mesh)))
		{
			HZ_CORE_ERROR_TAG("Mesh", "Failed to load mesh file: {0}", m_Path.string());
			meshSource->SetFlag(AssetFlag::Invalid);
			return nullptr;
		}

		meshSource->m_Skeleton = AssimpAnimationImporter::ImportSkeleton(scene);
		HZ_CORE_INFO_TAG("Animation", "Skeleton {0} found in mesh file '{1}'", meshSource->HasSkeleton() ? "" : "not", m_Path.string());

		meshSource->m_Animations.resize(scene->mNumAnimations);
		meshSource->m_AnimationNames.reserve(scene->mNumAnimations);
		for (uint32_t i = 0; i < scene->mNumAnimations; ++i)
		{
			meshSource->m_AnimationNames.emplace_back(scene->mAnimations[i]->mName.C_Str());
		}

		// Actual load of the animations is deferred until later.
		// Because:
		// 1. If there is no skin (mesh), then assimp will not have loaded the skeleton, and we cannot
		//    load the animations until we know what the skeleton is
		// 2. Loading the animation requires some extra parameters to control how to import the root motion
		//    This constructor has no way of knowing what those parameters are.

		// If no meshes in the scene, there's nothing more for us to do
		if (scene->HasMeshes())
		{
			uint32_t vertexCount = 0;
			uint32_t indexCount = 0;

			meshSource->m_BoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
			meshSource->m_BoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

			meshSource->m_Submeshes.reserve(scene->mNumMeshes);
			for (unsigned m = 0; m < scene->mNumMeshes; m++)
			{
				aiMesh* mesh = scene->mMeshes[m];

				Submesh& submesh = meshSource->m_Submeshes.emplace_back();
				submesh.BaseVertex = vertexCount;
				submesh.BaseIndex = indexCount;
				submesh.MaterialIndex = mesh->mMaterialIndex;
				submesh.VertexCount = mesh->mNumVertices;
				submesh.IndexCount = mesh->mNumFaces * 3;
				submesh.MeshName = mesh->mName.C_Str();

				vertexCount += mesh->mNumVertices;
				indexCount += submesh.IndexCount;

				HZ_CORE_ASSERT(mesh->HasPositions(), "Meshes require positions.");
				HZ_CORE_ASSERT(mesh->HasNormals(), "Meshes require normals.");

				// Vertices
				auto& aabb = submesh.BoundingBox;
				aabb.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
				aabb.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
				for (size_t i = 0; i < mesh->mNumVertices; i++)
				{
					Vertex vertex;
					vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
					vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
					aabb.Min.x = glm::min(vertex.Position.x, aabb.Min.x);
					aabb.Min.y = glm::min(vertex.Position.y, aabb.Min.y);
					aabb.Min.z = glm::min(vertex.Position.z, aabb.Min.z);
					aabb.Max.x = glm::max(vertex.Position.x, aabb.Max.x);
					aabb.Max.y = glm::max(vertex.Position.y, aabb.Max.y);
					aabb.Max.z = glm::max(vertex.Position.z, aabb.Max.z);

					if (mesh->HasTangentsAndBitangents())
					{
						vertex.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
						vertex.Binormal = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
					}

					if (mesh->HasTextureCoords(0))
						vertex.Texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

					meshSource->m_Vertices.push_back(vertex);
				}

				// Indices
				for (size_t i = 0; i < mesh->mNumFaces; i++)
				{
					HZ_CORE_ASSERT(mesh->mFaces[i].mNumIndices == 3, "Must have 3 indices.");
					Index index = { mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1], mesh->mFaces[i].mIndices[2] };
					meshSource->m_Indices.push_back(index);

					meshSource->m_TriangleCache[m].emplace_back(meshSource->m_Vertices[index.V1 + submesh.BaseVertex], meshSource->m_Vertices[index.V2 + submesh.BaseVertex], meshSource->m_Vertices[index.V3 + submesh.BaseVertex]);
				}
			}

#if MESH_DEBUG_LOG
			HZ_CORE_INFO_TAG("Mesh", "Traversing nodes for scene '{0}'", filename);
			Utils::PrintNode(scene->mRootNode, 0);
#endif

			MeshNode& rootNode = meshSource->m_Nodes.emplace_back();
			TraverseNodes(meshSource, scene->mRootNode, 0);

			for (const auto& submesh : meshSource->m_Submeshes)
			{
				AABB transformedSubmeshAABB = submesh.BoundingBox;
				glm::vec3 min = glm::vec3(submesh.Transform * glm::vec4(transformedSubmeshAABB.Min, 1.0f));
				glm::vec3 max = glm::vec3(submesh.Transform * glm::vec4(transformedSubmeshAABB.Max, 1.0f));

				meshSource->m_BoundingBox.Min.x = glm::min(meshSource->m_BoundingBox.Min.x, min.x);
				meshSource->m_BoundingBox.Min.y = glm::min(meshSource->m_BoundingBox.Min.y, min.y);
				meshSource->m_BoundingBox.Min.z = glm::min(meshSource->m_BoundingBox.Min.z, min.z);
				meshSource->m_BoundingBox.Max.x = glm::max(meshSource->m_BoundingBox.Max.x, max.x);
				meshSource->m_BoundingBox.Max.y = glm::max(meshSource->m_BoundingBox.Max.y, max.y);
				meshSource->m_BoundingBox.Max.z = glm::max(meshSource->m_BoundingBox.Max.z, max.z);
			}
		}

		// Bones
		if (meshSource->HasSkeleton())
		{
			meshSource->m_BoneInfluences.resize(meshSource->m_Vertices.size());
			for (uint32_t m = 0; m < scene->mNumMeshes; m++)
			{
				aiMesh* mesh = scene->mMeshes[m];
				Submesh& submesh = meshSource->m_Submeshes[m];

				if (mesh->mNumBones > 0)
				{
					submesh.IsRigged = true;
					for (uint32_t i = 0; i < mesh->mNumBones; i++)
					{
						aiBone* bone = mesh->mBones[i];
						bool hasNonZeroWeight = false;
						for (size_t j = 0; j < bone->mNumWeights; j++)
						{
							if (bone->mWeights[j].mWeight > 0.000001f)
							{
								hasNonZeroWeight = true;
							}
						}
						if (!hasNonZeroWeight)
							continue;

						// Find bone in skeleton
						uint32_t boneIndex = meshSource->m_Skeleton->GetBoneIndex(bone->mName.C_Str());
						if (boneIndex == Skeleton::NullIndex)
						{
							HZ_CORE_ERROR_TAG("Animation", "Could not find mesh bone '{}' in skeleton!", bone->mName.C_Str());
						}

						uint32_t boneInfoIndex = ~0;
						for (size_t j = 0; j < meshSource->m_BoneInfo.size(); ++j)
						{
							// note: Same bone could influence different submeshes (and each will have different transforms in the bind pose).
							//       Hence the need to differentiate on submesh index here.
							if ((meshSource->m_BoneInfo[j].BoneIndex == boneIndex) && (meshSource->m_BoneInfo[j].SubMeshIndex == m))
							{
								boneInfoIndex = static_cast<uint32_t>(j);
								break;
							}
						}
						if (boneInfoIndex == ~0)
						{
							boneInfoIndex = static_cast<uint32_t>(meshSource->m_BoneInfo.size());
							meshSource->m_BoneInfo.emplace_back(glm::inverse(submesh.Transform), Utils::Mat4FromAIMatrix4x4(bone->mOffsetMatrix), m, boneIndex);
						}

						for (size_t j = 0; j < bone->mNumWeights; j++)
						{
							int VertexID = submesh.BaseVertex + bone->mWeights[j].mVertexId;
							float Weight = bone->mWeights[j].mWeight;
							meshSource->m_BoneInfluences[VertexID].AddBoneData(boneInfoIndex, Weight);
						}
					}
				}
			}

			for (auto& boneInfluence : meshSource->m_BoneInfluences)
			{
				boneInfluence.NormalizeWeights();
			}
		}

		// Materials
		Ref<Texture2D> whiteTexture = Renderer::GetWhiteTexture();
		if (scene->HasMaterials())
		{
			HZ_MESH_LOG("---- Materials - {0} ----", filename);

			meshSource->m_Materials.resize(scene->mNumMaterials);

			for (uint32_t i = 0; i < scene->mNumMaterials; i++)
			{
				auto aiMaterial = scene->mMaterials[i];
				auto aiMaterialName = aiMaterial->GetName();
				Hazel::Ref<Material> mi = Material::Create(Renderer::GetShaderLibrary()->Get("HazelPBR_Static"), aiMaterialName.data);
				meshSource->m_Materials[i] = mi;

				HZ_MESH_LOG("  {0} (Index = {1})", aiMaterialName.data, i);
				aiString aiTexPath;
				uint32_t textureCount = aiMaterial->GetTextureCount(aiTextureType_DIFFUSE);
				HZ_MESH_LOG("    TextureCount = {0}", textureCount);

				glm::vec3 albedoColor(0.8f);
				float emission = 0.0f;
				aiColor3D aiColor, aiEmission;
				if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor) == AI_SUCCESS)
					albedoColor = { aiColor.r, aiColor.g, aiColor.b };

				if (aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, aiEmission) == AI_SUCCESS)
					emission = aiEmission.r;

				mi->Set("u_MaterialUniforms.AlbedoColor", albedoColor);
				mi->Set("u_MaterialUniforms.Emission", emission);

				float roughness, metalness;
				if (aiMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) != aiReturn_SUCCESS)
					roughness = 0.5f; // Default value

				if (aiMaterial->Get(AI_MATKEY_REFLECTIVITY, metalness) != aiReturn_SUCCESS)
					metalness = 0.0f;

				HZ_MESH_LOG("    COLOR = {0}, {1}, {2}", aiColor.r, aiColor.g, aiColor.b);
				HZ_MESH_LOG("    ROUGHNESS = {0}", roughness);
				HZ_MESH_LOG("    METALNESS = {0}", metalness);
				bool hasAlbedoMap = aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiTexPath) == AI_SUCCESS;
				bool fallback = !hasAlbedoMap;
				if (hasAlbedoMap)
				{
					AssetHandle textureHandle = 0;
					TextureSpecification spec;
					spec.DebugName = aiTexPath.C_Str();
					spec.SRGB = true;
					if (auto aiTexEmbedded = scene->GetEmbeddedTexture(aiTexPath.C_Str()))
					{
						spec.Format = ImageFormat::RGBA;
						spec.Width = aiTexEmbedded->mWidth;
						spec.Height = aiTexEmbedded->mHeight;
						textureHandle = AssetManager::CreateMemoryOnlyRendererAsset<Texture2D>(spec, Buffer(aiTexEmbedded->pcData, 1));
					}
					else
					{
						// TODO: Temp - this should be handled by Hazel's filesystem
						auto parentPath = m_Path.parent_path();
						parentPath /= std::string(aiTexPath.data);
						std::string texturePath = parentPath.string();
						HZ_MESH_LOG("    Albedo map path = {0}", texturePath);
						textureHandle = AssetManager::CreateMemoryOnlyRendererAsset<Texture2D>(spec, texturePath);
					}

					Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(textureHandle);
					if (texture && texture->Loaded())
					{
						mi->Set("u_AlbedoTexture", texture);
						mi->Set("u_MaterialUniforms.AlbedoColor", glm::vec3(1.0f));
					}
					else
					{
						HZ_CORE_ERROR_TAG("Mesh", "Could not load texture: {0}", aiTexPath.C_Str());
						fallback = true;
					}
				}

				if (fallback)
				{
					HZ_MESH_LOG("    No albedo map");
					mi->Set("u_AlbedoTexture", whiteTexture);
				}

				// Normal maps
				bool hasNormalMap = aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiTexPath) == AI_SUCCESS;
				fallback = !hasNormalMap;
				if (hasNormalMap)
				{
					AssetHandle textureHandle = 0;
					
					TextureSpecification spec;
					spec.DebugName = aiTexPath.C_Str();
					if (auto aiTexEmbedded = scene->GetEmbeddedTexture(aiTexPath.C_Str()))
					{
						spec.Format = ImageFormat::RGB;
						spec.Width = aiTexEmbedded->mWidth;
						spec.Height = aiTexEmbedded->mHeight;
						textureHandle = AssetManager::CreateMemoryOnlyRendererAsset<Texture2D>(spec, Buffer(aiTexEmbedded->pcData, 1));
					}
					else
					{

						// TODO: Temp - this should be handled by Hazel's filesystem
						auto parentPath = m_Path.parent_path();
						parentPath /= std::string(aiTexPath.data);
						std::string texturePath = parentPath.string();
						HZ_MESH_LOG("    Normal map path = {0}", texturePath);
						textureHandle = AssetManager::CreateMemoryOnlyRendererAsset<Texture2D>(spec, texturePath);
					}

					Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(textureHandle);
					if (texture && texture->Loaded())
					{
						mi->Set("u_NormalTexture", texture);
						mi->Set("u_MaterialUniforms.UseNormalMap", true);
					}
					else
					{
						HZ_CORE_ERROR_TAG("Mesh", "    Could not load texture: {0}", aiTexPath.C_Str());
						fallback = true;
					}
				}

				if (fallback)
				{
					HZ_MESH_LOG("    No normal map");
					mi->Set("u_NormalTexture", whiteTexture);
					mi->Set("u_MaterialUniforms.UseNormalMap", false);
				}

				// Roughness map
				bool hasRoughnessMap = aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &aiTexPath) == AI_SUCCESS;
				fallback = !hasRoughnessMap;
				if (hasRoughnessMap)
				{
					AssetHandle textureHandle = 0;
					TextureSpecification spec;
					spec.DebugName = aiTexPath.C_Str();
					if (auto aiTexEmbedded = scene->GetEmbeddedTexture(aiTexPath.C_Str()))
					{
						spec.Format = ImageFormat::RGB;
						spec.Width = aiTexEmbedded->mWidth;
						spec.Height = aiTexEmbedded->mHeight;
						textureHandle = AssetManager::CreateMemoryOnlyRendererAsset<Texture2D>(spec, Buffer(aiTexEmbedded->pcData, 1));
					}
					else
					{
						// TODO: Temp - this should be handled by Hazel's filesystem
						auto parentPath = m_Path.parent_path();
						parentPath /= std::string(aiTexPath.data);
						std::string texturePath = parentPath.string();
						HZ_MESH_LOG("    Roughness map path = {0}", texturePath);
						textureHandle = AssetManager::CreateMemoryOnlyRendererAsset<Texture2D>(spec, texturePath);
					}

					Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(textureHandle);
					if (texture && texture->Loaded())
					{
						mi->Set("u_RoughnessTexture", texture);
						mi->Set("u_MaterialUniforms.Roughness", 1.0f);
					}
					else
					{
						HZ_CORE_ERROR_TAG("Mesh", "    Could not load texture: {0}", aiTexPath.C_Str());
						fallback = true;
					}
				}

				if (fallback)
				{
					HZ_MESH_LOG("    No roughness map");
					mi->Set("u_RoughnessTexture", whiteTexture);
					mi->Set("u_MaterialUniforms.Roughness", roughness);
				}

#if 0
				// Metalness map (or is it??)
				if (aiMaterial->Get("$raw.ReflectionFactor|file", aiPTI_String, 0, aiTexPath) == AI_SUCCESS)
				{
					// TODO: Temp - this should be handled by Hazel's filesystem
					std::filesystem::path path = filename;
					auto parentPath = path.parent_path();
					parentPath /= std::string(aiTexPath.data);
					std::string texturePath = parentPath.string();

					auto texture = Texture2D::Create(texturePath);
					if (texture->Loaded())
					{
						HZ_MESH_LOG("    Metalness map path = {0}", texturePath);
						mi->Set("u_MetalnessTexture", texture);
						mi->Set("u_MetalnessTexToggle", 1.0f);
					}
					else
					{
						HZ_CORE_ERROR_TAG("Mesh", "Could not load texture: {0}", texturePath);
					}
				}
				else
				{
					HZ_MESH_LOG("    No metalness texture");
					mi->Set("u_Metalness", metalness);
				}
#endif

				bool metalnessTextureFound = false;
				for (uint32_t p = 0; p < aiMaterial->mNumProperties; p++)
				{
					auto prop = aiMaterial->mProperties[p];

#if DEBUG_PRINT_ALL_PROPS
					HZ_MESH_LOG("Material Property:");
					HZ_MESH_LOG("  Name = {0}", prop->mKey.data);
					// HZ_MESH_LOG("  Type = {0}", prop->mType);
					// HZ_MESH_LOG("  Size = {0}", prop->mDataLength);
					float data = *(float*)prop->mData;
					HZ_MESH_LOG("  Value = {0}", data);

					switch (prop->mSemantic)
					{
						case aiTextureType_NONE:
							HZ_MESH_LOG("  Semantic = aiTextureType_NONE");
							break;
						case aiTextureType_DIFFUSE:
							HZ_MESH_LOG("  Semantic = aiTextureType_DIFFUSE");
							break;
						case aiTextureType_SPECULAR:
							HZ_MESH_LOG("  Semantic = aiTextureType_SPECULAR");
							break;
						case aiTextureType_AMBIENT:
							HZ_MESH_LOG("  Semantic = aiTextureType_AMBIENT");
							break;
						case aiTextureType_EMISSIVE:
							HZ_MESH_LOG("  Semantic = aiTextureType_EMISSIVE");
							break;
						case aiTextureType_HEIGHT:
							HZ_MESH_LOG("  Semantic = aiTextureType_HEIGHT");
							break;
						case aiTextureType_NORMALS:
							HZ_MESH_LOG("  Semantic = aiTextureType_NORMALS");
							break;
						case aiTextureType_SHININESS:
							HZ_MESH_LOG("  Semantic = aiTextureType_SHININESS");
							break;
						case aiTextureType_OPACITY:
							HZ_MESH_LOG("  Semantic = aiTextureType_OPACITY");
							break;
						case aiTextureType_DISPLACEMENT:
							HZ_MESH_LOG("  Semantic = aiTextureType_DISPLACEMENT");
							break;
						case aiTextureType_LIGHTMAP:
							HZ_MESH_LOG("  Semantic = aiTextureType_LIGHTMAP");
							break;
						case aiTextureType_REFLECTION:
							HZ_MESH_LOG("  Semantic = aiTextureType_REFLECTION");
							break;
						case aiTextureType_UNKNOWN:
							HZ_MESH_LOG("  Semantic = aiTextureType_UNKNOWN");
							break;
					}
#endif

					if (prop->mType == aiPTI_String)
					{
						uint32_t strLength = *(uint32_t*)prop->mData;
						std::string str(prop->mData + 4, strLength);

						std::string key = prop->mKey.data;
						if (key == "$raw.ReflectionFactor|file")
						{
							AssetHandle textureHandle = 0;
							TextureSpecification spec;
							spec.DebugName = str;
							if (auto aiTexEmbedded = scene->GetEmbeddedTexture(str.data()))
							{
								spec.Format = ImageFormat::RGB;
								spec.Width = aiTexEmbedded->mWidth;
								spec.Height = aiTexEmbedded->mHeight;
								textureHandle = AssetManager::CreateMemoryOnlyRendererAsset<Texture2D>(spec, Buffer(aiTexEmbedded->pcData, 1));
							}
							else
							{
								// TODO: Temp - this should be handled by Hazel's filesystem
								auto parentPath = m_Path.parent_path();
								parentPath /= str;
								std::string texturePath = parentPath.string();
								HZ_MESH_LOG("    Metalness map path = {0}", texturePath);
								textureHandle = AssetManager::CreateMemoryOnlyRendererAsset<Texture2D>(spec, texturePath);
							}

							Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(textureHandle);
							if (texture && texture->Loaded())
							{
								metalnessTextureFound = true;
								mi->Set("u_MetalnessTexture", texture);
								mi->Set("u_MaterialUniforms.Metalness", 1.0f);
							}
							else
							{
								HZ_CORE_ERROR_TAG("Mesh", "    Could not load texture: {0}", str);
							}
							break;
						}
					}
				}

				fallback = !metalnessTextureFound;
				if (fallback)
				{
					HZ_MESH_LOG("    No metalness map");
					mi->Set("u_MetalnessTexture", whiteTexture);
					mi->Set("u_MaterialUniforms.Metalness", metalness);

				}
			}
			HZ_MESH_LOG("------------------------");
		}
		else
		{
			auto mi = Material::Create(Renderer::GetShaderLibrary()->Get("HazelPBR_Static"), "Hazel-Default");
			mi->Set("u_MaterialUniforms.AlbedoColor", glm::vec3(0.8f));
			mi->Set("u_MaterialUniforms.Emission", 0.0f);
			mi->Set("u_MaterialUniforms.Metalness", 0.0f);
			mi->Set("u_MaterialUniforms.Roughness", 0.8f);
			mi->Set("u_MaterialUniforms.UseNormalMap", false);

			mi->Set("u_AlbedoTexture", whiteTexture);
			mi->Set("u_MetalnessTexture", whiteTexture);
			mi->Set("u_RoughnessTexture", whiteTexture);
			meshSource->m_Materials.push_back(mi);
		}


		if (meshSource->m_Vertices.size())
			meshSource->m_VertexBuffer = VertexBuffer::Create(meshSource->m_Vertices.data(), (uint32_t)(meshSource->m_Vertices.size() * sizeof(Vertex)));

		if (meshSource->HasSkeleton())
		{
			meshSource->m_BoneInfluenceBuffer = VertexBuffer::Create(meshSource->m_BoneInfluences.data(), (uint32_t)(meshSource->m_BoneInfluences.size() * sizeof(BoneInfluence)));
		}

		if (meshSource->m_Indices.size())
			meshSource->m_IndexBuffer = IndexBuffer::Create(meshSource->m_Indices.data(), (uint32_t)(meshSource->m_Indices.size() * sizeof(Index)));


		return meshSource;
	}

	bool AssimpMeshImporter::ImportSkeleton(Scope<Skeleton>& skeleton)
	{
		Assimp::Importer importer;
		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

		const aiScene* scene = importer.ReadFile(m_Path.string(), s_MeshImportFlags);
		if (!scene)
		{
			HZ_CORE_ERROR_TAG("Animation", "Failed to load mesh source file: {0}", m_Path.string());
			return false;
		}

		skeleton = AssimpAnimationImporter::ImportSkeleton(scene);
		return true;
	}

	bool AssimpMeshImporter::ImportAnimation(const uint32_t animationIndex, const Skeleton& skeleton, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask, Scope<Animation>& animation)
	{
		Assimp::Importer importer;
		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

		const aiScene* scene = importer.ReadFile(m_Path.string(), s_MeshImportFlags);
		if (!scene)
		{
			HZ_CORE_ERROR_TAG("Animation", "Failed to load mesh source file: {0}", m_Path.string());
			return false;
		}

		if (animationIndex >= scene->mNumAnimations)
		{
			HZ_CORE_ERROR_TAG("Animation", "Animation index {0} out of range for mesh source file: {1}", animationIndex, m_Path.string());
			return false;
		}
		
		animation = AssimpAnimationImporter::ImportAnimation(scene, animationIndex, skeleton, isMaskedRootMotion, rootTranslationMask, rootRotationMask);
		return true;
	}

	bool AssimpMeshImporter::IsCompatibleSkeleton(const uint32_t animationIndex, const Skeleton& skeleton)
	{
		Assimp::Importer importer;
		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

		const aiScene* scene = importer.ReadFile(m_Path.string(), s_MeshImportFlags);
		if (!scene)
		{
			HZ_CORE_ERROR_TAG("Mesh", "Failed to load mesh file: {0}", m_Path.string());
			return false;
		}

		if (scene->mNumAnimations <= animationIndex)
		{
			return false;
		}

		const auto animationNames = AssimpAnimationImporter::GetAnimationNames(scene);
		if (animationNames.empty())
		{
			return false;
		}

		const aiAnimation* anim = scene->mAnimations[animationIndex];

		std::unordered_map<std::string_view, uint32_t> boneIndices;
		for (uint32_t i = 0; i < skeleton.GetNumBones(); ++i)
		{
			boneIndices.emplace(skeleton.GetBoneName(i), i);
		}

		std::set<std::tuple<uint32_t, aiNodeAnim*>> validChannels;
		for (uint32_t channelIndex = 0; channelIndex < anim->mNumChannels; ++channelIndex)
		{
			aiNodeAnim* nodeAnim = anim->mChannels[channelIndex];
			auto it = boneIndices.find(nodeAnim->mNodeName.C_Str());
			if (it != boneIndices.end())
			{
				validChannels.emplace(it->second, nodeAnim);
			}
		}

		// It's hard to decide whether or not the animation is "valid" for the given skeleton.
		// For example an animation does not necessarily contain channels for all bones.
		// Conversely, some channels in the animation might not be for bones.
		// So, you cannot simply check number of valid channels == number of bones
		// And you cannot simply check number of invalid channels == 0
		// For now, I've just decided on a simple number of valid channels must be at least 1
		return validChannels.size() > 0;
	}

	uint32_t AssimpMeshImporter::GetAnimationCount()
	{
		Assimp::Importer importer;
		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

		const aiScene* scene = importer.ReadFile(m_Path.string(), s_MeshImportFlags);
		if (!scene)
		{
			HZ_CORE_ERROR_TAG("Mesh", "Failed to load mesh file: {0}", m_Path.string());
			return false;
		}

		return (uint32_t)scene->mNumAnimations;
	}

	void AssimpMeshImporter::TraverseNodes(Ref<MeshSource> meshSource, void* assimpNode, uint32_t nodeIndex, const glm::mat4& parentTransform, uint32_t level)
	{
		aiNode* aNode = (aiNode*)assimpNode;

		MeshNode& node = meshSource->m_Nodes[nodeIndex];
		node.Name = aNode->mName.C_Str();
		node.LocalTransform = Utils::Mat4FromAIMatrix4x4(aNode->mTransformation);

		glm::mat4 transform = parentTransform * node.LocalTransform;
		for (uint32_t i = 0; i < aNode->mNumMeshes; i++)
		{
			uint32_t submeshIndex = aNode->mMeshes[i];
			auto& submesh = meshSource->m_Submeshes[submeshIndex];
			submesh.NodeName = aNode->mName.C_Str();
			submesh.Transform = transform;
			submesh.LocalTransform = node.LocalTransform;

			node.Submeshes.push_back(submeshIndex);
		}

		// HZ_MESH_LOG("{0} {1}", LevelToSpaces(level), node->mName.C_Str());

		uint32_t parentNodeIndex = (uint32_t)meshSource->m_Nodes.size() - 1;
		node.Children.resize(aNode->mNumChildren);
		for (uint32_t i = 0; i < aNode->mNumChildren; i++)
		{
			MeshNode& child = meshSource->m_Nodes.emplace_back();
			size_t childIndex = meshSource->m_Nodes.size() - 1;
			child.Parent = parentNodeIndex;
			meshSource->m_Nodes[nodeIndex].Children[i] = childIndex;
			TraverseNodes(meshSource, aNode->mChildren[i], uint32_t(childIndex), transform, level + 1);
		}
	}


}

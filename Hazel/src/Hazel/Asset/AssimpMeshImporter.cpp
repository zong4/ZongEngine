#include "hzpch.h"
#include "AssimpMeshImporter.h"

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Asset/AssimpAnimationImporter.h"
#include "Hazel/Asset/TextureImporter.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Utilities/AssimpLogStream.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

namespace Hazel {

#define MESH_DEBUG_LOG 0

#if MESH_DEBUG_LOG
#define DEBUG_PRINT_ALL_PROPS 1
#define HZ_MESH_LOG(...) HZ_CORE_TRACE_TAG("Mesh", __VA_ARGS__)
#define HZ_MESH_ERROR(...) HZ_CORE_ERROR_TAG("Mesh", __VA_ARGS__)
#else
#define HZ_MESH_LOG(...)
#define HZ_MESH_ERROR(...)
#endif

	static const uint32_t s_MeshImportFlags =
		aiProcess_CalcTangentSpace          // Create binormals/tangents just in case
		| aiProcess_Triangulate             // Make sure we're triangles
		| aiProcess_SortByPType             // Split meshes by primitive type
		| aiProcess_GenNormals              // Make sure we have legit normals
		| aiProcess_GenUVCoords             // Convert UVs if required 
//		| aiProcess_OptimizeGraph
		| aiProcess_OptimizeMeshes          // Batch draws where possible
		| aiProcess_JoinIdenticalVertices
		| aiProcess_LimitBoneWeights        // If more than N (=4) bone weights, discard least influencing bones and renormalise sum to 1
		| aiProcess_ValidateDataStructure   // Validation
		| aiProcess_GlobalScale             // e.g. convert cm to m for fbx import (and other formats where cm is native)
		;
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
			glm::quat rotationQuat;
			glm::vec3 scale;
			glm::mat4 transform = Mat4FromAIMatrix4x4(node->mTransformation);
			Math::DecomposeTransform(transform, translation, rotationQuat, scale);
			glm::vec3 rotation = glm::degrees(glm::eulerAngles(rotationQuat));

			HZ_MESH_LOG("{0:^{1}}translation: ({2:8.4f}, {3:8.4f}, {4:8.4f})", "", depth * 3, translation.x, translation.y, translation.z);
			HZ_MESH_LOG("{0:^{1}}rotation:    ({2:8.4f}, {3:8.4f}, {4:8.4f})", "", depth * 3, rotation.x, rotation.y, rotation.z);
			HZ_MESH_LOG("{0:^{1}}scale:       ({2:8.4f}, {3:8.4f}, {4:8.4f})", "", depth * 3, scale.x, scale.y, scale.z);
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

		// Actual load of the animations is deferred until later.
		// Because:
		// 1. If there is no skin (mesh), then assimp will not have loaded the skeleton, and we cannot
		//    load the animations until we know what the skeleton is
		// 2. Loading the animation requires some extra parameters to control how to import the root motion
		//    This constructor has no way of knowing what those parameters are.
		meshSource->m_AnimationNames = AssimpAnimationImporter::GetAnimationNames(scene);
		meshSource->m_Animations = std::vector<Scope<Animation>>(meshSource->m_AnimationNames.size());

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

				if (!mesh->HasPositions())
				{
					HZ_CONSOLE_LOG_WARN("Mesh index {0} with name '{1}' has no vertex positions - skipping import!", m, mesh->mName.C_Str());
				}
				if (!mesh->HasNormals())
				{
					HZ_CONSOLE_LOG_WARN("Mesh index {0} with name '{1}' has no vertex normals, and they could not be computed - skipping import!", m, mesh->mName.C_Str());
				}

				bool skip = !mesh->HasPositions() || !mesh->HasNormals();

				// still have to create a placeholder submesh even if we are skipping it (otherwise TraverseNodes() does not work)
				Submesh& submesh = meshSource->m_Submeshes.emplace_back();
				submesh.BaseVertex = vertexCount;
				submesh.BaseIndex = indexCount;
				submesh.MaterialIndex = mesh->mMaterialIndex;
				submesh.VertexCount = skip? 0 : mesh->mNumVertices;
				submesh.IndexCount = skip? 0 : mesh->mNumFaces * 3;
				submesh.MeshName = mesh->mName.C_Str();

				if (skip) continue;

				vertexCount += mesh->mNumVertices;
				indexCount += submesh.IndexCount;

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
					// we're using aiProcess_Triangulate so this should always be true
					HZ_CORE_ASSERT(mesh->mFaces[i].mNumIndices == 3, "Must have 3 indices.");
					Index index = { mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1], mesh->mFaces[i].mIndices[2] };
					meshSource->m_Indices.push_back(index);

					meshSource->m_TriangleCache[m].emplace_back(meshSource->m_Vertices[index.V1 + submesh.BaseVertex], meshSource->m_Vertices[index.V2 + submesh.BaseVertex], meshSource->m_Vertices[index.V3 + submesh.BaseVertex]);
				}
			}

#if MESH_DEBUG_LOG
			HZ_CORE_INFO_TAG("Mesh", "Traversing nodes for scene '{0}'", m_Path);
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

		// skinning weights
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
							if (meshSource->m_BoneInfo[j].BoneIndex == boneIndex)
							{
								boneInfoIndex = static_cast<uint32_t>(j);
								break;
							}
						}
						if (boneInfoIndex == ~0)
						{
							boneInfoIndex = static_cast<uint32_t>(meshSource->m_BoneInfo.size());
							const auto& boneInfo = meshSource->m_BoneInfo.emplace_back(Utils::Mat4FromAIMatrix4x4(bone->mOffsetMatrix), boneIndex);
#if MESH_DEBUG_LOG
							HZ_CORE_INFO_TAG("Mesh", "BoneInfo for bone '{0}'", bone->mName.C_Str());
							HZ_CORE_INFO_TAG("Mesh", "  SubMeshIndex = {0}", m);
							HZ_CORE_INFO_TAG("Mesh", "  BoneIndex = {0}", boneIndex);

							glm::vec3 translation;
							glm::quat rotationQuat;
							glm::vec3 scale;
							Math::DecomposeTransform(boneInfo.InverseBindPose, translation, rotationQuat, scale);
							glm::vec3 rotation = glm::degrees(glm::eulerAngles(rotationQuat));
							HZ_CORE_INFO_TAG("Mesh", "  Inverse Bind Pose = {");
							HZ_MESH_LOG("    translation: ({0:8.4f}, {1:8.4f}, {2:8.4f})", translation.x, translation.y, translation.z);
							HZ_MESH_LOG("    rotation:    ({0:8.4f}, {1:8.4f}, {2:8.4f})", rotation.x, rotation.y, rotation.z);
							HZ_MESH_LOG("    scale:       ({0:8.4f}, {1:8.4f}, {2:8.4f})", scale.x, scale.y, scale.z);
							HZ_MESH_LOG("  }");
#endif
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
			HZ_MESH_LOG("---- Materials - {0} ----", m_Path);

			meshSource->m_Materials.resize(scene->mNumMaterials);

			for (uint32_t i = 0; i < scene->mNumMaterials; i++)
			{
				auto aiMaterial = scene->mMaterials[i];
				auto aiMaterialName = aiMaterial->GetName();
				Ref<Material> material = Material::Create(Renderer::GetShaderLibrary()->Get("HazelPBR_Static"), aiMaterialName.data);
				auto ma = Ref<MaterialAsset>::Create(material);

				HZ_MESH_LOG("  {0} (Index = {1})", aiMaterialName.data, i);

#if DEBUG_PRINT_ALL_PROPS
				for (uint32_t p = 0; p < aiMaterial->mNumProperties; p++)
				{
					auto prop = aiMaterial->mProperties[p];

					HZ_MESH_LOG("Material Property:");
					HZ_MESH_LOG("  Name = {0}", prop->mKey.data);
					HZ_MESH_LOG("  Type = {0}", (int)prop->mType);
					HZ_MESH_LOG("  Size = {0}", prop->mDataLength);
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
				}
#endif

				aiString aiTexPath;

				glm::vec3 albedoColor(0.8f);
				float emission = 0.0f;
				aiColor3D aiColor, aiEmission;
				if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor) == AI_SUCCESS)
					albedoColor = { aiColor.r, aiColor.g, aiColor.b };

				if (aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, aiEmission) == AI_SUCCESS)
					emission = aiEmission.r;

				ma->SetAlbedoColor(albedoColor);
				ma->SetEmission(emission);

				float roughness, metalness;
				if (aiMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) != aiReturn_SUCCESS)
					roughness = 0.4f; // Default value

				if (aiMaterial->Get(AI_MATKEY_REFLECTIVITY, metalness) != aiReturn_SUCCESS)
					metalness = 0.0f;

				// Physically realistic materials are either metal (1.0) or not (0.0)
				// Some models seem to come in with 0.5 which seems wrong - materials are either metal or they are not.
				// (maybe these are specular workflow, and what we're seeing is specular = 0.5 in AI_MATKEY_REFLECTIVITY (?))
				if (metalness < 0.9f)
					metalness = 0.0f;
				else
					metalness = 1.0f;

				ma->SetRoughness(roughness);
				ma->SetMetalness(metalness);

				HZ_MESH_LOG("    COLOR = {0}, {1}, {2}", aiColor.r, aiColor.g, aiColor.b);
				HZ_MESH_LOG("    ROUGHNESS = {0}", roughness);
				HZ_MESH_LOG("    METALNESS = {0}", metalness);
				bool hasAlbedoMap = aiMaterial->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &aiTexPath) == AI_SUCCESS;
				if (!hasAlbedoMap)
				{
					// no PBR base color. Try old-school diffuse  (note: should probably combine with specular in this case)
					hasAlbedoMap = aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiTexPath) == AI_SUCCESS;
				}
				if (hasAlbedoMap)
				{
					AssetHandle textureHandle = 0;
					TextureSpecification spec;
					spec.DebugName = aiTexPath.C_Str();
					spec.Format = ImageFormat::SRGBA;
					if (auto aiTexEmbedded = scene->GetEmbeddedTexture(aiTexPath.C_Str()))
					{
						spec.Width = aiTexEmbedded->mWidth;
						spec.Height = aiTexEmbedded->mHeight;
						textureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, Buffer(aiTexEmbedded->pcData, 1)));
					}
					else
					{
						// TODO: Temp - this should be handled by Hazel's filesystem
						// NOTE(Yan): we probably shouldn't make this a memory-only asset, since this
						//            should already exist within the asset registry as a texture asset
						auto parentPath = m_Path.parent_path();
						auto texturePath = parentPath / aiTexPath.C_Str();
						if (!std::filesystem::exists(texturePath))
						{
							HZ_MESH_LOG("    Albedo map path = {0} --> NOT FOUND", texturePath);
							texturePath = parentPath / texturePath.filename();
						}
						HZ_MESH_LOG("    Albedo map path = {0}{1}", texturePath, std::filesystem::exists(texturePath)? "" : " --> NOT FOUND");
						textureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, texturePath));
					}

					ma->SetAlbedoMap(textureHandle);
					ma->SetAlbedoColor(glm::vec3(1.0f));
				}

				// Normal maps
				bool hasNormalMap = aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiTexPath) == AI_SUCCESS;
				if (hasNormalMap)
				{
					AssetHandle textureHandle = 0;
					
					TextureSpecification spec;
					spec.DebugName = aiTexPath.C_Str();
					if (auto aiTexEmbedded = scene->GetEmbeddedTexture(aiTexPath.C_Str()))
					{
						spec.Format = ImageFormat::RGBA;
						spec.Width = aiTexEmbedded->mWidth;
						spec.Height = aiTexEmbedded->mHeight;
						textureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, Buffer(aiTexEmbedded->pcData, 1)));
					}
					else
					{
						// TODO: Temp - this should be handled by Hazel's filesystem
						auto parentPath = m_Path.parent_path();
						auto texturePath = parentPath / aiTexPath.C_Str();
						if (!std::filesystem::exists(texturePath))
						{
							HZ_MESH_LOG("    Normal map path = {0} --> NOT FOUND", texturePath);
							texturePath = parentPath / texturePath.filename();
						}
						HZ_MESH_LOG("    Normal map path = {0}{1}", texturePath, std::filesystem::exists(texturePath) ? "" : " --> NOT FOUND");
						textureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, texturePath));
					}

					ma->SetNormalMap(textureHandle);
					// TODO(Yan): this needs to be false if it didn't work?
					ma->SetUseNormalMap(true);

				}

				// Roughness map
				bool hasRoughnessMap = aiMaterial->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &aiTexPath) == AI_SUCCESS;
				bool invertRoughness = false;
				if (!hasRoughnessMap)
				{
					// no PBR roughness. Try old-school shininess.  (note: this also picks up the gloss texture from PBR specular/gloss workflow).
					// Either way, Roughness = (1 - shininess)
					hasRoughnessMap = aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &aiTexPath) == AI_SUCCESS;
					invertRoughness = true;
				}

				AssetHandle roughnessTextureHandle = 0;
				if (hasRoughnessMap)
				{
					TextureSpecification spec;
					spec.DebugName = aiTexPath.C_Str();
					if (auto aiTexEmbedded = scene->GetEmbeddedTexture(aiTexPath.C_Str()))
					{
						spec.Format = ImageFormat::RGBA;
						spec.Width = aiTexEmbedded->mWidth;
						spec.Height = aiTexEmbedded->mHeight;
						aiTexel* texels = aiTexEmbedded->pcData;
						if (invertRoughness)
						{
							if (spec.Height == 0)
							{
								auto buffer = TextureImporter::ToBufferFromMemory(Buffer(aiTexEmbedded->pcData, spec.Width), spec.Format, spec.Width, spec.Height);
								texels = (aiTexel*)buffer.Data;
							}
							for(uint32_t i = 0; i < spec.Width * spec.Height; ++i)
							{
								auto& texel = texels[i];
								texel.r = 255 - texel.r;
								texel.g = 255 - texel.g;
								texel.b = 255 - texel.b;
							}
						}
						roughnessTextureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, Buffer(texels, 1)));
					}
					else
					{
						// TODO: Temp - this should be handled by Hazel's filesystem
						auto parentPath = m_Path.parent_path();
						auto texturePath = parentPath / aiTexPath.C_Str();
						if (!std::filesystem::exists(texturePath))
						{
							HZ_MESH_LOG("    Roughness map path = {0} --> NOT FOUND", texturePath);
							texturePath = parentPath / texturePath.filename();
						}
						HZ_MESH_LOG("    Roughness map path = {0}{1}", texturePath, std::filesystem::exists(texturePath) ? "" : " --> NOT FOUND");
						auto buffer = TextureImporter::ToBufferFromFile(texturePath, spec.Format, spec.Width, spec.Height);
						aiTexel* texels = (aiTexel*)buffer.Data;
						if(invertRoughness)
						{
							for(uint32_t i = 0; i < spec.Width * spec.Height; i+= 4)
							{
								aiTexel& texel = texels[i];
								texel.r = 255 - texel.r;
								texel.g = 255 - texel.g;
								texel.b = 255 - texel.b;
							}
						}
						roughnessTextureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, buffer));
					}

					ma->SetRoughnessMap(roughnessTextureHandle);
					ma->SetRoughness(1.0f);
				}

#if 0
				// Metalness map (or is it??)
				if (aiMaterial->Get("$raw.ReflectionFactor|file", aiPTI_String, 0, aiTexPath) == AI_SUCCESS)
				{
					// TODO: Temp - this should be handled by Hazel's filesystem
					auto parentPath = m_Path.parent_path();
					auto texturePath = parentPath / aiTexPath.C_Str();
					if (!std::filesystem::exists(texturePath))
					{
						HZ_MESH_LOG("    Metalness map path = {0} --> NOT FOUND", texturePath);
						texturePath = parentPath / texturePath.filename();
					}
					HZ_MESH_LOG("    Metalness map path = {0}{1}", texturePath, std::filesystem::exists(texturePath) ? "" : " --> NOT FOUND");
					auto texture = Texture2D::Create(texturePath);
					if (texture->Loaded())
					{
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

#if 0
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
								auto texturePath = parentPath / aiTexPath.C_Str();
								if (!std::filesystem::exists(texturePath))
								{
									HZ_MESH_LOG("    Metalness map path = {0} --> NOT FOUND", texturePath);
									texturePath = parentPath / texturePath.filename();
								}
								HZ_MESH_LOG("    Metalness map path = {0}{1}", texturePath, std::filesystem::exists(texturePath) ? "" : " --> NOT FOUND");
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

				// no metalness texture found, use roughness if there was one (the shaders read roughness from the texture's G channel, and metalness from B)
				if (hasRoughnessMap)
				{
					Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(roughnessTextureHandle);
					if (texture && texture->Loaded())
					{
						mi->Set("u_MetalnessTexture", texture);
						mi->Set("u_MaterialUniforms.Metalness", 1.0f);
					}
				}
				fallback = !hasRoughnessMap;
				if (fallback)
				{
					HZ_MESH_LOG("    No metalness map");
					mi->Set("u_MetalnessTexture", whiteTexture);
					mi->Set("u_MaterialUniforms.Metalness", metalness);

				}
#endif

				// Metalness map
				bool hasMetalnessMap = aiMaterial->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &aiTexPath) == AI_SUCCESS;
				AssetHandle metalnessTextureHandle = 0;
				if (hasMetalnessMap)
				{
					TextureSpecification spec;
					spec.DebugName = aiTexPath.C_Str();
					if (auto aiTexEmbedded = scene->GetEmbeddedTexture(aiTexPath.C_Str()))
					{
						spec.Format = ImageFormat::RGB;
						spec.Width = aiTexEmbedded->mWidth;
						spec.Height = aiTexEmbedded->mHeight;
						metalnessTextureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, Buffer(aiTexEmbedded->pcData, 1)));
					}
					else
					{
						// TODO: Temp - this should be handled by Hazel's filesystem
						auto parentPath = m_Path.parent_path();
						auto texturePath = parentPath / aiTexPath.C_Str();
						if (!std::filesystem::exists(texturePath))
						{
							HZ_MESH_LOG("    Roughness map path = {0} --> NOT FOUND", texturePath);
							texturePath = parentPath / texturePath.filename();
						}
						HZ_MESH_LOG("    Roughness map path = {0}{1}", texturePath, std::filesystem::exists(texturePath) ? "" : " --> NOT FOUND");
						metalnessTextureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, texturePath));
					}

					ma->SetMetalnessMap(metalnessTextureHandle);
					ma->SetMetalness(1.0f);
				}

				AssetHandle maHandle = AssetManager::AddMemoryOnlyAsset(ma);
				meshSource->m_Materials[i] = maHandle;

			}
			HZ_MESH_LOG("------------------------");
		}
		else
		{
			if (scene->HasMeshes())
			{
				Ref<Material> material = Material::Create(Renderer::GetShaderLibrary()->Get("HazelPBR_Static"), "Hazel-Default");
				AssetHandle maHandle = AssetManager::AddMemoryOnlyAsset(Ref<MaterialAsset>::Create(material));
				meshSource->m_Materials.push_back(maHandle);
			}
		}

		if (meshSource->m_Vertices.size())
			meshSource->m_VertexBuffer = VertexBuffer::Create(meshSource->m_Vertices.data(), (uint32_t)(meshSource->m_Vertices.size() * sizeof(Vertex)));

		if (meshSource->m_BoneInfluences.size() > 0)
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

	bool AssimpMeshImporter::ImportAnimation(const std::string_view animationName, const Skeleton& skeleton, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask, Scope<Animation>& animation)
	{
		Assimp::Importer importer;
		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

		const aiScene* scene = importer.ReadFile(m_Path.string(), s_MeshImportFlags);
		if (!scene)
		{
			HZ_CORE_ERROR_TAG("Animation", "Failed to load mesh source file: {0}", m_Path.string());
			return false;
		}

		uint32_t animationIndex = AssimpAnimationImporter::GetAnimationIndex(scene, animationName);

		if (animationIndex == ~0)
		{
			HZ_CORE_ERROR_TAG("Animation", "Animation '{0}' not found in mesh source file: {1}", animationName, m_Path.string());
			return false;
		}
		
		animation = AssimpAnimationImporter::ImportAnimation(scene, animationIndex, skeleton, isMaskedRootMotion, rootTranslationMask, rootRotationMask);
		return true;
	}

	bool AssimpMeshImporter::IsCompatibleSkeleton(std::string_view animationName, const Skeleton& skeleton)
	{
		Assimp::Importer importer;
		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

		const aiScene* scene = importer.ReadFile(m_Path.string(), s_MeshImportFlags);
		if (!scene)
		{
			HZ_CORE_ERROR_TAG("Mesh", "Failed to load mesh file: {0}", m_Path.string());
			return false;
		}

		uint32_t animationIndex = AssimpAnimationImporter::GetAnimationIndex(scene, animationName);
		if (animationIndex == ~0)
		{
			HZ_CORE_ERROR_TAG("Animation", "Animation '{0}' not found in mesh source file: {1}", animationName, m_Path.string());
			return false;
		}

		std::unordered_map<std::string_view, uint32_t> boneIndices;
		for (uint32_t i = 0; i < skeleton.GetNumBones(); ++i)
		{
			boneIndices.emplace(skeleton.GetBoneName(i), i);
		}

		std::set<std::tuple<uint32_t, aiNodeAnim*>> validChannels;
		const aiAnimation* anim = scene->mAnimations[animationIndex];
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
			uint32_t childIndex = static_cast<uint32_t>(meshSource->m_Nodes.size()) - 1;
			child.Parent = parentNodeIndex;
			meshSource->m_Nodes[nodeIndex].Children[i] = childIndex;
			TraverseNodes(meshSource, aNode->mChildren[i], childIndex, transform, level + 1);
		}
	}


}

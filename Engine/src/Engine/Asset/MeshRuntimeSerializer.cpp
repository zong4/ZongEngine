#include "pch.h"
#include "MeshRuntimeSerializer.h"

#include "MeshSourceFile.h"

#include "Engine/Asset/AssetManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Asset/AssimpMeshImporter.h"

namespace Engine {

	struct MeshMaterial
	{
		std::string MaterialName;
		std::string ShaderName;

		glm::vec3 AlbedoColor;
		float Emission;
		float Metalness;
		float Roughness;
		bool UseNormalMap;

		uint64_t AlbedoTexture;
		uint64_t NormalTexture;
		uint64_t MetalnessTexture;
		uint64_t RoughnessTexture;

		static void Serialize(StreamWriter* serializer, const MeshMaterial& instance)
		{
			serializer->WriteString(instance.MaterialName);
			serializer->WriteString(instance.ShaderName);

			serializer->WriteRaw(instance.AlbedoColor);
			serializer->WriteRaw(instance.Emission);
			serializer->WriteRaw(instance.Metalness);
			serializer->WriteRaw(instance.Roughness);
			serializer->WriteRaw(instance.UseNormalMap);
			
			serializer->WriteRaw(instance.AlbedoTexture);
			serializer->WriteRaw(instance.NormalTexture);
			serializer->WriteRaw(instance.MetalnessTexture);
			serializer->WriteRaw(instance.RoughnessTexture);
		}

		static void Deserialize(StreamReader* deserializer, MeshMaterial& instance)
		{
			deserializer->ReadString(instance.MaterialName);
			deserializer->ReadString(instance.ShaderName);

			deserializer->ReadRaw(instance.AlbedoColor);
			deserializer->ReadRaw(instance.Emission);
			deserializer->ReadRaw(instance.Metalness);
			deserializer->ReadRaw(instance.Roughness);
			deserializer->ReadRaw(instance.UseNormalMap);

			deserializer->ReadRaw(instance.AlbedoTexture);
			deserializer->ReadRaw(instance.NormalTexture);
			deserializer->ReadRaw(instance.MetalnessTexture);
			deserializer->ReadRaw(instance.RoughnessTexture);
		}
	};

	static void Serialize(StreamWriter* serializer, const Skeleton& skeleton)
	{
		serializer->WriteArray(skeleton.GetBoneNames());
		serializer->WriteArray(skeleton.GetParentBoneIndices());
		serializer->WriteArray(skeleton.GetBoneTranslations());
		serializer->WriteArray(skeleton.GetBoneRotations());
		serializer->WriteArray(skeleton.GetBoneScales());
	}

	static void Deserialize(StreamReader* deserializer, Skeleton& skeleton)
	{
		std::vector<std::string> boneNames;
		std::vector<uint32_t> parentBoneIndices;
		std::vector<glm::vec3> boneTranslations;
		std::vector<glm::quat> boneRotations;
		std::vector<glm::vec3> boneScales;

		deserializer->ReadArray(boneNames);
		deserializer->ReadArray(parentBoneIndices);
		deserializer->ReadArray(boneTranslations);
		deserializer->ReadArray(boneRotations);
		deserializer->ReadArray(boneScales);

		skeleton.SetBones(boneNames, parentBoneIndices, boneTranslations, boneRotations, boneScales);
	}

	static void Serialize(StreamWriter* serializer, const Animation& animation)
	{
		serializer->WriteRaw(animation.GetDuration());
		serializer->WriteRaw(animation.GetNumTracks());
		serializer->WriteArray(animation.GetTranslationKeys());
		serializer->WriteArray(animation.GetRotationKeys());
		serializer->WriteArray(animation.GetScaleKeys());
	}

	static void Deserialize(StreamReader* deserializer, Animation& animation)
	{
		uint32_t numTracks;
		float duration;
		std::vector<TranslationKey> translationKeys;
		std::vector<RotationKey> rotationKeys;
		std::vector<ScaleKey> scaleKeys;

		deserializer->ReadRaw(duration);
		deserializer->ReadRaw(numTracks);
		deserializer->ReadArray(translationKeys);
		deserializer->ReadArray(rotationKeys);
		deserializer->ReadArray(scaleKeys);

		animation = Animation(duration, numTracks);
		animation.SetKeys(std::move(translationKeys), std::move(rotationKeys), std::move(scaleKeys));
	}

	bool MeshRuntimeSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		outInfo.Offset = stream.GetStreamPosition();

		uint64_t streamOffset = stream.GetStreamPosition();

		Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(handle);

		MeshSourceFile file;
		
		bool hasMaterials = !meshSource->GetMaterials().empty();

		// The meshSource might contain some animations.  However, unless they are actually used in a scene they will not have been loaded (animation pointer is null)
		// In this case we are not interested in serializing them for runtime.
		size_t animationCount = std::count_if(std::begin(meshSource->m_Animations), std::end(meshSource->m_Animations), [](const auto& animation) { return animation != nullptr; });
		bool hasAnimation = animationCount != 0;

		bool hasSkeleton = meshSource->HasSkeleton();

		file.Data.Flags = 0;
		if (hasMaterials)
			file.Data.Flags |= (uint32_t)MeshSourceFile::MeshFlags::HasMaterials;
		if (hasAnimation)
			file.Data.Flags |= (uint32_t)MeshSourceFile::MeshFlags::HasAnimation;
		if (hasSkeleton)
			file.Data.Flags |= (uint32_t)MeshSourceFile::MeshFlags::HasSkeleton;

		// Write header
		stream.WriteRaw<MeshSourceFile::FileHeader>(file.Header);
		// Leave space for Metadata
		uint64_t metadataAbsolutePosition = stream.GetStreamPosition();
		stream.WriteZero(sizeof(MeshSourceFile::Metadata));

		// Write nodes
		file.Data.NodeArrayOffset = stream.GetStreamPosition() - streamOffset;
		stream.WriteArray(meshSource->m_Nodes);
		file.Data.NodeArraySize = (stream.GetStreamPosition() - streamOffset) - file.Data.NodeArrayOffset;

		// Write submeshes
		file.Data.SubmeshArrayOffset = stream.GetStreamPosition() - streamOffset;
		stream.WriteArray(meshSource->m_Submeshes);
		file.Data.SubmeshArraySize = (stream.GetStreamPosition() - streamOffset) - file.Data.SubmeshArrayOffset;

		// Write Material Buffer
		if (hasMaterials)
		{
			// Prepare materials
			std::vector<MeshMaterial> meshMaterials(meshSource->GetMaterials().size());
			const auto& meshSourceMaterials = meshSource->GetMaterials();
			for (size_t i = 0; i < meshMaterials.size(); i++)
			{
				MeshMaterial& material = meshMaterials[i];
				Ref<Material> meshSourceMaterial = meshSourceMaterials[i];

				material.MaterialName = meshSourceMaterial->GetName();
				material.ShaderName = meshSourceMaterial->GetShader()->GetName();

				material.AlbedoColor = meshSourceMaterial->GetVector3("u_MaterialUniforms.AlbedoColor");
				material.Emission = meshSourceMaterial->GetFloat("u_MaterialUniforms.Emission");
				material.Metalness = meshSourceMaterial->GetFloat("u_MaterialUniforms.Metalness");
				material.Roughness = meshSourceMaterial->GetFloat("u_MaterialUniforms.Roughness");
				material.UseNormalMap = meshSourceMaterial->GetBool("u_MaterialUniforms.UseNormalMap");

				auto albedoTexture = meshSourceMaterial->GetTexture2D("u_AlbedoTexture");
				auto normalTexture = meshSourceMaterial->GetTexture2D("u_NormalTexture");
				auto metalnessTexture = meshSourceMaterial->GetTexture2D("u_MetalnessTexture");
				auto roughnessTexture = meshSourceMaterial->GetTexture2D("u_RoughnessTexture");

				material.AlbedoTexture = albedoTexture ? albedoTexture->Handle : AssetHandle(0);
				material.NormalTexture = normalTexture ? normalTexture->Handle : AssetHandle(0);
				material.MetalnessTexture = metalnessTexture ? metalnessTexture->Handle : AssetHandle(0);
				material.RoughnessTexture = roughnessTexture ? roughnessTexture->Handle : AssetHandle(0);
			}

			// Write materials
			file.Data.MaterialArrayOffset = stream.GetStreamPosition() - streamOffset;
			stream.WriteArray(meshMaterials);
			file.Data.MaterialArraySize = (stream.GetStreamPosition() - streamOffset) - file.Data.MaterialArrayOffset;
		}
		else
		{
			// No materials
			file.Data.MaterialArrayOffset = 0;
			file.Data.MaterialArraySize = 0;
		}
		
		// Write Vertex Buffer
		file.Data.VertexBufferOffset = stream.GetStreamPosition() - streamOffset;
		stream.WriteArray(meshSource->m_Vertices);
		file.Data.VertexBufferSize = (stream.GetStreamPosition() - streamOffset) - file.Data.VertexBufferOffset;

		// Write Index Buffer
		file.Data.IndexBufferOffset = stream.GetStreamPosition() - streamOffset;
		stream.WriteArray(meshSource->m_Indices);
		file.Data.IndexBufferSize = (stream.GetStreamPosition() - streamOffset) - file.Data.IndexBufferOffset;

		// Write Animation Data
		if (hasAnimation || hasSkeleton)
		{
			file.Data.AnimationDataOffset = stream.GetStreamPosition() - streamOffset;

			if (hasSkeleton)
			{
				stream.WriteArray(meshSource->m_BoneInfluences);
				stream.WriteArray(meshSource->m_BoneInfo);
				Serialize(&stream, *meshSource->m_Skeleton);
			}

			stream.WriteRaw((uint32_t)animationCount);
			for (Scope<Animation>& animation : meshSource->m_Animations)
				if (animation)
					Serialize(&stream, *animation);

			file.Data.AnimationDataSize = (stream.GetStreamPosition() - streamOffset) - file.Data.AnimationDataOffset;
		}
		else
		{
			file.Data.AnimationDataOffset = 0;
			file.Data.AnimationDataSize = 0;
		}

		// Write Metadata
		uint64_t endOfStream = stream.GetStreamPosition();
		stream.SetStreamPosition(metadataAbsolutePosition);
		stream.WriteRaw<MeshSourceFile::Metadata>(file.Data);
		stream.SetStreamPosition(endOfStream);

		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> MeshRuntimeSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo)
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		uint64_t streamOffset = stream.GetStreamPosition();

		MeshSourceFile file;
		stream.ReadRaw<MeshSourceFile::FileHeader>(file.Header);
		bool validHeader = memcmp(file.Header.HEADER, "HZMS", 4) == 0;
		ZONG_CORE_ASSERT(validHeader);
		if (!validHeader)
			return nullptr;

		Ref<MeshSource> meshSource = Ref<MeshSource>::Create();
		meshSource->m_Runtime = true;

		stream.ReadRaw<MeshSourceFile::Metadata>(file.Data);

		const auto& metadata = file.Data;
		bool hasMaterials = metadata.Flags & (uint32_t)MeshSourceFile::MeshFlags::HasMaterials;
		bool hasAnimation = metadata.Flags & (uint32_t)MeshSourceFile::MeshFlags::HasAnimation;
		bool hasSkeleton = metadata.Flags & (uint32_t)MeshSourceFile::MeshFlags::HasSkeleton;

		stream.SetStreamPosition(metadata.NodeArrayOffset + streamOffset);
		stream.ReadArray(meshSource->m_Nodes);
		stream.SetStreamPosition(metadata.SubmeshArrayOffset + streamOffset);
		stream.ReadArray(meshSource->m_Submeshes);

		if (hasMaterials)
		{
			std::vector<MeshMaterial> meshMaterials;
			stream.SetStreamPosition(metadata.MaterialArrayOffset + streamOffset);
			stream.ReadArray(meshMaterials);

			meshSource->m_Materials.resize(meshMaterials.size());
			for (size_t i = 0; i < meshMaterials.size(); i++)
			{
				const auto& meshMaterial = meshMaterials[i];
				Ref<Material> material = Material::Create(Renderer::GetShaderLibrary()->Get(meshMaterial.ShaderName), meshMaterial.MaterialName);

				material->Set("u_MaterialUniforms.AlbedoColor", meshMaterial.AlbedoColor);
				material->Set("u_MaterialUniforms.Emission", meshMaterial.Emission);
				material->Set("u_MaterialUniforms.Metalness", meshMaterial.Metalness);
				material->Set("u_MaterialUniforms.Roughness", meshMaterial.Roughness);
				material->Set("u_MaterialUniforms.UseNormalMap", meshMaterial.UseNormalMap);

				// Get textures from AssetManager (note: this will potentially trigger additional loads)
				// TODO(Yan): set maybe to runtime error texture if no asset is present
				Ref<Texture2D> albedoTexture = AssetManager::GetAsset<Texture2D>(meshMaterial.AlbedoTexture);
				if (!albedoTexture)
					albedoTexture = Renderer::GetWhiteTexture();
				material->Set("u_AlbedoTexture", albedoTexture);

				Ref<Texture2D> normalTexture = AssetManager::GetAsset<Texture2D>(meshMaterial.NormalTexture);
				if (!normalTexture)
					normalTexture = Renderer::GetWhiteTexture();
				material->Set("u_NormalTexture", normalTexture);

				Ref<Texture2D> metalnessTexture = AssetManager::GetAsset<Texture2D>(meshMaterial.MetalnessTexture);
				if (!metalnessTexture)
					metalnessTexture = Renderer::GetWhiteTexture();
				material->Set("u_MetalnessTexture", metalnessTexture);

				Ref<Texture2D> roughnessTexture = AssetManager::GetAsset<Texture2D>(meshMaterial.NormalTexture);
				if (!roughnessTexture)
					roughnessTexture = Renderer::GetWhiteTexture();
				material->Set("u_RoughnessTexture", roughnessTexture);

				meshSource->m_Materials[i] = material;
			}
		}

		stream.SetStreamPosition(metadata.VertexBufferOffset + streamOffset);
		stream.ReadArray(meshSource->m_Vertices);

		stream.SetStreamPosition(metadata.IndexBufferOffset + streamOffset);
		stream.ReadArray(meshSource->m_Indices);

		if (hasAnimation || hasSkeleton)
		{
			stream.SetStreamPosition(metadata.AnimationDataOffset + streamOffset);
			if (hasSkeleton)
			{
				stream.ReadArray(meshSource->m_BoneInfluences);
				stream.ReadArray(meshSource->m_BoneInfo);

				meshSource->m_Skeleton = CreateScope<Skeleton>();
				Deserialize(&stream, *meshSource->m_Skeleton);
			}

			uint32_t animationCount;
			stream.ReadRaw(animationCount);
			meshSource->m_Animations.resize(animationCount);
			for (uint32_t i = 0; i < animationCount; i++)
			{
				meshSource->m_Animations[i] = CreateScope<Animation>();
				Deserialize(&stream, *meshSource->m_Animations[i]);
			}
		}

		if (!meshSource->m_Vertices.empty())
			meshSource->m_VertexBuffer = VertexBuffer::Create(meshSource->m_Vertices.data(), (uint32_t)(meshSource->m_Vertices.size() * sizeof(Vertex)));

		if (!meshSource->m_BoneInfluences.empty())
			meshSource->m_BoneInfluenceBuffer = VertexBuffer::Create(meshSource->m_BoneInfluences.data(), (uint32_t)(meshSource->m_BoneInfluences.size() * sizeof(BoneInfluence)));

		if(!meshSource->m_Indices.empty())
			meshSource->m_IndexBuffer = IndexBuffer::Create(meshSource->m_Indices.data(), (uint32_t)(meshSource->m_Indices.size() * sizeof(Index)));

		return meshSource;
	}

}

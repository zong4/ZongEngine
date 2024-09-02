#include "hzpch.h"
#include "ThumbnailGenerator.h"

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Scene/Entity.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/SceneEnvironment.h"
#include "Hazel/Renderer/MeshFactory.h"

#include "Hazel/Asset/AssimpMeshImporter.h"

#include "Hazel/Platform/Vulkan/VulkanImage.h"

namespace Hazel {

	namespace Utils {

		static Entity CreateDefaultCamera(Ref<Scene> scene, float cameraDistance = 1.5f)
		{
			Entity cameraEntity = scene->CreateEntity("Camera");
			auto& cc = cameraEntity.AddComponent<CameraComponent>();
			cc.Camera.SetPerspective(40.0f, 0.1f, 100.0f);
			auto& cameraTransform = cameraEntity.GetComponent<TransformComponent>();
			cameraTransform.Translation = glm::vec3(0, 0, cameraDistance);
			return cameraEntity;
		}

		static Entity CreateMeshCamera(Ref<Scene> scene, AABB meshBoundingBox, float cameraDistance = 1.5f)
		{
			Entity cameraEntity = scene->CreateEntity("Camera");
			auto& cc = cameraEntity.AddComponent<CameraComponent>();
			cc.Camera.SetPerspective(45.0f, 0.1f, 1000.0f);
			auto& cameraTransform = cameraEntity.GetComponent<TransformComponent>();
			cameraTransform.Translation = glm::vec3(-5.0f, 5.0f, 5.0f);
			cameraTransform.SetRotationEuler(glm::vec3(glm::radians(-27.0f), glm::radians(40.0f), 0));
			glm::vec3 cameraForward = cameraTransform.GetRotation() * glm::vec3(0, 0, -1);

			glm::vec3 objectSizes = meshBoundingBox.Size();
			float objectSize = glm::max(glm::max(objectSizes.x, objectSizes.y), objectSizes.z);
			float cameraView = 2.0f * glm::tan(0.5f * cc.Camera.GetRadPerspectiveVerticalFOV());
			float distance = cameraDistance * objectSize / cameraView;
			distance += 0.5f * objectSize;
			glm::vec3 translation = meshBoundingBox.Center() - distance * cameraForward;
			cameraTransform.Translation = translation;

			return cameraEntity;
		}

		static Entity CreateInvalidText(Ref<Scene> scene)
		{
			Entity textEntity = scene->CreateEntity("Text");
			auto& transform = textEntity.GetComponent<TransformComponent>();
			transform.Translation = { -0.6f, -0.6f, 0.0f };
			transform.Scale = glm::vec3(0.2f);
			auto& tc = textEntity.AddComponent<TextComponent>();
			tc.Color = glm::vec4(0.8f, 0.1f, 0.1f, 1.0f);
			tc.TextString = "INVALID";
			return textEntity;
		}

	}

	class MaterialThumbnailGenerator : public AssetThumbnailGenerator
	{
	public:
		MaterialThumbnailGenerator()
		{
			// TODO(Yan): get default sphere?
			AssimpMeshImporter importer("Resources/Meshes/Default/Sphere.gltf");
			Ref<MeshSource> meshSource = importer.ImportToMeshSource();
			AssetHandle meshSourceHandle = AssetManager::AddMemoryOnlyAsset(meshSource);
			m_Sphere = AssetManager::AddMemoryOnlyAsset(Ref<StaticMesh>::Create(meshSourceHandle, false));
		}

		virtual ~MaterialThumbnailGenerator() = default;

		virtual void OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> sceneRenderer) override
		{
			m_DirectionalLight = scene->CreateEntity("DirectionalLight");
			m_DirectionalLight.GetComponent<TransformComponent>().SetRotationEuler(glm::vec3(glm::radians(-90.0f), 0.0f, glm::radians(180.0f)));
			auto& dlc = m_DirectionalLight.AddComponent<DirectionalLightComponent>();
			dlc.Intensity = 0.5f;

			m_Entity = scene->CreateEntity("Model");
			auto& smc = m_Entity.AddComponent<StaticMeshComponent>(m_Sphere);
			smc.MaterialTable->SetMaterial(0, assetHandle);

			m_CameraEntity = Utils::CreateDefaultCamera(scene, 1.75f);
		}

		virtual void OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> sceneRenderer) override
		{
			scene->DestroyEntity(m_Entity);
			scene->DestroyEntity(m_DirectionalLight);
			scene->DestroyEntity(m_CameraEntity);
		}
	private:
		AssetHandle m_Sphere = 0;

		Entity m_Entity;
		Entity m_DirectionalLight;
		Entity m_CameraEntity;
	};

	class EnvironmentThumbnailGenerator : public AssetThumbnailGenerator
	{
	public:
		EnvironmentThumbnailGenerator()
		{
			// TODO(Yan): get default sphere?
			AssimpMeshImporter importer("Resources/Meshes/Default/Sphere.gltf");
			Ref<MeshSource> meshSource = importer.ImportToMeshSource();
			AssetHandle meshSourceHandle = AssetManager::AddMemoryOnlyAsset(meshSource);
			m_Sphere = AssetManager::AddMemoryOnlyAsset(Ref<StaticMesh>::Create(meshSourceHandle, false));
		}

		virtual ~EnvironmentThumbnailGenerator() = default;

		virtual void OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> sceneRenderer) override
		{
			m_DirectionalLight = scene->CreateEntity("DirectionalLight");
			m_DirectionalLight.GetComponent<TransformComponent>().SetRotationEuler(glm::vec3(glm::radians(-90.0f), 0.0f, glm::radians(180.0f)));
			auto& dlc = m_DirectionalLight.AddComponent<DirectionalLightComponent>();
			dlc.Intensity = 0.0f;

			m_Entity = scene->CreateEntity("Model");
			auto& smc = m_Entity.AddComponent<StaticMeshComponent>(m_Sphere);
			auto material = Ref<MaterialAsset>::Create();
			material->SetAlbedoColor(glm::vec3(1.0f));
			material->SetMetalness(1.0f);
			material->SetRoughness(0.0f);
			AssetHandle materialH = AssetManager::AddMemoryOnlyAsset(material);
			smc.MaterialTable->SetMaterial(0, materialH);

			m_SkyLight = scene->CreateEntity("Skylight");
			SkyLightComponent& slc = m_SkyLight.AddComponent<SkyLightComponent>();
			slc.SceneEnvironment = assetHandle;
			slc.Intensity = 0.7f;
			slc.Lod = 1.5f;

			m_CameraEntity = Utils::CreateDefaultCamera(scene, 0.9f);
			m_CameraEntity.GetComponent<CameraComponent>().Camera.SetDegPerspectiveVerticalFOV(80.0f);
		}

		virtual void OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> sceneRenderer) override
		{
			scene->DestroyEntity(m_Entity);
			scene->DestroyEntity(m_DirectionalLight);
			scene->DestroyEntity(m_SkyLight);
			scene->DestroyEntity(m_CameraEntity);
		}
	private:
		AssetHandle m_Sphere = 0;

		Entity m_Entity;
		Entity m_DirectionalLight;
		Entity m_SkyLight;
		Entity m_CameraEntity;
	};


	// TODO(Yan): what is this?
	class TextureThumbnailGenerator : public AssetThumbnailGenerator
	{
	public:
		TextureThumbnailGenerator()
		{
			AssimpMeshImporter importer("Resources/Meshes/Default/Sphere.gltf");
			Ref<MeshSource> meshSource = importer.ImportToMeshSource();
			//m_Sphere = AssetManager::CreateMemoryOnlyAsset<StaticMesh>(meshSource);
			HZ_CORE_VERIFY(false);
		}

		virtual ~TextureThumbnailGenerator() = default;

		virtual void OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> sceneRenderer) override
		{
			HZ_CORE_VERIFY(false);
			m_DirectionalLight = scene->CreateEntity("DirectionalLight");
			m_DirectionalLight.GetComponent<TransformComponent>().SetRotationEuler(glm::vec3(glm::radians(-90.0f), 0.0f, glm::radians(180.0f)));
			auto& dlc = m_DirectionalLight.AddComponent<DirectionalLightComponent>();
			dlc.Intensity = 0.5f;

			m_Entity = scene->CreateEntity("Model");
			auto& smc = m_Entity.AddComponent<StaticMeshComponent>(m_Sphere);
			smc.MaterialTable->SetMaterial(0, assetHandle);

			m_CameraEntity = scene->CreateEntity("Camera");
			auto& cc = m_CameraEntity.AddComponent<CameraComponent>();
			cc.Camera.SetPerspective(40.0f, 0.1f, 100.0f);
			auto& cameraTransform = m_CameraEntity.GetComponent<TransformComponent>();
			cameraTransform.Translation = glm::vec3(0, 0, 1.75f);
		}

		virtual void OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> sceneRenderer) override
		{
			HZ_CORE_VERIFY(false);
			scene->DestroyEntity(m_Entity);
			scene->DestroyEntity(m_DirectionalLight);
			scene->DestroyEntity(m_CameraEntity);
		}
	private:
		AssetHandle m_Sphere = 0;

		Entity m_Entity;
		Entity m_DirectionalLight;
		Entity m_CameraEntity;
	};

	class StaticMeshThumbnailGenerator : public AssetThumbnailGenerator
	{
	public:
		StaticMeshThumbnailGenerator() = default;
		virtual ~StaticMeshThumbnailGenerator() = default;

		virtual void OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> sceneRenderer) override
		{
			m_TextEntity = {};

			m_DirectionalLight = scene->CreateEntity("DirectionalLight");
			m_DirectionalLight.GetComponent<TransformComponent>().SetRotationEuler(glm::vec3(glm::radians(188.0f), glm::radians(68.0f), 0.0f));
			auto& dlc = m_DirectionalLight.AddComponent<DirectionalLightComponent>();
			dlc.Intensity = 0.5f;

			m_Entity = scene->CreateEntity("Model");
			m_Entity.AddComponent<StaticMeshComponent>(assetHandle);

			Ref<StaticMesh> staticMesh = AssetManager::GetAsset<StaticMesh>(assetHandle);
			if (staticMesh)
			{
				Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(staticMesh->GetMeshSource());
				AABB boundingBox = meshSource->GetBoundingBox();
				m_CameraEntity = Utils::CreateMeshCamera(scene, boundingBox);
			}
			else
			{
				m_CameraEntity = Utils::CreateDefaultCamera(scene, 1.75f);
				m_TextEntity = Utils::CreateInvalidText(scene);
			}
		}

		virtual void OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> sceneRenderer) override
		{
			scene->DestroyEntity(m_Entity);
			scene->DestroyEntity(m_DirectionalLight);
			scene->DestroyEntity(m_CameraEntity);
			scene->DestroyEntity(m_TextEntity);
		}
	private:
		Entity m_Entity;
		Entity m_DirectionalLight;
		Entity m_CameraEntity;
		Entity m_TextEntity;
	};

	class MeshThumbnailGenerator : public AssetThumbnailGenerator
	{
	public:
		MeshThumbnailGenerator() = default;
		virtual ~MeshThumbnailGenerator() = default;

		virtual void OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> sceneRenderer) override
		{
			m_TextEntity = {};

			m_DirectionalLight = scene->CreateEntity("DirectionalLight");
			m_DirectionalLight.GetComponent<TransformComponent>().SetRotationEuler(glm::vec3(glm::radians(188.0f), glm::radians(68.0f), 0.0f));
			auto& dlc = m_DirectionalLight.AddComponent<DirectionalLightComponent>();
			dlc.Intensity = 0.5f;

			m_Entity = scene->CreateEntity("Model");
			m_Entity.AddComponent<MeshComponent>(assetHandle);

			Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(assetHandle);
			if (mesh)
			{
				Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(mesh->GetMeshSource());
				AABB boundingBox = meshSource->GetBoundingBox();
				m_CameraEntity = Utils::CreateMeshCamera(scene, boundingBox);
			}
			else
			{
				m_CameraEntity = Utils::CreateDefaultCamera(scene, 1.75f);
				m_TextEntity = Utils::CreateInvalidText(scene);
			}
		}

		virtual void OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> sceneRenderer) override
		{
			scene->DestroyEntity(m_Entity);
			scene->DestroyEntity(m_DirectionalLight);
			scene->DestroyEntity(m_CameraEntity);
			scene->DestroyEntity(m_TextEntity);
		}
	private:
		Entity m_Entity;
		Entity m_DirectionalLight;
		Entity m_CameraEntity;
		Entity m_TextEntity;
	};

	class MeshSourceThumbnailGenerator : public AssetThumbnailGenerator
	{
	public:
		MeshSourceThumbnailGenerator() = default;
		virtual ~MeshSourceThumbnailGenerator() = default;

		virtual void OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> sceneRenderer) override
		{
			m_TextEntity = {};

			m_DirectionalLight = scene->CreateEntity("DirectionalLight");
			m_DirectionalLight.GetComponent<TransformComponent>().SetRotationEuler(glm::vec3(glm::radians(188.0f), glm::radians(68.0f), 0.0f));
			auto& dlc = m_DirectionalLight.AddComponent<DirectionalLightComponent>();
			dlc.Intensity = 0.5f;

			float cameraDistance = 1.5f;
			m_Entity = scene->CreateEntity("Model");

			Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(assetHandle);
			if (meshSource)
			{
				AssetHandle staticMesh = AssetManager::AddMemoryOnlyAsset(Ref<StaticMesh>::Create(assetHandle, false));
				m_Entity.AddComponent<StaticMeshComponent>(staticMesh);

				AABB boundingBox = meshSource->GetBoundingBox();
				m_CameraEntity = Utils::CreateMeshCamera(scene, boundingBox);
			}
			else
			{
				// TODO: error text
				m_CameraEntity = Utils::CreateDefaultCamera(scene, 1.75f);
				m_TextEntity = Utils::CreateInvalidText(scene);
			}
		}

		virtual void OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> sceneRenderer) override
		{
			scene->DestroyEntity(m_Entity);
			scene->DestroyEntity(m_DirectionalLight);
			scene->DestroyEntity(m_CameraEntity);
			scene->DestroyEntity(m_TextEntity);
		}
	private:
		Entity m_Entity;
		Entity m_DirectionalLight;
		Entity m_CameraEntity;
		Entity m_TextEntity;
	};

	ThumbnailGenerator::ThumbnailGenerator()
	{
		m_AssetThumbnailGenerators[AssetType::Material] = Ref<MaterialThumbnailGenerator>::Create();
		//m_AssetThumbnailGenerators[AssetType::Texture] = Ref<TextureThumbnailGenerator>::Create();
		m_AssetThumbnailGenerators[AssetType::StaticMesh] = Ref<StaticMeshThumbnailGenerator>::Create();
		m_AssetThumbnailGenerators[AssetType::Mesh] = Ref<MeshThumbnailGenerator>::Create();
		m_AssetThumbnailGenerators[AssetType::MeshSource] = Ref<MeshSourceThumbnailGenerator>::Create();
		m_AssetThumbnailGenerators[AssetType::EnvMap] = Ref<EnvironmentThumbnailGenerator>::Create();
		// TODO: Texture?

		m_Scene = Ref<Scene>::Create("ThumbnailGenerator");
		m_Scene->SetViewportSize(m_Width, m_Height);
	
		// SkyLight
		{
			Entity skyLight = m_Scene->CreateEntity("SkyLight");
			auto& slc = skyLight.AddComponent<SkyLightComponent>();

			auto [radiance, irradiance] = Renderer::CreateEnvironmentMap("Resources/EnvironmentMaps/meadow_2_2k.hdr");
			AssetHandle handle = AssetManager::AddMemoryOnlyAsset(Ref<Environment>::Create(radiance, irradiance));
			slc.SceneEnvironment = handle;
			slc.Intensity = 1.0f;
			slc.Lod = AssetManager::GetAsset<Environment>(handle)->RadianceMap->GetMipLevelCount() - 3;
		}

		SceneRendererSpecification sceneRendererSpec;
		sceneRendererSpec.ViewportWidth = 512;
		sceneRendererSpec.ViewportHeight = 512;
		m_SceneRenderer = Ref<SceneRenderer>::Create(m_Scene, sceneRendererSpec);
		m_SceneRenderer->GetOptions().ShowGrid = false;
		m_SceneRenderer->GetBloomSettings().Intensity = 0.5f;
		m_SceneRenderer->GetBloomSettings().Threshold = 1.25f;

		// Create resources (no camera lol)
		m_Scene->OnRenderRuntime(m_SceneRenderer, 0.0f);

		m_EnvironmentScene = Ref<Scene>::Create("ThumbnailGenerator-EnvironmentScene");
		m_EnvironmentScene->SetViewportSize(m_Width, m_Height);
		m_EnvironmentScene->OnRenderRuntime(m_SceneRenderer, 0.0f);

		m_RenderCommandBuffer = RenderCommandBuffer::Create(1);
	}

	ThumbnailGenerator::~ThumbnailGenerator()
	{
	}

	Ref<Image2D> ThumbnailGenerator::GenerateThumbnail(AssetHandle handle)
	{
		if (!m_SceneRenderer->IsReady())
			return nullptr;

		if (!AssetManager::IsAssetHandleValid(handle))
			return nullptr;

		AssetManager::EnsureCurrent(handle);

		const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(handle);
		if (metadata.Type == AssetType::Texture)
		{
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);

			uint32_t textureWidth = texture->GetWidth();
			uint32_t textureHeight = texture->GetHeight();
			float verticalAspectRatio = 1.0f;
			float horizontalAspectRatio = 1.0f;
			if (textureWidth > textureHeight)
				verticalAspectRatio = (float)textureHeight / (float)textureWidth;
			else
				horizontalAspectRatio = (float)textureWidth / (float)textureHeight;

			ImageSpecification imageSpec;
			imageSpec.Width = m_Width * horizontalAspectRatio;
			imageSpec.Height = m_Height * verticalAspectRatio;
			imageSpec.Format = ImageFormat::RGBA;
			imageSpec.Usage = ImageUsage::Storage;
			imageSpec.Transfer = true;
			Ref<Image2D> result = Image2D::Create(imageSpec);
			result->Invalidate();

			m_RenderCommandBuffer->Begin();
			Renderer::BlitImage(m_RenderCommandBuffer, texture->GetImage(), result);
			m_RenderCommandBuffer->End();
			m_RenderCommandBuffer->Submit();

			HZ_CORE_WARN("Returning texture thumbnail");
			return result;
		}

		if (m_AssetThumbnailGenerators.find(metadata.Type) == m_AssetThumbnailGenerators.end())
			return nullptr;

		Ref<Scene> scene = metadata.Type == AssetType::EnvMap ? m_EnvironmentScene : m_Scene;

		m_AssetThumbnailGenerators.at(metadata.Type)->OnPrepare(handle, scene, m_SceneRenderer);

		scene->OnRenderRuntime(m_SceneRenderer, 0.0f);

		m_AssetThumbnailGenerators.at(metadata.Type)->OnFinish(handle, scene, m_SceneRenderer);

		if (!m_SceneRenderer->GetFinalPassImage().As<VulkanImage2D>()->GetImageInfo().Image)
		{
			HZ_CORE_ERROR("Image is null!");
			return {};
		}

		ImageSpecification spec;
		spec.Width = m_Width;
		spec.Height = m_Height;
		spec.Format = ImageFormat::RGBA;
		spec.Usage = ImageUsage::Storage;
		spec.Transfer = true;
		Ref<Image2D> result = Image2D::Create(spec);
		result->Invalidate();

		m_RenderCommandBuffer->Begin();
		Renderer::BlitImage(m_RenderCommandBuffer, m_SceneRenderer->GetFinalPassImage(), result);
		m_RenderCommandBuffer->End();
		m_RenderCommandBuffer->Submit();

		return result;

	}

}

#include "pch.h"
#include "Scene.h"

#include "Entity.h"
#include "Prefab.h"

#include "Components.h"

#include "Engine/Editor/NodeGraphEditor/AnimationGraph/AnimationGraphAsset.h"  // TODO (0x): need to separate prototype from the editor asset

#include "Engine/Core/Application.h"
#include "Engine/Core/Events/EditorEvents.h"

#include "Engine/Renderer/SceneRenderer.h"
#include "Engine/Script/ScriptEngine.h"
#include "Engine/Script/ScriptUtils.h"

#include "Engine/Asset/AssetManager.h"

#include "Engine/Renderer/Renderer2D.h"
#include "Engine/Physics/PhysicsSystem.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Physics2D/Physics2D.h"
#include "Engine/Audio/AudioEngine.h"
#include "Engine/Audio/AudioComponent.h"

#include "Engine/Math/Math.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/SceneRenderer.h"

#include "Engine/Editor/SelectionManager.h"

#include "Engine/Debug/Profiler.h"

#include "SceneSerializer.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

// Box2D
#include <box2d/box2d.h>
#include <assimp/scene.h>

namespace Hazel {
	std::unordered_map<UUID, Scene*> s_ActiveScenes;

	struct SceneComponent
	{
		UUID SceneID;
	};

	struct PhysicsSceneComponent
	{
		Ref<PhysicsScene> PhysicsWorld = nullptr;
	};

	namespace Utils {
		glm::mat4 Mat4FromAIMatrix4x4(const aiMatrix4x4& matrix);
	}

	// If this function becomes needed outside of Scene.cpp, then consider moving it to be a a constructor for AudioTransform.
	// That would couple Audio.h to Comopnents.h though, which is why I haven't done it that way.
	Audio::Transform GetAudioTransform(const TransformComponent& transformComponent)
	{
		auto rotation = transformComponent.GetRotation();
		return {
			transformComponent.Translation,
			rotation * glm::vec3(0.0f, 0.0f, -1.0f) /* orientation */,
			rotation * glm::vec3(0.0f, 1.0f, 0.0f)  /* up */
		};
	}

	Scene::Scene(const std::string& name, bool isEditorScene, bool initalize)
		: m_Name(name), m_IsEditorScene(isEditorScene)
	{
		m_SceneEntity = m_Registry.create();
		m_Registry.emplace<SceneComponent>(m_SceneEntity, m_SceneID);
		s_ActiveScenes[m_SceneID] = this;

		if (!initalize)
			return;

		// This might not be the the best way, but Audio Engine needs to keep track of all audio component
		// to have an easy way to lookup a component associated with active sound.
		m_Registry.on_construct<AudioComponent>().connect<&Scene::OnAudioComponentConstruct>(this);
		m_Registry.on_destroy<AudioComponent>().connect<&Scene::OnAudioComponentDestroy>(this);

		m_Registry.on_construct<MeshColliderComponent>().connect<&Scene::OnMeshColliderComponentConstruct>(this);
		m_Registry.on_destroy<MeshColliderComponent>().connect<&Scene::OnMeshColliderComponentDestroy>(this);

		m_Registry.on_construct<RigidBody2DComponent>().connect<&Scene::OnRigidBody2DComponentConstruct>(this);
		m_Registry.on_destroy<RigidBody2DComponent>().connect<&Scene::OnRigidBody2DComponentDestroy>(this);
		m_Registry.on_construct<BoxCollider2DComponent>().connect<&Scene::OnBoxCollider2DComponentConstruct>(this);
		m_Registry.on_destroy<BoxCollider2DComponent>().connect<&Scene::OnBoxCollider2DComponentDestroy>(this);
		m_Registry.on_construct<CircleCollider2DComponent>().connect<&Scene::OnCircleCollider2DComponentConstruct>(this);
		m_Registry.on_destroy<CircleCollider2DComponent>().connect<&Scene::OnCircleCollider2DComponentDestroy>(this);

		m_Registry.on_construct<RigidBodyComponent>().connect<&Scene::OnRigidBodyComponentConstruct>(this);
		// m_Registry.on_destroy<RigidBodyComponent>().connect<&Scene::OnRigidBodyComponentDestroy>(this);
		// ^ replaced by OnRigidBodyComponentDestroy_ProEdition for LD53

		Box2DWorldComponent& b2dWorld = m_Registry.emplace<Box2DWorldComponent>(m_SceneEntity, std::make_unique<b2World>(b2Vec2{ 0.0f, -9.8f }));
		b2dWorld.World->SetContactListener(&b2dWorld.ContactListener);

		Init();
	}

	Scene::~Scene()
	{
		// NOTE(Peter): **VERY** ugly hack around SkyLight GPU leaks
		auto lights = m_Registry.group<SkyLightComponent>(entt::get<TransformComponent>);
		for (auto entity : lights)
		{
			auto [transformComponent, skyLightComponent] = lights.get<TransformComponent, SkyLightComponent>(entity);
			if (AssetManager::IsAssetHandleValid(skyLightComponent.SceneEnvironment) && skyLightComponent.DynamicSky)
				AssetManager::RemoveAsset(skyLightComponent.SceneEnvironment);
		}

		// Clear the registry so that all callbacks are called
		m_Registry.clear();

		// Disconnect EnTT callbacks
		m_Registry.on_construct<ScriptComponent>().disconnect(this);
		m_Registry.on_destroy<ScriptComponent>().disconnect(this);

		m_Registry.on_construct<AudioComponent>().disconnect();
		m_Registry.on_destroy<AudioComponent>().disconnect();

		m_Registry.on_construct<MeshColliderComponent>().disconnect();
		m_Registry.on_destroy<MeshColliderComponent>().disconnect();

		m_Registry.on_construct<RigidBody2DComponent>().disconnect();
		m_Registry.on_destroy<RigidBody2DComponent>().disconnect();

		m_Registry.on_construct<RigidBodyComponent>().disconnect();
		// m_Registry.on_destroy<RigidBodyComponent>().disconnect();
		// ^ replaced by OnRigidBodyComponentDestroy_ProEdition for LD53

		s_ActiveScenes.erase(m_SceneID);
		MiniAudioEngine::OnSceneDestruct(m_SceneID);
	}

	void Scene::Init()
	{
	}

	// Merge OnUpdate/Render into one function?
	void Scene::OnUpdateRuntime(Timestep ts)
	{
		ZONG_PROFILE_FUNC();

		ts = ts * m_TimeScale;

		// Box2D physics
		auto sceneView = m_Registry.view<Box2DWorldComponent>();
		auto& box2DWorld = m_Registry.get<Box2DWorldComponent>(sceneView.front()).World;
		int32_t velocityIterations = 6;
		int32_t positionIterations = 2;
		{
			ZONG_PROFILE_SCOPE("Box2DWorld::Step");
			box2DWorld->Step(ts, velocityIterations, positionIterations);
		}

		{
			auto view = m_Registry.view<RigidBody2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& rb2d = e.GetComponent<RigidBody2DComponent>();

				if (rb2d.RuntimeBody == nullptr)
					continue;

				b2Body* body = static_cast<b2Body*>(rb2d.RuntimeBody);

				auto& position = body->GetPosition();
				auto& transform = e.GetComponent<TransformComponent>();
				transform.Translation.x = position.x;
				transform.Translation.y = position.y;
				glm::vec3 rotation = transform.GetRotationEuler();
				rotation.z = body->GetAngle();
				transform.SetRotationEuler(rotation);
			}
		}

		auto physicsScene = GetPhysicsScene();

		if (m_ShouldSimulate)
		{
			{
				Timer timer;
				physicsScene->Simulate(ts);
				m_PerformanceTimers.PhysicsStep = timer.ElapsedMillis();
			}

			if (m_IsPlaying)
			{
				{
					ZONG_PROFILE_FUNC("Scene::OnUpdate - C# OnUpdate");
					Timer timer;

					for (const auto& [entityID, entityInstance] : ScriptEngine::GetEntityInstances())
					{
						if (m_EntityIDMap.find(entityID) != m_EntityIDMap.end())
						{
							Entity entity = m_EntityIDMap.at(entityID);

							if (ScriptEngine::IsEntityInstantiated(entity))
								ScriptEngine::CallMethod<float>(entityInstance, "OnUpdate", ts);
						}

					}
					m_PerformanceTimers.ScriptUpdate = timer.ElapsedMillis();
				}

				{
					ZONG_PROFILE_FUNC("Scene::OnUpdate - C# OnLateUpdate");
					Timer timer;

					for (const auto& [entityID, entityInstance] : ScriptEngine::GetEntityInstances())
					{
						if (m_EntityIDMap.find(entityID) != m_EntityIDMap.end())
						{
							Entity entity = m_EntityIDMap.at(entityID);

							if (ScriptEngine::IsEntityInstantiated(entity))
								ScriptEngine::CallMethod<float>(entityInstance, "OnLateUpdate", ts);
						}

					}
					m_PerformanceTimers.ScriptLateUpdate = timer.ElapsedMillis();
				}

				physicsScene->SynchronizePendingBodyTransforms();

				for (auto&& fn : m_PostUpdateQueue)
					fn();
				m_PostUpdateQueue.clear();
			}

			UpdateAnimation(ts, true);
		}

		{	//--- Update Audio Listener ---
			//=============================


			// TODO: should probably store active listener handle somewhere
			//		and subscribe to destroy and created events to update the handle?

			ZONG_PROFILE_SCOPE("Scene::OnUpdate - Update Audio Listener");
			auto view = m_Registry.view<AudioListenerComponent>();
			Entity listener;
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& listenerComponent = e.GetComponent<AudioListenerComponent>();
				if (listenerComponent.Active)
				{
					listener = e;
					auto transform = GetAudioTransform(GetWorldSpaceTransform(listener));
					MiniAudioEngine::Get().UpdateListenerPosition(transform);
					MiniAudioEngine::Get().UpdateListenerConeAttenuation(listenerComponent.ConeInnerAngleInRadians,
																		 listenerComponent.ConeOuterAngleInRadians,
																		 listenerComponent.ConeOuterGain);

					/*if (auto physicsActor = physicsScene->GetRigidBody(listener))
					{
						if (physicsActor->IsDynamic())
							MiniAudioEngine::Get().UpdateListenerVelocity(physicsActor->GetLinearVelocity());
					}*/
					break;
				}
			}

			// If listener wasn't found, fallback to using main camera as an active listener
			if (listener.m_EntityHandle == entt::null)
			{
				listener = GetMainCameraEntity();
				if (listener.m_EntityHandle != entt::null)
				{
					// If camera was changed or destroyed during Runtime, it might not have Listener Component (?)
					if (!listener.HasComponent<AudioListenerComponent>())
						listener.AddComponent<AudioListenerComponent>();

					auto transform = GetAudioTransform(GetWorldSpaceTransform(listener));
					MiniAudioEngine::Get().UpdateListenerPosition(transform);

					auto& listenerComponent = listener.GetComponent<AudioListenerComponent>();
					MiniAudioEngine::Get().UpdateListenerConeAttenuation(listenerComponent.ConeInnerAngleInRadians,
																		 listenerComponent.ConeOuterAngleInRadians,
																		 listenerComponent.ConeOuterGain);

					if (physicsScene)
					{
						auto physicsActor = physicsScene->GetEntityBody(listener);
						if (physicsActor && physicsActor->IsDynamic())
							MiniAudioEngine::Get().UpdateListenerVelocity(physicsActor->GetLinearVelocity());
					}
				}
			}
			//! If we don't have Main Camera in the scene, nor any user created Listeners,
			//! we won't be able to hear any sound!
			//! Unless there's a way to retrieve the editor's camera,
			//! which is not a part of this runtime scene.

			//? allow updating listener settings at runtime?
			//auto& audioListenerComponent = view.get(entity); 
		}

		{	//--- Update Audio Components ---
			//===============================

			ZONG_PROFILE_SCOPE("Scene::OnUpdate - Update Audio Components");

			// 1. We need to handle entities that are no longer used by Audio Engine,
			// mainly the ones that were created for "fire and forge" audio events.
			const std::unordered_set<UUID> inactiveEntities = MiniAudioEngine::Get().GetInactiveEntities();
			for (const UUID entityID : inactiveEntities)
			{
				Entity entity = TryGetEntityWithUUID(entityID);
				if (entity == Entity{} || !entity.HasComponent<AudioComponent>())
					continue;

				const auto& audioComponent = entity.GetComponent<AudioComponent>();
				
				// AutoDestroy flag is only set for "one-shot" sounds
				if (audioComponent.bAutoDestroy)
					DestroyEntity(entityID);
			}

			auto view = m_Registry.view<AudioComponent>();

			std::vector<SoundSourceUpdateData> updateData;
			updateData.reserve(view.size());

			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& audioComponent = e.GetComponent<AudioComponent>();

				// 2. Update positions of associated sound sources
				auto transform = GetAudioTransform(GetWorldSpaceTransform(e));

				// 3. Update velocities of associated sound sources
				glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
				if (auto physicsActor = physicsScene->GetEntityBody(e))
				{
					if (physicsActor->IsDynamic())
						velocity = physicsActor->GetLinearVelocity();
				}

				updateData.emplace_back(SoundSourceUpdateData{ e.GetUUID(),
					transform,
					velocity,
					audioComponent.VolumeMultiplier,
					audioComponent.PitchMultiplier });
			}

			//--- Submit values to AudioEngine to update associated sound sources ---
			//-----------------------------------------------------------------------
			MiniAudioEngine::Get().SubmitSourceUpdateData(std::move(updateData));
		}

	}

	void Scene::OnUpdateEditor(Timestep ts)
	{
		ZONG_PROFILE_FUNC();
		UpdateAnimation(ts, false);
	}

	void Scene::OnRenderRuntime(Ref<SceneRenderer> renderer, Timestep ts)
	{
		ZONG_PROFILE_FUNC();

		/////////////////////////////////////////////////////////////////////
		// RENDER 3D SCENE
		/////////////////////////////////////////////////////////////////////
		Entity cameraEntity = GetMainCameraEntity();
		if (!cameraEntity)
			return;

		glm::mat4 cameraViewMatrix = glm::inverse(GetWorldSpaceTransformMatrix(cameraEntity));
		ZONG_CORE_ASSERT(cameraEntity, "Scene does not contain any cameras!");
		SceneCamera& camera = cameraEntity.GetComponent<CameraComponent>();
		camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);

		// Process lights
		{
			m_LightEnvironment = LightEnvironment();
			// Directional Lights	
			{
				auto lights = m_Registry.group<DirectionalLightComponent>(entt::get<TransformComponent>);
				uint32_t directionalLightIndex = 0;
				for (auto entity : lights)
				{
					auto [transformComponent, lightComponent] = lights.get<TransformComponent, DirectionalLightComponent>(entity);
					glm::vec3 direction = -glm::normalize(glm::mat3(transformComponent.GetTransform()) * glm::vec3(1.0f));
					m_LightEnvironment.DirectionalLights[directionalLightIndex++] =
					{
						direction,
						lightComponent.Radiance,
						lightComponent.Intensity,
						lightComponent.ShadowAmount,
						lightComponent.CastShadows,
					};
				}
				// Point Lights
				{
					auto pointLights = m_Registry.group<PointLightComponent>(entt::get<TransformComponent>);
					m_LightEnvironment.PointLights.resize(pointLights.size());
					uint32_t pointLightIndex = 0;
					for (auto e : pointLights)
					{
						Entity entity(e, this);
						auto [transformComponent, lightComponent] = pointLights.get<TransformComponent, PointLightComponent>(e);
						auto transform = GetWorldSpaceTransform(entity);
						m_LightEnvironment.PointLights[pointLightIndex++] = {
							transform.Translation,
							lightComponent.Intensity,
							lightComponent.Radiance,
							lightComponent.MinRadius,
							lightComponent.Radius,
							lightComponent.Falloff,
							lightComponent.LightSize,
							lightComponent.CastsShadows,
						};
					}
				}
				// Spot Lights
				{
					auto spotLights = m_Registry.group<SpotLightComponent>(entt::get<TransformComponent>);
					m_LightEnvironment.SpotLights.resize(spotLights.size());
					uint32_t spotLightIndex = 0;
					for (auto e : spotLights)
					{
						Entity entity(e, this);
						auto [transformComponent, lightComponent] = spotLights.get<TransformComponent, SpotLightComponent>(e);
						auto transform = GetWorldSpaceTransform(entity);
						glm::vec3 direction = glm::normalize(glm::rotate(transform.GetRotation(), glm::vec3(1.0f, 0.0f, 0.0f)));

						m_LightEnvironment.SpotLights[spotLightIndex++] = {
							transform.Translation,
							lightComponent.Intensity,
							direction,
							lightComponent.AngleAttenuation,
							lightComponent.Radiance,
							lightComponent.Range,
							lightComponent.Angle,
							lightComponent.Falloff,
							lightComponent.SoftShadows,
							{},
							lightComponent.CastsShadows
						};
					}
				}
			}
		}

		// TODO: only one sky light at the moment!
		{
			auto lights = m_Registry.group<SkyLightComponent>(entt::get<TransformComponent>);

			for (auto entity : lights)
			{
				auto [transformComponent, skyLightComponent] = lights.get<TransformComponent, SkyLightComponent>(entity);
				if (!AssetManager::IsAssetHandleValid(skyLightComponent.SceneEnvironment) && skyLightComponent.DynamicSky)
				{
					Ref<TextureCube> preethamEnv = Renderer::CreatePreethamSky(skyLightComponent.TurbidityAzimuthInclination.x, skyLightComponent.TurbidityAzimuthInclination.y, skyLightComponent.TurbidityAzimuthInclination.z);
					skyLightComponent.SceneEnvironment = AssetManager::CreateMemoryOnlyAsset<Environment>(preethamEnv, preethamEnv);
				}
				
				m_Environment = AssetManager::GetAsset<Environment>(skyLightComponent.SceneEnvironment);
				m_EnvironmentIntensity = skyLightComponent.Intensity;
				m_SkyboxLod = skyLightComponent.Lod;
			}

			bool invalidSkyLight = !m_Environment || lights.empty();

			if (invalidSkyLight)
			{
				m_Environment = Ref<Environment>::Create(Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture());
				m_EnvironmentIntensity = 0.0f;
				m_SkyboxLod = 1.0f;
			}
		}

		renderer->SetScene(this);
		renderer->BeginScene({ camera, cameraViewMatrix, camera.GetPerspectiveNearClip(), camera.GetPerspectiveFarClip(), camera.GetRadPerspectiveVerticalFOV() });

		// Render Static Meshes
		{
			auto group = m_Registry.group<StaticMeshComponent>(entt::get<TransformComponent>);
			for (auto entity : group)
			{
				ZONG_PROFILE_SCOPE("Scene-SubmitStaticMesh");
				auto [transformComponent, staticMeshComponent] = group.get<TransformComponent, StaticMeshComponent>(entity);
				if (!staticMeshComponent.Visible)
					continue;

				if (AssetManager::IsAssetHandleValid(staticMeshComponent.StaticMesh))
				{
					Ref<StaticMesh> staticMesh = AssetManager::GetAsset<StaticMesh>(staticMeshComponent.StaticMesh);
					if (staticMesh && !staticMesh->IsFlagSet(AssetFlag::Missing))
					{
						Entity e = Entity(entity, this);
						glm::mat4 transform = GetWorldSpaceTransformMatrix(e);
						renderer->SubmitStaticMesh(staticMesh, staticMeshComponent.MaterialTable, transform);
					}
				}
			}
		}

		// Render Dynamic Meshes
		{
			auto view = m_Registry.view<MeshComponent, TransformComponent>();
			for (auto entity : view)
			{
				ZONG_PROFILE_SCOPE("Scene-SubmitDynamicMesh");
				auto [transformComponent, meshComponent] = view.get<TransformComponent, MeshComponent>(entity);
				if (!meshComponent.Visible)
					continue;

				if (AssetManager::IsAssetHandleValid(meshComponent.Mesh))
				{
					Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(meshComponent.Mesh);
					if (mesh && !mesh->IsFlagSet(AssetFlag::Missing))
					{
						Entity e = Entity(entity, this);
						glm::mat4 transform = GetWorldSpaceTransformMatrix(e);
						renderer->SubmitMesh(mesh, meshComponent.SubmeshIndex, meshComponent.MaterialTable, transform, GetModelSpaceBoneTransforms(meshComponent.BoneEntityIds, mesh));
					}
				}
			}
		}

		RenderPhysicsDebug(renderer, true);

		renderer->EndScene();
		/////////////////////////////////////////////////////////////////////
		// Render 2D
		if (renderer->GetFinalPassImage())
		{
			Ref<Renderer2D> renderer2D = renderer->GetRenderer2D();
			Ref<Renderer2D> screenSpaceRenderer2D = renderer->GetScreenSpaceRenderer2D();

			bool hasScreenSpaceEntities = false;
			
			renderer2D->ResetStats();
			renderer2D->BeginScene(camera.GetProjectionMatrix() * cameraViewMatrix, cameraViewMatrix);
			renderer2D->SetTargetFramebuffer(renderer->GetExternalCompositeFramebuffer());
			{
				auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
				for (auto entity : view)
				{
					Entity e = Entity(entity, this);
					auto [transformComponent, spriteRendererComponent] = view.get<TransformComponent, SpriteRendererComponent>(entity);
					if (spriteRendererComponent.Texture)
					{
						if (AssetManager::IsAssetHandleValid(spriteRendererComponent.Texture))
						{
							Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(spriteRendererComponent.Texture);
							renderer2D->DrawQuad(GetWorldSpaceTransformMatrix(e), texture, spriteRendererComponent.TilingFactor,
							spriteRendererComponent.Color, spriteRendererComponent.UVStart, spriteRendererComponent.UVEnd);
						}
					}
					else
					{
						renderer2D->DrawQuad(GetWorldSpaceTransformMatrix(e), spriteRendererComponent.Color);
					}
				}

				auto group = m_Registry.group<TransformComponent>(entt::get<TextComponent>);
				for (auto entity : group)
				{
					auto [transformComponent, textComponent] = group.get<TransformComponent, TextComponent>(entity);

					// Defer screen space elements to next pass
					if (textComponent.ScreenSpace)
					{
						hasScreenSpaceEntities = true;
						continue;
					}

					Entity e = Entity(entity, this);
					auto font = Font::GetFontAssetForTextComponent(textComponent);
					renderer2D->DrawString(textComponent.TextString, font, GetWorldSpaceTransformMatrix(e), textComponent.MaxWidth, textComponent.Color, textComponent.LineSpacing, textComponent.Kerning);
				}
			}

			// Save line width
			float lineWidth = renderer2D->GetLineWidth();

			// Debug Renderer
			{
				Ref<DebugRenderer> debugRenderer = renderer->GetDebugRenderer();
				auto& renderQueue = debugRenderer->GetRenderQueue();
				for (auto&& func : renderQueue)
					func(renderer2D);

				debugRenderer->ClearRenderQueue();
			}

			Render2DPhysicsDebug(renderer, renderer2D, true);

			renderer2D->EndScene();

			if (hasScreenSpaceEntities)
			{
				// Render screen space elements
				screenSpaceRenderer2D->ResetStats();
				screenSpaceRenderer2D->BeginScene(renderer->GetScreenSpaceProjectionMatrix(), glm::mat4(1.0f));
				screenSpaceRenderer2D->SetTargetFramebuffer(renderer->GetExternalCompositeFramebuffer());

				{
					auto group = m_Registry.group<TransformComponent>(entt::get<TextComponent>);
					for (auto entity : group)
					{
						auto [transformComponent, textComponent] = group.get<TransformComponent, TextComponent>(entity);
						// Already rendered non-screenspace elements
						if (!textComponent.ScreenSpace)
							continue;

						Entity e = Entity(entity, this);
						auto font = Font::GetFontAssetForTextComponent(textComponent);

						float renderScale = renderer->GetSpecification().Tiering.RendererScale;
						if (textComponent.DropShadow)
						{
							glm::mat4 scale = glm::scale(glm::mat4(1.0f), transformComponent.Scale * renderScale);
							glm::mat4 transformShadow = glm::translate(glm::mat4(1.0f), { (transformComponent.Translation.x + textComponent.ShadowDistance) * renderScale, (transformComponent.Translation.y - textComponent.ShadowDistance) * renderScale, transformComponent.Translation.z + 0.01f}) * scale;
							screenSpaceRenderer2D->DrawString(textComponent.TextString, font, transformShadow, textComponent.MaxWidth, textComponent.ShadowColor, textComponent.LineSpacing, textComponent.Kerning);
						}

						glm::mat4 transform = glm::translate(glm::mat4(1.0f), transformComponent.Translation * glm::vec3(renderScale, renderScale, 1.0f)) * glm::toMat4(transformComponent.GetRotation())
							* glm::scale(glm::mat4(1.0f), transformComponent.Scale * renderScale);

						//screenSpaceRenderer2D->DrawString(textComponent.TextString, font, glm::scale(transformComponent.GetTransform(), glm::vec3(renderScale, renderScale, 1.0f)), textComponent.MaxWidth, textComponent.Color, textComponent.LineSpacing, textComponent.Kerning);
						screenSpaceRenderer2D->DrawString(textComponent.TextString, font, transform, textComponent.MaxWidth, textComponent.Color, textComponent.LineSpacing, textComponent.Kerning);
					}
				}

				screenSpaceRenderer2D->EndScene();

				// Restore line width (in case it was changed)
				screenSpaceRenderer2D->SetLineWidth(lineWidth);
			}

			// Restore line width (in case it was changed)
			renderer2D->SetLineWidth(lineWidth);
		}
	}

	void Scene::OnRenderEditor(Ref<SceneRenderer> renderer, Timestep ts, const EditorCamera& editorCamera)
	{
		ZONG_PROFILE_FUNC();

		/////////////////////////////////////////////////////////////////////
		// RENDER 3D SCENE
		/////////////////////////////////////////////////////////////////////

		// Lighting
		{
			m_LightEnvironment = LightEnvironment();
			//Directional Lights
			{
				auto dirLights = m_Registry.group<DirectionalLightComponent>(entt::get<TransformComponent>);
				uint32_t directionalLightIndex = 0;
				for (auto entity : dirLights)
				{
					if (directionalLightIndex >= LightEnvironment::MaxDirectionalLights)
						break;

					auto [transformComponent, lightComponent] = dirLights.get<TransformComponent, DirectionalLightComponent>(entity);
					glm::vec3 direction = -glm::normalize(glm::mat3(transformComponent.GetTransform()) * glm::vec3(1.0f));
					m_LightEnvironment.DirectionalLights[directionalLightIndex++] =
					{
						direction,
						lightComponent.Radiance,
						lightComponent.Intensity,
						lightComponent.ShadowAmount,
						lightComponent.CastShadows
					};
				}
			}
			// Point Lights
			{
				auto pointLights = m_Registry.group<PointLightComponent>(entt::get<TransformComponent>);
				m_LightEnvironment.PointLights.resize(pointLights.size());
				uint32_t pointLightIndex = 0;
				for (auto entity : pointLights)
				{
					auto [transformComponent, lightComponent] = pointLights.get<TransformComponent, PointLightComponent>(entity);
					auto transform = GetWorldSpaceTransform(Entity(entity, this));
					m_LightEnvironment.PointLights[pointLightIndex++] = {
						transform.Translation,
						lightComponent.Intensity,
						lightComponent.Radiance,
						lightComponent.MinRadius,
						lightComponent.Radius,
						lightComponent.Falloff,
						lightComponent.LightSize,
						lightComponent.CastsShadows,
					};

				}
			}
			// Spot Lights
			{
				auto spotLights = m_Registry.group<SpotLightComponent>(entt::get<TransformComponent>);
				m_LightEnvironment.SpotLights.resize(spotLights.size());
				uint32_t spotLightIndex = 0;
				for (auto e : spotLights)
				{
					Entity entity(e, this);
					auto [transformComponent, lightComponent] = spotLights.get<TransformComponent, SpotLightComponent>(e);
					auto transform = entity.HasComponent<RigidBodyComponent>() ? entity.Transform() : GetWorldSpaceTransform(entity);
					glm::vec3 direction = glm::normalize(glm::rotate(transform.GetRotation(), glm::vec3(1.0f, 0.0f, 0.0f)));

					m_LightEnvironment.SpotLights[spotLightIndex++] = {
						transform.Translation,
						lightComponent.Intensity,
						direction,
						lightComponent.AngleAttenuation,
						lightComponent.Radiance,
						lightComponent.Range,
						lightComponent.Angle,
						lightComponent.Falloff,
						lightComponent.SoftShadows,
						{},
						lightComponent.CastsShadows
					};
				}
			}
		}

		{
			auto lights = m_Registry.group<SkyLightComponent>(entt::get<TransformComponent>);

			for (auto entity : lights)
			{
				auto [transformComponent, skyLightComponent] = lights.get<TransformComponent, SkyLightComponent>(entity);

				if (!AssetManager::IsAssetHandleValid(skyLightComponent.SceneEnvironment) && skyLightComponent.DynamicSky)
				{
					Ref<TextureCube> preethamEnv = Renderer::CreatePreethamSky(skyLightComponent.TurbidityAzimuthInclination.x, skyLightComponent.TurbidityAzimuthInclination.y, skyLightComponent.TurbidityAzimuthInclination.z);
					skyLightComponent.SceneEnvironment = AssetManager::CreateMemoryOnlyAsset<Environment>(preethamEnv, preethamEnv);
				}

				m_Environment = AssetManager::GetAsset<Environment>(skyLightComponent.SceneEnvironment);
				m_EnvironmentIntensity = skyLightComponent.Intensity;
				m_SkyboxLod = skyLightComponent.Lod;
			}

			bool invalidSkyLight = !m_Environment || lights.empty();

			if (invalidSkyLight)
			{
				m_Environment = Ref<Environment>::Create(Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture());
				m_EnvironmentIntensity = 0.0f;
				m_SkyboxLod = 1.0f;
			}
		}

		renderer->SetScene(this);
		renderer->BeginScene({ editorCamera, editorCamera.GetViewMatrix(), editorCamera.GetNearClip(), editorCamera.GetFarClip(), editorCamera.GetVerticalFOV() });

		// Render Static Meshes
		{
			auto group = m_Registry.group<StaticMeshComponent>(entt::get<TransformComponent>);
			for (auto entity : group)
			{
				auto [transformComponent, staticMeshComponent] = group.get<TransformComponent, StaticMeshComponent>(entity);
				if (!staticMeshComponent.Visible)
					continue;

				auto staticMesh = AssetManager::GetAsset<StaticMesh>(staticMeshComponent.StaticMesh);
				if (staticMesh && !staticMesh->IsFlagSet(AssetFlag::Missing))
				{
					Entity e = Entity(entity, this);
					glm::mat4 transform = GetWorldSpaceTransformMatrix(e);

					if (SelectionManager::IsEntityOrAncestorSelected(e))
						renderer->SubmitSelectedStaticMesh(staticMesh, staticMeshComponent.MaterialTable, transform);
					else
						renderer->SubmitStaticMesh(staticMesh, staticMeshComponent.MaterialTable, transform);
				}
			}
		}

		// Render Dynamic Meshes
		{
			auto view = m_Registry.view<MeshComponent, TransformComponent>();
			for (auto entity : view)
			{
				auto [transformComponent, meshComponent] = view.get<TransformComponent, MeshComponent>(entity);
				if (!meshComponent.Visible)
					continue;

				auto mesh = AssetManager::GetAsset<Mesh>(meshComponent.Mesh);
				if (mesh && !mesh->IsFlagSet(AssetFlag::Missing))
				{
					Entity e = Entity(entity, this);
					glm::mat4 transform = GetWorldSpaceTransformMatrix(e);

					// TODO: Should we render (logically)
					if (SelectionManager::IsEntityOrAncestorSelected(e))
						renderer->SubmitSelectedMesh(mesh, meshComponent.SubmeshIndex, meshComponent.MaterialTable, transform, GetModelSpaceBoneTransforms(meshComponent.BoneEntityIds, mesh));
					else
						renderer->SubmitMesh(mesh, meshComponent.SubmeshIndex, meshComponent.MaterialTable, transform, GetModelSpaceBoneTransforms(meshComponent.BoneEntityIds, mesh));
				}
			}
		}

		RenderPhysicsDebug(renderer, false);

		renderer->EndScene();
		/////////////////////////////////////////////////////////////////////

		// TODO: workaround for Scene not being updated if SceneState is Edit (not Play), need to update audio listener position somewhere
		{
			const auto& camPosition = editorCamera.GetPosition();
			//? can't grab forward direction because of const qualifier, so have to convert manually here
			auto camDirection = glm::rotate(editorCamera.GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
			auto camUp = glm::rotate(editorCamera.GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
			Audio::Transform transform{ camPosition, camDirection, camUp };

			MiniAudioEngine::Get().UpdateListenerPosition(transform);

			//? allow updating listener settings at ruSntime?
			//auto& audioListenerComponent = view.get(entity); 
		}


		{	//--- Update Audio Component (editor scene update) ---
			//====================================================

			// 1st We need to handle entities that are no longer used by Audio Engine,
			// mainly the ones that were created for "fire and forge" audio events.
			const std::unordered_set<UUID> inactiveEntities = MiniAudioEngine::Get().GetInactiveEntities();
			for (const UUID entityID : inactiveEntities)
			{
				Entity entity = TryGetEntityWithUUID(entityID);
				if (entity == Entity{} || !entity.HasComponent<AudioComponent>())
					continue;

				const auto& audioComponent = entity.GetComponent<AudioComponent>();

				// AutoDestroy flag is only set for "one-shot" sounds
				if (audioComponent.bAutoDestroy)
					DestroyEntity(entityID);
			}

			auto view = m_Registry.view<AudioComponent>();

			std::vector<Entity> deadEntities;

			std::vector<SoundSourceUpdateData> updateData;
			updateData.reserve(view.size());

			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& audioComponent = e.GetComponent<AudioComponent>();
				
				auto transform = GetAudioTransform(GetWorldSpaceTransform(e));

				glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
				updateData.emplace_back(SoundSourceUpdateData{ e.GetUUID(),
					transform,
					velocity,
					audioComponent.VolumeMultiplier,
					audioComponent.PitchMultiplier });
			}

			//--- Submit values to AudioEngine to update associated sound sources ---
			//-----------------------------------------------------------------------
			MiniAudioEngine::Get().SubmitSourceUpdateData(std::move(updateData));
		}

		// Render 2D
		if (renderer->GetFinalPassImage())
		{
			Ref<Renderer2D> renderer2D = renderer->GetRenderer2D();
			Ref<Renderer2D> screenSpaceRenderer2D = renderer->GetScreenSpaceRenderer2D();

			renderer2D->ResetStats();
			renderer2D->BeginScene(editorCamera.GetViewProjection(), editorCamera.GetViewMatrix());
			renderer2D->SetTargetFramebuffer(renderer->GetExternalCompositeFramebuffer());

			bool hasScreenSpaceEntities = false;
			{
				auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
				for (auto entity : view)
				{
					Entity e = Entity(entity, this);
					auto [transformComponent, spriteRendererComponent] = view.get<TransformComponent, SpriteRendererComponent>(entity);
					if (spriteRendererComponent.Texture)
					{
						if (AssetManager::IsAssetHandleValid(spriteRendererComponent.Texture))
						{
							Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(spriteRendererComponent.Texture);
							renderer2D->DrawQuad(GetWorldSpaceTransformMatrix(e), texture, spriteRendererComponent.TilingFactor,
							spriteRendererComponent.Color, spriteRendererComponent.UVStart, spriteRendererComponent.UVEnd);
						}
					}
					else
					{
						renderer2D->DrawQuad(GetWorldSpaceTransformMatrix(e), spriteRendererComponent.Color);
					}
				}
				auto group = m_Registry.group<TransformComponent>(entt::get<TextComponent>);
				for (auto entity : group)
				{
					auto [transformComponent, textComponent] = group.get<TransformComponent, TextComponent>(entity);

					// Defer screen space elements to next pass
					if (textComponent.ScreenSpace)
					{
						hasScreenSpaceEntities = true;
						continue;
					}

					Entity e = Entity(entity, this);
					auto font = Font::GetFontAssetForTextComponent(textComponent);
					renderer2D->DrawString(textComponent.TextString, font, GetWorldSpaceTransformMatrix(e), textComponent.MaxWidth, textComponent.Color, textComponent.LineSpacing, textComponent.Kerning);
				}
			}

			// Save line width
			float lineWidth = renderer2D->GetLineWidth();

			// Debug Renderer
			{
				Ref<DebugRenderer> debugRenderer = renderer->GetDebugRenderer();
				auto& renderQueue = debugRenderer->GetRenderQueue();
				for (auto&& func : renderQueue)
					func(renderer2D);

				debugRenderer->ClearRenderQueue();
			}

			Render2DPhysicsDebug(renderer, renderer2D, true);

			renderer2D->EndScene();
			// Restore line width (in case it was changed)
			renderer2D->SetLineWidth(lineWidth);

			if (hasScreenSpaceEntities)
			{
				// Render screen space elements
				screenSpaceRenderer2D->ResetStats();
				screenSpaceRenderer2D->BeginScene(renderer->GetScreenSpaceProjectionMatrix(), glm::mat4(1.0f));
				screenSpaceRenderer2D->SetTargetFramebuffer(renderer->GetExternalCompositeFramebuffer());

				{
					auto group = m_Registry.group<TransformComponent>(entt::get<TextComponent>);
					for (auto entity : group)
					{
						auto [transformComponent, textComponent] = group.get<TransformComponent, TextComponent>(entity);
						// Already rendered non-screenspace elements
						if (!textComponent.ScreenSpace)
							continue;

						Entity e = Entity(entity, this);
						auto font = Font::GetFontAssetForTextComponent(textComponent);

						if (textComponent.DropShadow)
						{
							glm::mat4 scale = glm::scale(glm::mat4(1.0f), transformComponent.Scale);
							glm::mat4 transformShadow = glm::translate(glm::mat4(1.0f), { transformComponent.Translation.x + textComponent.ShadowDistance, transformComponent.Translation.y - textComponent.ShadowDistance, transformComponent.Translation.z + 0.01f }) * scale;
							screenSpaceRenderer2D->DrawString(textComponent.TextString, font, transformShadow, textComponent.MaxWidth, textComponent.ShadowColor, textComponent.LineSpacing, textComponent.Kerning);
						}
						screenSpaceRenderer2D->DrawString(textComponent.TextString, font, transformComponent.GetTransform(), textComponent.MaxWidth, textComponent.Color, textComponent.LineSpacing, textComponent.Kerning);
					}
				}

				screenSpaceRenderer2D->EndScene();

				// Restore line width (in case it was changed)
				screenSpaceRenderer2D->SetLineWidth(lineWidth);
			}
		
		}
	}

	void Scene::OnRenderSimulation(Ref<SceneRenderer> renderer, Timestep ts, const EditorCamera& editorCamera)
	{
		ZONG_PROFILE_FUNC();

		/////////////////////////////////////////////////////////////////////
		// RENDER 3D SCENE
		/////////////////////////////////////////////////////////////////////

		// Lighting
		{
			m_LightEnvironment = LightEnvironment();
			//Directional Lights
			{
				auto dirLights = m_Registry.group<DirectionalLightComponent>(entt::get<TransformComponent>);
				uint32_t directionalLightIndex = 0;
				for (auto entity : dirLights)
				{
					auto [transformComponent, lightComponent] = dirLights.get<TransformComponent, DirectionalLightComponent>(entity);
					glm::vec3 direction = -glm::normalize(glm::mat3(transformComponent.GetTransform()) * glm::vec3(1.0f));
					m_LightEnvironment.DirectionalLights[directionalLightIndex++] =
					{
						direction,
						lightComponent.Radiance,
						lightComponent.Intensity,
						lightComponent.ShadowAmount,
						lightComponent.CastShadows
					};
				}
			}
			// Point Lights
			{
				auto pointLights = m_Registry.group<PointLightComponent>(entt::get<TransformComponent>);
				m_LightEnvironment.PointLights.resize(pointLights.size());
				uint32_t pointLightIndex = 0;
				for (auto e : pointLights)
				{
					Entity entity(e, this);
					auto [transformComponent, lightComponent] = pointLights.get<TransformComponent, PointLightComponent>(entity);
					auto transform = entity.HasComponent<RigidBodyComponent>() ? entity.Transform() : GetWorldSpaceTransform(entity);
					m_LightEnvironment.PointLights[pointLightIndex++] = {
						transform.Translation,
						lightComponent.Intensity,
						lightComponent.Radiance,
						lightComponent.MinRadius,
						lightComponent.Radius,
						lightComponent.Falloff,
						lightComponent.LightSize,
						lightComponent.CastsShadows,
					};

				}
			}
			// Spot Lights
			{
				auto spotLights = m_Registry.group<SpotLightComponent>(entt::get<TransformComponent>);
				m_LightEnvironment.SpotLights.resize(spotLights.size());
				uint32_t spotLightIndex = 0;
				for (auto e : spotLights)
				{
					Entity entity(e, this);
					auto [transformComponent, lightComponent] = spotLights.get<TransformComponent, SpotLightComponent>(e);
					auto transform = entity.HasComponent<RigidBodyComponent>() ? entity.Transform() : GetWorldSpaceTransform(entity);
					glm::vec3 direction = glm::normalize(glm::rotate(transform.GetRotation(), glm::vec3(1.0f, 0.0f, 0.0f)));

					m_LightEnvironment.SpotLights[spotLightIndex++] = {
						transform.Translation,
						lightComponent.Intensity,
						direction,
						lightComponent.AngleAttenuation,
						lightComponent.Radiance,
						lightComponent.Range,
						lightComponent.Angle,
						lightComponent.Falloff,
						lightComponent.SoftShadows,
						{},
						lightComponent.CastsShadows
					};
				}
			}
		}

		{
			auto lights = m_Registry.group<SkyLightComponent>(entt::get<TransformComponent>);
			if (lights.empty())
				m_Environment = Ref<Environment>::Create(Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture());

			for (auto entity : lights)
			{
				auto [transformComponent, skyLightComponent] = lights.get<TransformComponent, SkyLightComponent>(entity);
				if (!AssetManager::IsAssetHandleValid(skyLightComponent.SceneEnvironment) && skyLightComponent.DynamicSky)
				{
					Ref<TextureCube> preethamEnv = Renderer::CreatePreethamSky(skyLightComponent.TurbidityAzimuthInclination.x, skyLightComponent.TurbidityAzimuthInclination.y, skyLightComponent.TurbidityAzimuthInclination.z);
					skyLightComponent.SceneEnvironment = AssetManager::CreateMemoryOnlyAsset<Environment>(preethamEnv, preethamEnv);
				}
				m_Environment = AssetManager::GetAsset<Environment>(skyLightComponent.SceneEnvironment);
				m_EnvironmentIntensity = skyLightComponent.Intensity;
				m_SkyboxLod = skyLightComponent.Lod;
			}

			if (!m_Environment)
			{
				m_Environment = Ref<Environment>::Create(Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture());
				m_EnvironmentIntensity = 0.0f;
				m_SkyboxLod = 1.0f;
			}
		}

		auto group = m_Registry.group<MeshComponent>(entt::get<TransformComponent>);
		renderer->SetScene(this);
		renderer->BeginScene({ editorCamera, editorCamera.GetViewMatrix(), editorCamera.GetNearClip(), editorCamera.GetFarClip(), editorCamera.GetVerticalFOV() });

		// Render Static Meshes
		{
			auto group = m_Registry.group<StaticMeshComponent>(entt::get<TransformComponent>);
			for (auto entity : group)
			{
				auto [transformComponent, staticMeshComponent] = group.get<TransformComponent, StaticMeshComponent>(entity);
				if (!staticMeshComponent.Visible)
					continue;

				if (AssetManager::IsAssetHandleValid(staticMeshComponent.StaticMesh))
				{
					auto staticMesh = AssetManager::GetAsset<StaticMesh>(staticMeshComponent.StaticMesh);
					if (!staticMesh->IsFlagSet(AssetFlag::Missing))
					{
						Entity e = Entity(entity, this);
						glm::mat4 transform = GetWorldSpaceTransformMatrix(e);

						if (SelectionManager::IsEntityOrAncestorSelected(e))
							renderer->SubmitSelectedStaticMesh(staticMesh, staticMeshComponent.MaterialTable, transform);
						else
							renderer->SubmitStaticMesh(staticMesh, staticMeshComponent.MaterialTable, transform);
					}
				}
			}
		}

		// Render Dynamic Meshes
		{
			auto view = m_Registry.view<MeshComponent, TransformComponent>();
			for (auto entity : view)
			{
				auto [transformComponent, meshComponent] = view.get<TransformComponent, MeshComponent>(entity);
				if (!meshComponent.Visible)
					continue;

				if (AssetManager::IsAssetHandleValid(meshComponent.Mesh))
				{
					auto mesh = AssetManager::GetAsset<Mesh>(meshComponent.Mesh);
					if (mesh && !mesh->IsFlagSet(AssetFlag::Missing))
					{
						Entity e = Entity(entity, this);
						glm::mat4 transform = GetWorldSpaceTransformMatrix(e);

						if (SelectionManager::IsEntityOrAncestorSelected(e))
							renderer->SubmitSelectedMesh(mesh, meshComponent.SubmeshIndex, meshComponent.MaterialTable, transform, GetModelSpaceBoneTransforms(meshComponent.BoneEntityIds, mesh));
						else
							renderer->SubmitMesh(mesh, meshComponent.SubmeshIndex, meshComponent.MaterialTable, transform, GetModelSpaceBoneTransforms(meshComponent.BoneEntityIds, mesh));
					}
				}
			}
		}

		RenderPhysicsDebug(renderer, true);

		renderer->EndScene();

		// Render 2D
		if (renderer->GetFinalPassImage())
		{
			Ref<Renderer2D> renderer2D = renderer->GetRenderer2D();
			Ref<Renderer2D> screenSpaceRenderer2D = renderer->GetScreenSpaceRenderer2D();

			bool hasScreenSpaceEntities = false;

			renderer2D->ResetStats();
			renderer2D->BeginScene(editorCamera.GetViewProjection(), editorCamera.GetViewMatrix());
			renderer2D->SetTargetFramebuffer(renderer->GetExternalCompositeFramebuffer());
			{

				auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
				for (auto entity : view)
				{
					Entity e = Entity(entity, this);
					auto [transformComponent, spriteRendererComponent] = view.get<TransformComponent, SpriteRendererComponent>(entity);
					if (spriteRendererComponent.Texture)
					{
						if (AssetManager::IsAssetHandleValid(spriteRendererComponent.Texture))
						{
							Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(spriteRendererComponent.Texture);
							renderer2D->DrawQuad(GetWorldSpaceTransformMatrix(e), texture, spriteRendererComponent.TilingFactor,
							spriteRendererComponent.Color, spriteRendererComponent.UVStart, spriteRendererComponent.UVEnd);
						}
					}
					else
					{
						renderer2D->DrawQuad(GetWorldSpaceTransformMatrix(e), spriteRendererComponent.Color);
					}
				}
				auto group = m_Registry.group<TransformComponent>(entt::get<TextComponent>);
				for (auto entity : group)
				{
					auto [transformComponent, textComponent] = group.get<TransformComponent, TextComponent>(entity);

					// Defer screen space elements to next pass
					if (textComponent.ScreenSpace)
					{
						hasScreenSpaceEntities = true;
						continue;
					}

					Entity e = Entity(entity, this);
					auto font = Font::GetFontAssetForTextComponent(textComponent);
					renderer2D->DrawString(textComponent.TextString, font, GetWorldSpaceTransformMatrix(e), textComponent.MaxWidth, textComponent.Color, textComponent.LineSpacing, textComponent.Kerning);
				}
			}

			// Save line width
			float lineWidth = renderer2D->GetLineWidth();

			// Debug Renderer
			{
				Ref<DebugRenderer> debugRenderer = renderer->GetDebugRenderer();
				auto& renderQueue = debugRenderer->GetRenderQueue();
				for (auto&& func : renderQueue)
					func(renderer2D);

				debugRenderer->ClearRenderQueue();
			}

			Render2DPhysicsDebug(renderer, renderer2D, true);

			renderer2D->EndScene();

			if (hasScreenSpaceEntities)
			{
				// Render screen space elements
				screenSpaceRenderer2D->ResetStats();
				screenSpaceRenderer2D->BeginScene(renderer->GetScreenSpaceProjectionMatrix(), glm::mat4(1.0f));
				screenSpaceRenderer2D->SetTargetFramebuffer(renderer->GetExternalCompositeFramebuffer());

				{
					auto group = m_Registry.group<TransformComponent>(entt::get<TextComponent>);
					for (auto entity : group)
					{
						auto [transformComponent, textComponent] = group.get<TransformComponent, TextComponent>(entity);
						// Already rendered non-screenspace elements
						if (!textComponent.ScreenSpace)
							continue;

						Entity e = Entity(entity, this);
						auto font = Font::GetFontAssetForTextComponent(textComponent);

						if (textComponent.DropShadow)
						{
							glm::mat4 scale = glm::scale(glm::mat4(1.0f), transformComponent.Scale);
							glm::mat4 transformShadow = glm::translate(glm::mat4(1.0f), { transformComponent.Translation.x + textComponent.ShadowDistance, transformComponent.Translation.y - textComponent.ShadowDistance, transformComponent.Translation.z + 0.01f }) * scale;
							screenSpaceRenderer2D->DrawString(textComponent.TextString, font, transformShadow, textComponent.MaxWidth, textComponent.ShadowColor, textComponent.LineSpacing, textComponent.Kerning);
						}
						screenSpaceRenderer2D->DrawString(textComponent.TextString, font, transformComponent.GetTransform(), textComponent.MaxWidth, textComponent.Color, textComponent.LineSpacing, textComponent.Kerning);
					}
				}

				screenSpaceRenderer2D->EndScene();

				// Restore line width (in case it was changed)
				screenSpaceRenderer2D->SetLineWidth(lineWidth);
			}
		}
	}


	void Scene::OnAnimationGraphCompiled(AssetHandle AnimationGraphHandle)
	{
		auto view = GetAllEntitiesWith<AnimationComponent>();
		for (auto entity : view)
		{
			Entity e = { entity, this };
			auto& anim = e.GetComponent<AnimationComponent>();
			if (anim.AnimationGraphHandle == AnimationGraphHandle)
			{
				auto animationGraphAsset = AssetManager::GetAsset<AnimationGraphAsset>(anim.AnimationGraphHandle);
				if (
					animationGraphAsset && animationGraphAsset->Prototype &&
					(!anim.AnimationGraph || anim.AnimationGraph->ID != animationGraphAsset->Prototype->ID)
				)
				{
					auto oldGraph = anim.AnimationGraph;
					anim.AnimationGraph = animationGraphAsset->CreateInstance();

					// Attempt to copy over old graph input values to new graph.
					// New graph may not have the same inputs, and even if it does, the types may not match.
					if (oldGraph)
					{
						for (auto [id, oldValue] : oldGraph->Ins)
						{
							try
							{
								auto newValue = anim.AnimationGraph->InValue(id);
								if (oldValue.isInt32() && newValue.isInt32())          newValue.set(oldValue.getInt32());
								else if (oldValue.isInt64() && newValue.isInt64())     newValue.set(oldValue.getInt64());
								else if (oldValue.isFloat32() && newValue.isFloat32()) newValue.set(oldValue.getFloat32());
								else if (oldValue.isFloat64() && newValue.isFloat64()) newValue.set(oldValue.getFloat64());
							}
							catch (const std::out_of_range&)
							{
								;
							}
						}
					}

					anim.BoneEntityIds = FindBoneEntityIds(e, e, anim.AnimationGraph);
				}
			}
		}
	}


	void Scene::RenderPhysicsDebug(Ref<SceneRenderer> renderer, bool runtime)
	{
		if (!renderer->GetOptions().ShowPhysicsColliders)
			return;

		SceneRendererOptions::PhysicsColliderView colliderView = renderer->GetOptions().PhysicsColliderMode;

		if (colliderView == SceneRendererOptions::PhysicsColliderView::SelectedEntity)
		{
			if (SelectionManager::GetSelectionCount(SelectionContext::Scene) == 0)
				return;

			for (auto entityID : SelectionManager::GetSelections(SelectionContext::Scene))
			{
				Entity entity = GetEntityWithUUID(entityID);

				if (entity.HasComponent<BoxColliderComponent>())
				{
					Ref<StaticMesh> boxDebugMesh = AssetManager::GetAsset<StaticMesh>(PhysicsSystem::GetMeshCache().GetBoxDebugMesh());
					glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);
					const auto& collider = entity.GetComponent<BoxColliderComponent>();
					glm::mat4 colliderTransform = glm::translate(glm::mat4(1.0), collider.Offset) * glm::scale(glm::mat4(1.0f), collider.HalfSize * 2.0f);
					renderer->SubmitPhysicsStaticDebugMesh(boxDebugMesh, transform * colliderTransform);
				}

				if (entity.HasComponent<SphereColliderComponent>())
				{
					Ref<StaticMesh> sphereDebugMesh = AssetManager::GetAsset<StaticMesh>(PhysicsSystem::GetMeshCache().GetSphereDebugMesh());
					glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);
					const auto& collider = entity.GetComponent<SphereColliderComponent>();
					glm::mat4 colliderTransform = glm::translate(glm::mat4(1.0), collider.Offset) * glm::scale(glm::mat4(1.0f), glm::vec3(collider.Radius * 2.0f));
					renderer->SubmitPhysicsStaticDebugMesh(sphereDebugMesh, transform * colliderTransform);
				}

				if (entity.HasComponent<CapsuleColliderComponent>())
				{
					Ref<StaticMesh> capsuleDebugMesh = AssetManager::GetAsset<StaticMesh>(PhysicsSystem::GetMeshCache().GetCapsuleDebugMesh());
					glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);
					const auto& collider = entity.GetComponent<CapsuleColliderComponent>();
					glm::mat4 colliderTransform = glm::translate(glm::mat4(1.0), collider.Offset) * glm::scale(glm::mat4(1.0f), glm::vec3(collider.Radius * 2.0f, collider.HalfHeight * 2.0f, collider.Radius * 2.0f));
					renderer->SubmitPhysicsStaticDebugMesh(capsuleDebugMesh, transform * colliderTransform);
				}

				if (entity.HasComponent<MeshColliderComponent>())
				{
					glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);
					const auto& collider = entity.GetComponent<MeshColliderComponent>();

					Ref<MeshColliderAsset> colliderAsset = AssetManager::GetAsset<MeshColliderAsset>(collider.ColliderAsset);
					if (!colliderAsset)
						return;

					Ref<Mesh> simpleDebugMesh = PhysicsSystem::GetMeshCache().GetDebugMesh(colliderAsset);
					if (simpleDebugMesh && colliderAsset->CollisionComplexity != ECollisionComplexity::UseComplexAsSimple)
					{
						renderer->SubmitPhysicsDebugMesh(simpleDebugMesh, collider.SubmeshIndex, transform);
					}

					Ref<StaticMesh> complexDebugMesh = PhysicsSystem::GetMeshCache().GetDebugStaticMesh(colliderAsset);
					if (complexDebugMesh && colliderAsset->CollisionComplexity != ECollisionComplexity::UseSimpleAsComplex)
					{
						renderer->SubmitPhysicsStaticDebugMesh(complexDebugMesh, transform, false);
					}
				}
			}
		}
		else
		{
			{
				auto view = m_Registry.view<BoxColliderComponent>();
				Ref<StaticMesh> boxDebugMesh = AssetManager::GetAsset<StaticMesh>(PhysicsSystem::GetMeshCache().GetBoxDebugMesh());
				for (auto entity : view)
				{
					Entity e = { entity, this };
					glm::mat4 transform = GetWorldSpaceTransformMatrix(e);
					const auto& collider = e.GetComponent<BoxColliderComponent>();
					glm::mat4 colliderTransform = glm::translate(glm::mat4(1.0), collider.Offset) * glm::scale(glm::mat4(1.0f), collider.HalfSize * 2.0f);
					renderer->SubmitPhysicsStaticDebugMesh(boxDebugMesh, transform * colliderTransform);
				}
			}

			{
				auto view = m_Registry.view<SphereColliderComponent>();
				Ref<StaticMesh> sphereDebugMesh = AssetManager::GetAsset<StaticMesh>(PhysicsSystem::GetMeshCache().GetSphereDebugMesh());
				for (auto entity : view)
				{
					Entity e = { entity, this };
					glm::mat4 transform = GetWorldSpaceTransformMatrix(e);
					const auto& collider = e.GetComponent<SphereColliderComponent>();
					glm::mat4 colliderTransform = glm::translate(glm::mat4(1.0), collider.Offset) * glm::scale(glm::mat4(1.0f), glm::vec3(collider.Radius * 2.0f));
					renderer->SubmitPhysicsStaticDebugMesh(sphereDebugMesh, transform * colliderTransform);
				}
			}

			{
				auto view = m_Registry.view<CapsuleColliderComponent>();
				Ref<StaticMesh> capsuleDebugMesh = AssetManager::GetAsset<StaticMesh>(PhysicsSystem::GetMeshCache().GetCapsuleDebugMesh());
				for (auto entity : view)
				{
					Entity e = { entity, this };
					glm::mat4 transform = GetWorldSpaceTransformMatrix(e);
					const auto& collider = e.GetComponent<CapsuleColliderComponent>();
					glm::mat4 colliderTransform = glm::translate(glm::mat4(1.0), collider.Offset) * glm::scale(glm::mat4(1.0f), glm::vec3(collider.Radius * 2.0f, collider.HalfHeight * 2.0f, collider.Radius * 2.0f));
					renderer->SubmitPhysicsStaticDebugMesh(capsuleDebugMesh, transform * colliderTransform);
				}
			}

			{
				auto view = m_Registry.view<MeshColliderComponent>();
				for (auto entity : view)
				{
					Entity e = { entity, this };
					glm::mat4 transform = GetWorldSpaceTransformMatrix(e);
					const auto& collider = e.GetComponent<MeshColliderComponent>();

					Ref<MeshColliderAsset> colliderAsset = AssetManager::GetAsset<MeshColliderAsset>(collider.ColliderAsset);
					if (!colliderAsset)
						continue;

					Ref<Mesh> simpleDebugMesh = PhysicsSystem::GetMeshCache().GetDebugMesh(colliderAsset);
					if (simpleDebugMesh && colliderAsset->CollisionComplexity != ECollisionComplexity::UseComplexAsSimple)
						renderer->SubmitPhysicsDebugMesh(simpleDebugMesh, collider.SubmeshIndex, transform);

					Ref<StaticMesh> complexDebugMesh = PhysicsSystem::GetMeshCache().GetDebugStaticMesh(colliderAsset);
					if (complexDebugMesh && colliderAsset->CollisionComplexity != ECollisionComplexity::UseSimpleAsComplex)
						renderer->SubmitPhysicsStaticDebugMesh(complexDebugMesh, transform, false);
				}
			}
		}
	}

	void Scene::Render2DPhysicsDebug(Ref<SceneRenderer> renderer, Ref<Renderer2D> renderer2D, bool runtime)
	{
		if (!renderer->GetOptions().ShowPhysicsColliders)
			return;

		SceneRendererOptions::PhysicsColliderView colliderView = renderer->GetOptions().PhysicsColliderMode;

		if (colliderView == SceneRendererOptions::PhysicsColliderView::SelectedEntity)
		{
			if (SelectionManager::GetSelectionCount(SelectionContext::Scene) == 0)
				return;

			for (auto entityID : SelectionManager::GetSelections(SelectionContext::Scene))
			{
				Entity entity = GetEntityWithUUID(entityID);

				if (entity.HasComponent<BoxCollider2DComponent>())
				{
					const auto& tc = GetWorldSpaceTransform(entity);
					auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();
					renderer2D->DrawRotatedRect(glm::vec2{ tc.Translation.x, tc.Translation.y } + bc2d.Offset, (2.0f * bc2d.Size) * glm::vec2(tc.Scale), tc.GetRotationEuler().z, { 0.25f, 0.6f, 1.0f, 1.0f });
				}

				if (entity.HasComponent<CircleCollider2DComponent>())
				{
					const auto& tc = GetWorldSpaceTransform(entity);
					auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();
					renderer2D->DrawCircle(glm::vec3{ tc.Translation.x, tc.Translation.y, 0.0f } + glm::vec3(cc2d.Offset, 0.0f), glm::vec3(0.0f), cc2d.Radius * tc.Scale.x, { 0.25f, 0.6f, 1.0f, 1.0f });
				}
			}
		}
		else
		{
			{
				auto view = m_Registry.view<BoxCollider2DComponent>();
				for (auto entity : view)
				{
					Entity e = { entity, this };
					const auto& tc = GetWorldSpaceTransform(e);
					auto& bc2d = e.GetComponent<BoxCollider2DComponent>();
					renderer2D->DrawRotatedRect(glm::vec2{ tc.Translation.x, tc.Translation.y } + bc2d.Offset, (2.0f * bc2d.Size) * glm::vec2(tc.Scale), tc.GetRotationEuler().z, { 0.25f, 0.6f, 1.0f, 1.0f });
				}
			}

			{
				auto view = m_Registry.view<CircleCollider2DComponent>();
				for (auto entity : view)
				{
					Entity e = { entity, this };
					const auto& tc = GetWorldSpaceTransform(e);
					auto& cc2d = e.GetComponent<CircleCollider2DComponent>();
					renderer2D->DrawCircle(glm::vec3{ tc.Translation.x, tc.Translation.y, 0.0f } + glm::vec3(cc2d.Offset, 0.0f), glm::vec3(0.0f), cc2d.Radius * tc.Scale.x, { 0.25f, 0.6f, 1.0f, 1.0f });
				}
			}
		}
	}

	void Scene::OnEvent(Event& e)
	{
	}

	void Scene::OnRuntimeStart()
	{
		ZONG_PROFILE_FUNC();

		m_IsPlaying = true;
		m_ShouldSimulate = true;

		MiniAudioEngine::SetSceneContext(this);

		Ref<Scene> _this = this;
		Application::Get().DispatchEvent<ScenePreStartEvent, true>(_this);

		// Box2D physics
		auto sceneView = m_Registry.view<Box2DWorldComponent>();
		auto& world = m_Registry.get<Box2DWorldComponent>(sceneView.front()).World;

		{
			auto view = m_Registry.view<RigidBody2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				UUID entityID = e.GetComponent<IDComponent>().ID;
				TransformComponent& transform = e.GetComponent<TransformComponent>();
				auto& rigidBody2D = m_Registry.get<RigidBody2DComponent>(entity);

				b2BodyDef bodyDef;
				if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Static)
					bodyDef.type = b2_staticBody;
				else if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Dynamic)
					bodyDef.type = b2_dynamicBody;
				else if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Kinematic)
					bodyDef.type = b2_kinematicBody;
				bodyDef.position.Set(transform.Translation.x, transform.Translation.y);

				bodyDef.angle = transform.GetRotationEuler().z;

				b2Body* body = world->CreateBody(&bodyDef);
				body->SetFixedRotation(rigidBody2D.FixedRotation);
				b2MassData massData;
				body->GetMassData(&massData);
				massData.mass = rigidBody2D.Mass;
				body->SetMassData(&massData);
				body->SetGravityScale(rigidBody2D.GravityScale);
				body->SetLinearDamping(rigidBody2D.LinearDrag);
				body->SetAngularDamping(rigidBody2D.AngularDrag);
				body->SetBullet(rigidBody2D.IsBullet);
				body->GetUserData().pointer = (uintptr_t)entityID;
				rigidBody2D.RuntimeBody = body;
			}
		}

		{
			auto view = m_Registry.view<BoxCollider2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& transform = e.Transform();

				auto& boxCollider2D = m_Registry.get<BoxCollider2DComponent>(entity);
				if (e.HasComponent<RigidBody2DComponent>())
				{
					auto& rigidBody2D = e.GetComponent<RigidBody2DComponent>();
					ZONG_CORE_ASSERT(rigidBody2D.RuntimeBody);
					b2Body* body = static_cast<b2Body*>(rigidBody2D.RuntimeBody);

					b2PolygonShape polygonShape;
					polygonShape.SetAsBox(transform.Scale.x * boxCollider2D.Size.x, transform.Scale.y * boxCollider2D.Size.y, b2Vec2(boxCollider2D.Offset.x, boxCollider2D.Offset.y), 0.0f);

					b2FixtureDef fixtureDef;
					fixtureDef.shape = &polygonShape;
					fixtureDef.density = boxCollider2D.Density;
					fixtureDef.friction = boxCollider2D.Friction;
					body->CreateFixture(&fixtureDef);
				}
			}
		}

		{
			auto view = m_Registry.view<CircleCollider2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& transform = e.Transform();

				auto& circleCollider2D = m_Registry.get<CircleCollider2DComponent>(entity);
				if (e.HasComponent<RigidBody2DComponent>())
				{
					auto& rigidBody2D = e.GetComponent<RigidBody2DComponent>();
					ZONG_CORE_ASSERT(rigidBody2D.RuntimeBody);
					b2Body* body = static_cast<b2Body*>(rigidBody2D.RuntimeBody);

					b2CircleShape circleShape;
					circleShape.m_p = b2Vec2(circleCollider2D.Offset.x, circleCollider2D.Offset.y);
					circleShape.m_radius = transform.Scale.x * circleCollider2D.Radius;

					b2FixtureDef fixtureDef;
					fixtureDef.shape = &circleShape;
					fixtureDef.density = circleCollider2D.Density;
					fixtureDef.friction = circleCollider2D.Friction;
					body->CreateFixture(&fixtureDef);
				}
			}
		}

		auto& physicsSceneComponent = m_Registry.emplace<PhysicsSceneComponent>(m_SceneEntity);
		physicsSceneComponent.PhysicsWorld = PhysicsSystem::CreatePhysicsScene(_this);

		ScriptEngine::InitializeRuntime();

		{	//--- Make sure we have an audio listener ---
			//===========================================

			// If no audio listeners were added by the user, create one on the main camera

			// TODO: make a user option to automatically set active listener to the main camera vs override

			// Main Camera should have listener component in case of fallback
			Entity mainCam = GetMainCameraEntity();
			Entity listener;

			auto view = m_Registry.view<AudioListenerComponent>();
			bool listenerFound = !view.empty();

			if (mainCam.m_EntityHandle != entt::null)
			{
				if (!mainCam.HasComponent<AudioListenerComponent>())
					mainCam.AddComponent<AudioListenerComponent>();
			}

			if (listenerFound)
			{
				for (auto entity : view)
				{
					listener = { entity, this };
					if (listener.GetComponent<AudioListenerComponent>().Active)
					{
						listenerFound = true;
						break;
					}
					listenerFound = false;
				}
			}

			// If found listener has not been set Active, fallback to using Main Camera
			if (!listenerFound)
				listener = mainCam;

			// Don't update position if we faild to get active listener and Main Camera
			if (listener.m_EntityHandle != entt::null)
			{
				// Initialize listener's position
				auto transform = GetAudioTransform(GetWorldSpaceTransform(listener));
				MiniAudioEngine::Get().UpdateListenerPosition(transform);

				auto& listenerComponent = listener.GetComponent<AudioListenerComponent>();
				MiniAudioEngine::Get().UpdateListenerConeAttenuation(listenerComponent.ConeInnerAngleInRadians,
					listenerComponent.ConeOuterAngleInRadians,
					listenerComponent.ConeOuterGain);
			}
		}

		{	//--- Initialize audio component sound positions ---
			//==================================================

			auto view = m_Registry.view<AudioComponent>();

			std::vector<SoundSourceUpdateData> updateData;
			updateData.reserve(view.size());

			for (auto entity : view)
			{
				auto& audioComponent = view.get(entity);

				Entity e = { entity, this };
				auto transform = GetAudioTransform(GetWorldSpaceTransform(e));

				glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
				updateData.emplace_back(SoundSourceUpdateData{ e.GetUUID(),
				transform,
				velocity,
				audioComponent.VolumeMultiplier,
				audioComponent.PitchMultiplier });
			}

			//--- Submit values to AudioEngine to update associated sound sources ---
			//-----------------------------------------------------------------------
			MiniAudioEngine::Get().SubmitSourceUpdateData(std::move(updateData));
		}

		m_IsPlaying = true;
		m_ShouldSimulate = true;

		physicsSceneComponent.PhysicsWorld->OnScenePostStart();

		Application::Get().DispatchEvent<ScenePostStartEvent, true>(_this);

		// Now that the scene has initialized, we can start sounds marked to "play on awake"
		// or the sounds that were deserialized in playing state (in the future).
		MiniAudioEngine::OnRuntimePlaying(m_SceneID);
	}

	void Scene::OnRuntimeStop()
	{
		Ref<Scene> _this = this;
		Application::Get().DispatchEvent<ScenePreStopEvent, true>(_this);

		ScriptEngine::ShutdownRuntime();

		// TODO: Destroy the entire scene

		auto& physicsSceneComponent = m_Registry.get<PhysicsSceneComponent>(m_SceneEntity);
		physicsSceneComponent.PhysicsWorld->Destroy();
		physicsSceneComponent.PhysicsWorld = nullptr;
		m_Registry.remove<PhysicsSceneComponent>(m_SceneEntity);

		MiniAudioEngine::SetSceneContext(nullptr);
		m_IsPlaying = false;
		m_ShouldSimulate = false;

		Application::Get().DispatchEvent<ScenePostStopEvent, true>(_this);
	}

	void Scene::OnSimulationStart()
	{
		Ref<Scene> _this = this;
		Application::Get().DispatchEvent<ScenePreStartEvent, true>(_this);

		// Box2D physics
		auto sceneView = m_Registry.view<Box2DWorldComponent>();
		auto& world = m_Registry.get<Box2DWorldComponent>(sceneView.front()).World;

		{
			auto view = m_Registry.view<RigidBody2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				UUID entityID = e.GetComponent<IDComponent>().ID;
				TransformComponent& transform = e.GetComponent<TransformComponent>();
				auto& rigidBody2D = m_Registry.get<RigidBody2DComponent>(entity);

				b2BodyDef bodyDef;
				if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Static)
					bodyDef.type = b2_staticBody;
				else if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Dynamic)
					bodyDef.type = b2_dynamicBody;
				else if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Kinematic)
					bodyDef.type = b2_kinematicBody;
				bodyDef.position.Set(transform.Translation.x, transform.Translation.y);

				bodyDef.angle = transform.GetRotationEuler().z;

				b2Body* body = world->CreateBody(&bodyDef);
				body->SetFixedRotation(rigidBody2D.FixedRotation);
				body->GetUserData().pointer = (uintptr_t)new UUID(entityID);
				rigidBody2D.RuntimeBody = body;
			}
		}

		{
			auto view = m_Registry.view<BoxCollider2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& transform = e.Transform();

				auto& boxCollider2D = m_Registry.get<BoxCollider2DComponent>(entity);
				if (e.HasComponent<RigidBody2DComponent>())
				{
					auto& rigidBody2D = e.GetComponent<RigidBody2DComponent>();
					ZONG_CORE_ASSERT(rigidBody2D.RuntimeBody);
					b2Body* body = static_cast<b2Body*>(rigidBody2D.RuntimeBody);

					b2PolygonShape polygonShape;
					polygonShape.SetAsBox(transform.Scale.x * boxCollider2D.Size.x, transform.Scale.y * boxCollider2D.Size.y, b2Vec2(boxCollider2D.Offset.x, boxCollider2D.Offset.y), 0.0f);

					b2FixtureDef fixtureDef;
					fixtureDef.shape = &polygonShape;
					fixtureDef.density = boxCollider2D.Density;
					fixtureDef.friction = boxCollider2D.Friction;
					body->CreateFixture(&fixtureDef);
				}
			}
		}

		{
			auto view = m_Registry.view<CircleCollider2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& transform = e.Transform();

				auto& circleCollider2D = m_Registry.get<CircleCollider2DComponent>(entity);
				if (e.HasComponent<RigidBody2DComponent>())
				{
					auto& rigidBody2D = e.GetComponent<RigidBody2DComponent>();
					ZONG_CORE_ASSERT(rigidBody2D.RuntimeBody);
					b2Body* body = static_cast<b2Body*>(rigidBody2D.RuntimeBody);

					b2CircleShape circleShape;
					circleShape.m_p = b2Vec2(circleCollider2D.Offset.x, circleCollider2D.Offset.y);
					circleShape.m_radius = transform.Scale.x * circleCollider2D.Radius;

					b2FixtureDef fixtureDef;
					fixtureDef.shape = &circleShape;
					fixtureDef.density = circleCollider2D.Density;
					fixtureDef.friction = circleCollider2D.Friction;
					body->CreateFixture(&fixtureDef);
				}
			}
		}

		auto& physicsSceneComponent = m_Registry.emplace<PhysicsSceneComponent>(m_SceneEntity);
		physicsSceneComponent.PhysicsWorld = PhysicsSystem::CreatePhysicsScene(_this);

		m_ShouldSimulate = true;

		physicsSceneComponent.PhysicsWorld->OnScenePostStart();

		Application::Get().DispatchEvent<ScenePostStartEvent, true>(_this);
	}

	void Scene::OnSimulationStop()
	{
		Ref<Scene> _this = this;
		Application::Get().DispatchEvent<ScenePreStopEvent, true>(_this);

		{
			auto view = m_Registry.view<RigidBody2DComponent>();

			for (auto entity : view)
			{
				auto& rb2d = view.get<RigidBody2DComponent>(entity);
				b2Body* body = (b2Body*)rb2d.RuntimeBody;
				delete (UUID*)body->GetUserData().pointer;
			}
		}

		auto& physicsSceneComponent = m_Registry.get<PhysicsSceneComponent>(m_SceneEntity);
		physicsSceneComponent.PhysicsWorld->Destroy();
		physicsSceneComponent.PhysicsWorld = nullptr;
		m_Registry.remove<PhysicsSceneComponent>(m_SceneEntity);

		m_ShouldSimulate = false;

		Application::Get().DispatchEvent<ScenePostStopEvent, true>(_this);
	}

	void Scene::OnAudioComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		auto entityID = registry.get<IDComponent>(entity).ID;
		ZONG_CORE_ASSERT(m_EntityIDMap.find(entityID) != m_EntityIDMap.end());
		registry.get<AudioComponent>(entity).ParentHandle = entityID;
		MiniAudioEngine::Get().OnAudibleEntityCreated(m_EntityIDMap.at(entityID));
	}

	void Scene::OnAudioComponentDestroy(entt::registry& registry, entt::entity entity)
	{
#if 0
		if (registry.has<IDComponent>(entity))
		{
			auto entityID = registry.get<IDComponent>(entity).ID;
			MiniAudioEngine::Get().OnAudioComponentDestroy(GetUUID(), entityID);
		}
#endif
	}

	void Scene::OnMeshColliderComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		ZONG_PROFILE_FUNC();

		Entity e = { entity, this };
		auto& component = e.GetComponent<MeshColliderComponent>();
		if (AssetManager::IsAssetHandleValid(component.ColliderAsset))
			PhysicsSystem::GetOrCreateColliderAsset(e, component);
	}

	void Scene::OnMeshColliderComponentDestroy(entt::registry& registry, entt::entity entity)
	{
	}

	std::vector<glm::mat4> Scene::GetModelSpaceBoneTransforms(const std::vector<UUID>& boneEntityIds, Ref<Mesh> mesh)
	{
		std::vector<glm::mat4> boneTransforms(boneEntityIds.size());

		if (mesh->HasSkeleton())
		{
			const auto& skeleton = mesh->GetMeshSource()->GetSkeleton();

			// Can get mismatches if user changes which mesh an entity refers to after the bone entities have been set up
			// TODO(0x): need a better way to handle the bone entities
			//ZONG_CORE_ASSERT(boneEntityIds.size() == skeleton.GetNumBones(), "Wrong number of boneEntityIds for mesh skeleton!");
			for (uint32_t i = 0; i < std::min(skeleton.GetNumBones(), (uint32_t)boneEntityIds.size()); ++i)
			{
				auto boneEntity = TryGetEntityWithUUID(boneEntityIds[i]);
				glm::mat4 localTransform = boneEntity ? boneEntity.GetComponent<TransformComponent>().GetTransform() : glm::identity<glm::mat4>();
				auto parentIndex = skeleton.GetParentBoneIndex(i);
				boneTransforms[i] = (parentIndex == Skeleton::NullIndex) ? localTransform : boneTransforms[parentIndex] * localTransform;
			}
		}
		return boneTransforms;
	}


	void HandleAnimationEvent(void* context, Identifier eventID)
	{
		auto* sc = static_cast<ScriptComponent*>(context);
		ScriptEngine::CallMethod(sc->ManagedInstance, "OnAnimationEventInternal", eventID);
	}


	void Scene::UpdateAnimation(Timestep ts, bool isRuntime)
	{
		auto view = GetAllEntitiesWith<AnimationComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };
			auto& anim = entity.GetComponent<AnimationComponent>();

			// If animation component has no AnimationGraph, or no bone entities then there is nothing to do
			if (anim.AnimationGraph && anim.BoneEntityIds.size() > 0)
			{
				anim.AnimationGraph->Process(ts);

				// Note: assumption here is that anim.BoneEntityIds[i] <=> AnimationGraph.Transform[i+1]  (0 being the artificial track for root transform)
				// So there is no need to look up the mapping of mesh -> joint index
				// index 0 is root motion
				// index 1 is fake root bone
				const Pose* pose = anim.AnimationGraph->GetPose();
				for (size_t i = 0; i < anim.BoneEntityIds.size(); ++i)
				{
					auto boneTransformEntity = TryGetEntityWithUUID(anim.BoneEntityIds[i]);
					if (boneTransformEntity)
					{
						// Note: we're assuming there is always a transform component
						auto& transform = boneTransformEntity.GetComponent<TransformComponent>();
						transform.Translation = pose->BoneTransforms[i + 1].Translation;
						transform.SetRotation(pose->BoneTransforms[i + 1].Rotation);
						transform.Scale = pose->BoneTransforms[i + 1].Scale;
					}
				}

				if (isRuntime)
				{
					auto parentEntity = entity.GetParent();
					// Figure out how to apply the root motion, depending on what components the target entity has
					auto& transform = entity.Transform();
					glm::mat3 rs = transform.GetTransform();
					if (m_ShouldSimulate && entity.HasComponent<CharacterControllerComponent>())
					{
						// 1. target entity is a physics character controller
						//    => apply root motion to character pose
						Ref<CharacterController> controller = GetPhysicsScene()->GetCharacterController(entity);
						ZONG_CORE_ASSERT(controller);
						{
							const glm::vec3 displacement = rs * pose->RootMotion.Translation;
							controller->Move(displacement);
							if (!glm::all(glm::equal(pose->RootMotion.Rotation, glm::identity<glm::quat>(), glm::epsilon<float>())))
								controller->SetRotation(transform.GetRotation() * pose->RootMotion.Rotation);
						}
					}
					else if (m_ShouldSimulate && entity.HasComponent<RigidBodyComponent>())
					{
						// 2. target entity is a physics rigid body.
						//    => apply root motion as kinematic target  (or do nothing if it isnt a kinematic rigidbody. We do not support attempting to convert root motion into physics impulses)
						//
						// QUESTION: Should we even support this?
						// Are there situations where you want to move a kinematic rigidbody via animation root motion? (vs. doing it with character controller, or without any physics at all)
						// NOTE(Peter): Removed this code path for now (with approval from 0x)
						ZONG_CORE_VERIFY(false);
					}
					else
					{
						// 3. either we aren't simulating physics, or the target entity is not a physics body
						//    => apply root motion directly to the target entity's transform
						transform.Translation += rs * pose->RootMotion.Translation;

						// Setting rotation involves some expensive (and not necessarily robust) trignometry.
						// Avoid it if the quaternion is identity.
						if (!glm::all(glm::equal(pose->RootMotion.Rotation, glm::identity<glm::quat>(), glm::epsilon<float>())))
							transform.SetRotation(transform.GetRotation() * pose->RootMotion.Rotation);
					}
				}

				if (entity.HasComponent<ScriptComponent>())
				{
					auto& sc = entity.GetComponent<ScriptComponent>();
					if (ScriptEngine::IsModuleValid(sc.ScriptClassHandle) && ScriptEngine::IsEntityInstantiated(entity))
					{
						anim.AnimationGraph->HandleOutgoingEvents(&sc, HandleAnimationEvent);
					}
				}
				anim.AnimationGraph->HandleOutgoingEvents(nullptr, nullptr);

			}
		}
	}

	void Scene::OnRigidBody2DComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		ZONG_PROFILE_FUNC();

		if (m_IsEditorScene)
			return;

		auto sceneView = registry.view<Box2DWorldComponent>();
		auto& world = registry.get<Box2DWorldComponent>(sceneView.front()).World;

		Entity e = { entity, this };
		UUID entityID = e.GetComponent<IDComponent>().ID;
		TransformComponent& transform = e.GetComponent<TransformComponent>();
		auto& rigidBody2D = registry.get<RigidBody2DComponent>(entity);

		b2BodyDef bodyDef;
		if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Static)
			bodyDef.type = b2_staticBody;
		else if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Dynamic)
			bodyDef.type = b2_dynamicBody;
		else if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Kinematic)
			bodyDef.type = b2_kinematicBody;
		bodyDef.position.Set(transform.Translation.x, transform.Translation.y);

		bodyDef.angle = transform.GetRotationEuler().z;

		b2Body* body = world->CreateBody(&bodyDef);
		body->SetFixedRotation(rigidBody2D.FixedRotation);
		b2MassData massData;
		body->GetMassData(&massData);
		massData.mass = rigidBody2D.Mass;
		body->SetMassData(&massData);
		body->SetGravityScale(rigidBody2D.GravityScale);
		body->SetLinearDamping(rigidBody2D.LinearDrag);
		body->SetAngularDamping(rigidBody2D.AngularDrag);
		body->SetBullet(rigidBody2D.IsBullet);
		body->GetUserData().pointer = (uintptr_t)entityID;
		rigidBody2D.RuntimeBody = body;
	}

	void Scene::OnRigidBody2DComponentDestroy(entt::registry& registry, entt::entity entity)
	{
		ZONG_PROFILE_FUNC();

		if (!m_IsPlaying)
			return;
	}

	void Scene::OnBoxCollider2DComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		ZONG_PROFILE_FUNC();

		if (m_IsEditorScene)
			return;

		Entity e = { entity, this };
		auto& transform = e.Transform();

		auto& boxCollider2D = registry.get<BoxCollider2DComponent>(entity);
		if (e.HasComponent<RigidBody2DComponent>())
		{
			auto& rigidBody2D = e.GetComponent<RigidBody2DComponent>();
			ZONG_CORE_ASSERT(rigidBody2D.RuntimeBody);
			b2Body* body = static_cast<b2Body*>(rigidBody2D.RuntimeBody);

			b2PolygonShape polygonShape;
			polygonShape.SetAsBox(transform.Scale.x * boxCollider2D.Size.x, transform.Scale.y * boxCollider2D.Size.y);

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &polygonShape;
			fixtureDef.density = boxCollider2D.Density;
			fixtureDef.friction = boxCollider2D.Friction;
			body->CreateFixture(&fixtureDef);
		}
	}

	void Scene::OnBoxCollider2DComponentDestroy(entt::registry& registry, entt::entity entity)
	{
	}

	void Scene::OnCircleCollider2DComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		ZONG_PROFILE_FUNC();

		if (m_IsEditorScene)
			return;

		Entity e = { entity, this };
		auto& transform = e.Transform();

		auto& circleCollider2D = registry.get<CircleCollider2DComponent>(entity);
		if (e.HasComponent<RigidBody2DComponent>())
		{
			auto& rigidBody2D = e.GetComponent<RigidBody2DComponent>();
			ZONG_CORE_ASSERT(rigidBody2D.RuntimeBody);
			b2Body* body = static_cast<b2Body*>(rigidBody2D.RuntimeBody);

			b2CircleShape circleShape;
			circleShape.m_radius = transform.Scale.x * circleCollider2D.Radius;

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &circleShape;
			fixtureDef.density = circleCollider2D.Density;
			fixtureDef.friction = circleCollider2D.Friction;
			body->CreateFixture(&fixtureDef);
		}
	}

	void Scene::OnCircleCollider2DComponentDestroy(entt::registry& registry, entt::entity entity)
	{
	}

	void Scene::OnRigidBodyComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		ZONG_PROFILE_FUNC();
		
		if (m_IsEditorScene)
			return;

		Entity e = { entity, this };

		auto physicsScene = GetPhysicsScene();
		if (physicsScene)
			physicsScene->CreateBody(e);
	}

	void Scene::OnRigidBodyComponentDestroy(entt::registry& registry, entt::entity entity)
	{
		ZONG_PROFILE_FUNC();

		if (m_IsEditorScene)
			return;

		Entity e = { entity, this };

		auto physicsScene = GetPhysicsScene();
		if (physicsScene)
			physicsScene->DestroyBody(e);
	}

	void Scene::OnRigidBodyComponentDestroy_ProEdition(Entity entity)
	{
		ZONG_PROFILE_FUNC();

		if (m_IsEditorScene)
			return;

		auto physicsScene = GetPhysicsScene();
		if (physicsScene)
			physicsScene->DestroyBody(entity);
	}

	void Scene::SetViewportSize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;
	}

	Entity Scene::GetMainCameraEntity()
	{
		ZONG_PROFILE_FUNC();

		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& comp = view.get<CameraComponent>(entity);
			if (comp.Primary)
			{
				ZONG_CORE_ASSERT(comp.Camera.GetOrthographicSize() || comp.Camera.GetDegPerspectiveVerticalFOV(), "Camera is not fully initialized");
				return { entity, this };
			}
		}
		return {};
	}


	std::vector<UUID> Scene::GetAllChildren(Entity entity) const
	{
		std::vector<UUID> result;

		for (auto childID : entity.Children())
		{
			result.push_back(childID);

			std::vector<UUID> childChildrenIDs = GetAllChildren(GetEntityWithUUID(childID));
			result.reserve(result.size() + childChildrenIDs.size());
			result.insert(result.end(), childChildrenIDs.begin(), childChildrenIDs.end());
		}

		return result;
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateChildEntity({}, name);
	}

	Entity Scene::CreateChildEntity(Entity parent, const std::string& name)
	{
		ZONG_PROFILE_FUNC();

		auto entity = Entity{ m_Registry.create(), this };
		auto& idComponent = entity.AddComponent<IDComponent>();
		idComponent.ID = {};

		entity.AddComponent<TransformComponent>();
		if (!name.empty())
			entity.AddComponent<TagComponent>(name);

		entity.AddComponent<RelationshipComponent>();

		if (parent)
			entity.SetParent(parent);

		m_EntityIDMap[idComponent.ID] = entity;

		SortEntities();

		return entity;
	}

	Entity Scene::CreateEntityWithID(UUID uuid, const std::string& name, bool shouldSort)
	{
		ZONG_PROFILE_FUNC();

		auto entity = Entity{ m_Registry.create(), this };
		auto& idComponent = entity.AddComponent<IDComponent>();
		idComponent.ID = uuid;

		entity.AddComponent<TransformComponent>();
		if (!name.empty())
			entity.AddComponent<TagComponent>(name);

		entity.AddComponent<RelationshipComponent>();

		ZONG_CORE_ASSERT(m_EntityIDMap.find(uuid) == m_EntityIDMap.end());
		m_EntityIDMap[uuid] = entity;

		if (shouldSort)
			SortEntities();

		return entity;
	}

	void Scene::SubmitToDestroyEntity(Entity entity)
	{
		bool isValid = m_Registry.valid((entt::entity)entity);
		if (!isValid)
		{
			ZONG_CORE_WARN_TAG("Scene", "Trying to destroy invalid entity! entt={0}", (uint32_t)entity);
			return;
		}

		SubmitPostUpdateFunc([entity]() { entity.m_Scene->DestroyEntity(entity); });
	}

	void Scene::DestroyEntity(Entity entity, bool excludeChildren, bool first)
	{
		ZONG_PROFILE_FUNC();

		if (!entity)
			return;

		if (entity.HasComponent<ScriptComponent>())
			ScriptEngine::ShutdownScriptEntity(entity, m_IsEditorScene);

		if (entity.HasComponent<AudioComponent>())
			MiniAudioEngine::Get().OnAudibleEntityDestroy(entity);

		if (!m_IsEditorScene)
		{
			auto physicsWorld = GetPhysicsScene();

			if (entity.HasComponent<RigidBodyComponent>())
				physicsWorld->DestroyBody(entity);

			if (entity.HasComponent<RigidBody2DComponent>())
			{
				auto& world = m_Registry.get<Box2DWorldComponent>(m_SceneEntity).World;
				b2Body* body = (b2Body*)entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
				world->DestroyBody(body);
			}
		}

		if (m_OnEntityDestroyedCallback)
			m_OnEntityDestroyedCallback(entity);

		if (!excludeChildren)
		{
			// NOTE(Yan): don't make this a foreach loop because entt will move the children
			//            vector in memory as entities/components get deleted
			for (size_t i = 0; i < entity.Children().size(); i++)
			{
				auto childId = entity.Children()[i];
				Entity child = GetEntityWithUUID(childId);
				DestroyEntity(child, excludeChildren, false);
			}
		}

		if (first)
		{
			if (auto parent = entity.GetParent(); parent)
				parent.RemoveChild(entity);
		}

		UUID id = entity.GetUUID();

		if (SelectionManager::IsSelected(id))
			SelectionManager::Deselect(id);

		if (entity.HasComponent<RigidBodyComponent>())
			OnRigidBodyComponentDestroy_ProEdition(entity);

		m_Registry.destroy(entity.m_EntityHandle);
		m_EntityIDMap.erase(id);

		SortEntities();
	}

	void Scene::DestroyEntity(UUID entityID, bool excludeChildren, bool first)
	{
		auto it = m_EntityIDMap.find(entityID);
		if (it == m_EntityIDMap.end())
			return;

		DestroyEntity(it->second, excludeChildren, first);
	}

	void Scene::ResetTransformsToMesh(Entity entity, bool resetChildren)
	{
		if (entity.HasComponent<MeshComponent>())
		{
			auto& mc = entity.GetComponent<MeshComponent>();
			if (AssetManager::IsAssetHandleValid(mc.Mesh))
			{
				Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(mc.Mesh);
				const auto& submeshes = mesh->GetMeshSource()->GetSubmeshes();
				if (submeshes.size() > mc.SubmeshIndex)
				{
					entity.Transform().SetTransform(submeshes[mc.SubmeshIndex].LocalTransform);
				}
			}
		}

		if (resetChildren)
		{
			for (UUID childID : entity.Children())
			{
				Entity child = GetEntityWithUUID(childID);
				ResetTransformsToMesh(child, resetChildren);
			}

		}
	}

	template<typename T>
	static void CopyComponent(entt::registry& dstRegistry, entt::registry& srcRegistry, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		auto srcEntities = srcRegistry.view<T>();
		for (auto srcEntity : srcEntities)
		{
			entt::entity destEntity = enttMap.at(srcRegistry.get<IDComponent>(srcEntity).ID);

			auto& srcComponent = srcRegistry.get<T>(srcEntity);
			auto& destComponent = dstRegistry.emplace_or_replace<T>(destEntity, srcComponent);
		}
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		ZONG_PROFILE_FUNC();

		auto parentNewEntity = [&entity, scene = this](Entity newEntity)
		{
			if (auto parent = entity.GetParent(); parent)
			{
				newEntity.SetParentUUID(parent.GetUUID());
				parent.Children().push_back(newEntity.GetUUID());
			}
		};

		if (entity.HasComponent<PrefabComponent>())
		{
			auto prefabID = entity.GetComponent<PrefabComponent>().PrefabID;
			ZONG_CORE_VERIFY(AssetManager::IsAssetHandleValid(prefabID));
			const auto& entityTransform = entity.GetComponent<TransformComponent>();
			glm::vec3 rotation = entityTransform.GetRotationEuler();
			Entity prefabInstance = Instantiate(AssetManager::GetAsset<Prefab>(prefabID), &entityTransform.Translation, &rotation, &entityTransform.Scale);
			parentNewEntity(prefabInstance);
			return prefabInstance;
		}

		Entity newEntity;
		if (entity.HasComponent<TagComponent>())
			newEntity = CreateEntity(entity.GetComponent<TagComponent>().Tag);
		else
			newEntity = CreateEntity();

		CopyComponentIfExists<TransformComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		// NOTE(Peter): We can't use this method for copying the RelationshipComponent since we
		//				need to duplicate the entire child hierarchy and basically reconstruct the entire RelationshipComponent from the ground up
		//CopyComponentIfExists<RelationshipComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<MeshComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<StaticMeshComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<AnimationComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<ScriptComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<CameraComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<SpriteRendererComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<TextComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<RigidBody2DComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<BoxCollider2DComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<CircleCollider2DComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<RigidBodyComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<CharacterControllerComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<FixedJointComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<CompoundColliderComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<BoxColliderComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<SphereColliderComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<CapsuleColliderComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<MeshColliderComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<DirectionalLightComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<PointLightComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<SpotLightComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<SkyLightComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<AudioComponent>(newEntity.m_EntityHandle, m_Registry, entity);
		CopyComponentIfExists<AudioListenerComponent>(newEntity.m_EntityHandle, m_Registry, entity);

#if _DEBUG && 0
		// Check that nothing has been forgotten...
		bool foundAll = true;
		m_Registry.visit(entity, [&](entt::id_type type)
		{
			if (type != entt::type_index<RelationshipComponent>().value())
				bool foundOne = false;
			m_Registry.visit(newEntity, [type, &foundOne](entt::id_type newType) {if (newType == type) foundOne = true; });
			foundAll = foundAll && foundOne;
		});
		ZONG_CORE_ASSERT(foundAll, "At least one component was not duplicated - have you added a new component type and not dealt with it here?");
#endif

		if (newEntity.HasComponent<AnimationComponent>())
		{
			entity.m_Scene->DuplicateAnimationInstance(newEntity, entity);
		}

		auto childIds = entity.Children(); // need to take a copy of children here, because the collection is mutated below
		for (auto childId : childIds)
		{
			Entity childDuplicate = DuplicateEntity(GetEntityWithUUID(childId));

			// At this point childDuplicate is a child of entity, we need to remove it from that entity
			UnparentEntity(childDuplicate, false);

			childDuplicate.SetParentUUID(newEntity.GetUUID());
			newEntity.Children().push_back(childDuplicate.GetUUID());
		}

		parentNewEntity(newEntity);

		BuildBoneEntityIds(newEntity);

		if (newEntity.HasComponent<ScriptComponent>())
			ScriptEngine::DuplicateScriptInstance(entity, newEntity);

		return newEntity;
	}

	Entity Scene::CreatePrefabEntity(Entity entity, Entity parent, const glm::vec3* translation, const glm::vec3* rotation, const glm::vec3* scale)
	{
		ZONG_PROFILE_FUNC();

		ZONG_CORE_VERIFY(entity.HasComponent<PrefabComponent>());

		Entity newEntity = CreateEntity();
		if (parent)
			newEntity.SetParent(parent);

		entity.m_Scene->CopyComponentIfExists<TagComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<PrefabComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<TransformComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<MeshComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<StaticMeshComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<AnimationComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<DirectionalLightComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<PointLightComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<SpotLightComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<SkyLightComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<ScriptComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<CameraComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<SpriteRendererComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<TextComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<RigidBody2DComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<BoxCollider2DComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<CircleCollider2DComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<RigidBodyComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<CharacterControllerComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<FixedJointComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<CompoundColliderComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<BoxColliderComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<SphereColliderComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<CapsuleColliderComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<MeshColliderComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<AudioComponent>(newEntity, m_Registry, entity);
		entity.m_Scene->CopyComponentIfExists<AudioListenerComponent>(newEntity, m_Registry, entity);

		if (translation)
			newEntity.Transform().Translation = *translation;
		if (rotation)
			newEntity.Transform().SetRotationEuler(*rotation);
		if (scale)
			newEntity.Transform().Scale = *scale;

		if (newEntity.HasComponent<AnimationComponent>())
		{
			entity.m_Scene->DuplicateAnimationInstance(newEntity, entity);
		}

		// Create children
		for (auto childId : entity.Children())
			CreatePrefabEntity(entity.m_Scene->GetEntityWithUUID(childId), newEntity);

		BuildBoneEntityIds(newEntity);

		if (!m_IsEditorScene)
		{
			if (newEntity.HasComponent<RigidBodyComponent>())
			{
				GetPhysicsScene()->CreateBody(newEntity, BodyAddType::AddBulk);
			}
			else if (newEntity.HasAny<BoxColliderComponent, SphereColliderComponent, CapsuleColliderComponent, MeshColliderComponent>())
			{
				newEntity.AddComponent<RigidBodyComponent>();
				GetPhysicsScene()->CreateBody(newEntity, BodyAddType::AddBulk);
			}
		}

		if (newEntity.HasComponent<ScriptComponent>())
			ScriptEngine::DuplicateScriptInstance(entity, newEntity);

		return newEntity;
	}

	Entity Scene::Instantiate(Ref<Prefab> prefab, const glm::vec3* translation, const glm::vec3* rotation, const glm::vec3* scale)
	{
		ZONG_PROFILE_FUNC();

		Entity result;

		// TODO: we need a better way of retrieving the "root" entity
		auto entities = prefab->m_Scene->GetAllEntitiesWith<RelationshipComponent>();
		for (auto e : entities)
		{
			Entity entity = { e, prefab->m_Scene.Raw() };
			if (!entity.GetParent())
			{
				result = CreatePrefabEntity(entity, {}, translation, rotation, scale);
				break;
			}
		}

		// Need to rebuild mapping of meshes -> bones because the prefab entity
		// will have newly generated UUIDs for all the bone entities.
		// This has to be done after the entire hierarchy of entities in the prefab
		// has been created.
		BuildBoneEntityIds(result);

		return result;
	}

	Entity Scene::InstantiateChild(Ref<Prefab> prefab, Entity parent, const glm::vec3* translation /*= nullptr*/, const glm::vec3* rotation /*= nullptr*/, const glm::vec3* scale /*= nullptr*/)
	{
		ZONG_PROFILE_FUNC();

		Entity result;

		// TODO: we need a better way of retrieving the "root" entity
		auto entities = prefab->m_Scene->GetAllEntitiesWith<RelationshipComponent>();
		for (auto e : entities)
		{
			Entity entity = { e, prefab->m_Scene.Raw() };
			Entity currentParent = {};

			if (entity.GetParentUUID() == 0)
				currentParent = parent;

			if (!entity.GetParent())
			{
				result = CreatePrefabEntity(entity, currentParent, translation, rotation, scale);
				break;
			}
		}

		// Need to rebuild mapping of meshes -> bones because the prefab entity
		// will have newly generated UUIDs for all the bone entities.
		// This has to be done after the entire hierarchy of entities in the prefab
		// has been created.
		BuildBoneEntityIds(result);

		return result;
	}

	void Scene::BuildMeshEntityHierarchy(Entity parent, Ref<Mesh> mesh, const MeshNode& node, bool generateColliders)
	{
		Ref<MeshSource> meshSource = mesh->GetMeshSource();
		const auto& nodes = meshSource->GetNodes();

		// Skip empty root node
		if (node.IsRoot() && node.Submeshes.size() == 0)
		{
			for (uint32_t child : node.Children)
				BuildMeshEntityHierarchy(parent, mesh, nodes[child], generateColliders);

			return;
		}

		Entity nodeEntity = CreateChildEntity(parent, node.Name);
		nodeEntity.Transform().SetTransform(node.LocalTransform);

		if (node.Submeshes.size() == 1)
		{
			// Node == Mesh in this case
			uint32_t submeshIndex = node.Submeshes[0];
			auto& mc = nodeEntity.AddComponent<MeshComponent>(mesh->Handle, submeshIndex);

			if (generateColliders)
			{
				auto& colliderComponent = nodeEntity.AddComponent<MeshColliderComponent>();
				Ref<MeshColliderAsset> colliderAsset = PhysicsSystem::GetOrCreateColliderAsset(nodeEntity, colliderComponent);
				colliderComponent.ColliderAsset = colliderAsset->Handle;
				colliderComponent.SubmeshIndex = submeshIndex;
				colliderComponent.UseSharedShape = colliderAsset->AlwaysShareShape;
				nodeEntity.AddComponent<RigidBodyComponent>();
			}
		}
		else if (node.Submeshes.size() > 1)
		{
			// Create one entity per child mesh, parented under node
			for (uint32_t i = 0; i < node.Submeshes.size(); i++)
			{
				uint32_t submeshIndex = node.Submeshes[i];

				// NOTE(Yan): original implemenation use to use "mesh name" from assimp;
				//            we don't store that so use node name instead. Maybe we
				//            should store it?
				Entity childEntity = CreateChildEntity(nodeEntity, node.Name);

				childEntity.AddComponent<MeshComponent>(mesh->Handle, submeshIndex);

				if (generateColliders)
				{
					auto& colliderComponent = childEntity.AddComponent<MeshColliderComponent>();
					Ref<MeshColliderAsset> colliderAsset = PhysicsSystem::GetOrCreateColliderAsset(childEntity, colliderComponent);
					colliderComponent.ColliderAsset = colliderAsset->Handle;
					colliderComponent.SubmeshIndex = submeshIndex;
					colliderComponent.UseSharedShape = colliderAsset->AlwaysShareShape;
					childEntity.AddComponent<RigidBodyComponent>();
				}
			}

		}

		for (uint32_t child : node.Children)
			BuildMeshEntityHierarchy(nodeEntity, mesh, nodes[child], generateColliders);
	}

	Entity Scene::InstantiateMesh(Ref<Mesh> mesh, bool generateColliders)
	{
		auto& assetData = Project::GetEditorAssetManager()->GetMetadata(mesh->Handle);
		Entity rootEntity = CreateEntity(assetData.FilePath.stem().string());
		BuildMeshEntityHierarchy(rootEntity, mesh, mesh->GetMeshSource()->GetRootNode(), generateColliders);
		BuildBoneEntityIds(rootEntity);
		return rootEntity;
	}

	void Scene::BuildBoneEntityIds(Entity entity)
	{
		BuildMeshBoneEntityIds(entity, entity);
		BuildAnimationBoneEntityIds(entity, entity);
	}

	void Scene::BuildMeshBoneEntityIds(Entity entity, Entity rootEntity)
	{
		if (entity.HasComponent<MeshComponent>())
		{
			auto& mc = entity.GetComponent<MeshComponent>();
			auto mesh = AssetManager::GetAsset<Mesh>(mc.Mesh);
			if (mesh && mesh->HasSkeleton())
				mc.BoneEntityIds = FindBoneEntityIds(entity, rootEntity, mesh);
		}
		for (auto childId : entity.Children())
		{
			Entity child = GetEntityWithUUID(childId);
			BuildMeshBoneEntityIds(child, rootEntity);
		}
	}

	void Scene::BuildAnimationBoneEntityIds(Entity entity, Entity rootEntity)
	{
		if (entity.HasComponent<AnimationComponent>())
		{
			auto& anim = entity.GetComponent<AnimationComponent>();
			anim.BoneEntityIds = FindBoneEntityIds(entity, rootEntity, anim.AnimationGraph);
		}
		for (auto childId : entity.Children())
		{
			Entity child = GetEntityWithUUID(childId);
			BuildAnimationBoneEntityIds(child, rootEntity);
		}
	}

	std::vector<UUID> Scene::FindBoneEntityIds(Entity entity, Entity rootEntity, Ref<Mesh> mesh)
	{
		std::vector<UUID> boneEntityIds;
		// given an entity, find descendant entities holding the transforms for the specified mesh's bones
		if (mesh && mesh->HasSkeleton())
		{
			Entity rootParentEntity = rootEntity ? rootEntity.GetParent() : rootEntity;
			auto boneNames = mesh->GetMeshSource()->GetSkeleton().GetBoneNames();
			boneEntityIds.reserve(boneNames.size());
			bool foundAtLeastOne = false;
			for (const auto& boneName : boneNames)
			{
				bool found = false;
				Entity e = entity;
				while (e && e != rootParentEntity)
				{
					Entity boneEntity = TryGetDescendantEntityWithTag(e, boneName);
					if (boneEntity)
					{
						boneEntityIds.emplace_back(boneEntity.GetUUID());
						found = true;
						break;
					}
					e = e.GetParent();
				}
				if (found)
					foundAtLeastOne = true;
				else
					boneEntityIds.emplace_back(0);
			}
			if (!foundAtLeastOne)
				boneEntityIds.resize(0);
		}
		return boneEntityIds;
	}

	std::vector<UUID> Scene::FindBoneEntityIds(Entity entity, Entity rootEntity, Ref<AnimationGraph::AnimationGraph> animationGraph)
	{
		if (!animationGraph)
		{
			return {};
		}
			
		std::vector<UUID> boneEntityIds;
		// given a parent entity, find descendant entities holding the transforms for the specified animation graph's skeleton
		if (auto skeleton = animationGraph->GetSkeleton())
		{
			Entity rootParentEntity = rootEntity.GetParent();
			auto boneNames = skeleton->GetBoneNames();
			boneEntityIds.reserve(boneNames.size());
			bool foundAtLeastOne = false;
			for (const auto& boneName : boneNames)
			{
				bool found = false;
				Entity e = entity;
				while (e && e != rootParentEntity)
				{
					Entity boneEntity = TryGetDescendantEntityWithTag(entity, boneName);
					if (boneEntity)
					{
						boneEntityIds.emplace_back(boneEntity.GetUUID());
						found = true;
						break;
					}
					e = e.GetParent();
				}
				if (found)
					foundAtLeastOne = true;
				else
					boneEntityIds.emplace_back(0);
			}
			if (!foundAtLeastOne)
				boneEntityIds.resize(0);
		}
		return boneEntityIds;
	}

	Entity Scene::GetEntityWithUUID(UUID id) const
	{
		//ZONG_PROFILE_FUNC();
		ZONG_CORE_VERIFY(m_EntityIDMap.find(id) != m_EntityIDMap.end(), "Invalid entity ID or entity doesn't exist in scene!");
		return m_EntityIDMap.at(id);
	}

	Entity Scene::TryGetEntityWithUUID(UUID id) const
	{
		//ZONG_PROFILE_FUNC();
		if (const auto iter = m_EntityIDMap.find(id); iter != m_EntityIDMap.end())
			return iter->second;
		return Entity{};
	}

	Entity Scene::TryGetEntityWithTag(const std::string& tag)
	{
		//ZONG_PROFILE_FUNC();
		auto entities = GetAllEntitiesWith<TagComponent>();
		for (auto e : entities)
		{
			if (entities.get<TagComponent>(e).Tag == tag)
				return Entity(e, const_cast<Scene*>(this));
		}
		return Entity{};
	}

	Entity Scene::TryGetDescendantEntityWithTag(Entity entity, const std::string& tag)
	{
		//ZONG_PROFILE_FUNC();
		if (entity)
		{
			if (entity.GetComponent<TagComponent>().Tag == tag)
				return entity;

			for (const auto childId : entity.Children())
			{
				Entity descendant = TryGetDescendantEntityWithTag(GetEntityWithUUID(childId), tag);
				if (descendant)
					return descendant;
			}
		}
		return {};
	}

	void Scene::SortEntities()
	{
		m_Registry.sort<IDComponent>([&](const auto lhs, const auto rhs)
		{
			auto lhsEntity = m_EntityIDMap.find(lhs.ID);
			auto rhsEntity = m_EntityIDMap.find(rhs.ID);
			return static_cast<uint32_t>(lhsEntity->second) < static_cast<uint32_t>(rhsEntity->second);
		});
	}

	void Scene::ConvertToLocalSpace(Entity entity)
	{
		ZONG_PROFILE_FUNC();

		Entity parent = TryGetEntityWithUUID(entity.GetParentUUID());

		if (!parent)
			return;

		auto& transform = entity.Transform();
		glm::mat4 parentTransform = GetWorldSpaceTransformMatrix(parent);
		glm::mat4 localTransform = glm::inverse(parentTransform) * transform.GetTransform();
		transform.SetTransform(localTransform);
	}

	void Scene::ConvertToWorldSpace(Entity entity)
	{
		ZONG_PROFILE_FUNC();

		Entity parent = TryGetEntityWithUUID(entity.GetParentUUID());

		if (!parent)
			return;

		glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);
		auto& entityTransform = entity.Transform();
		entityTransform.SetTransform(transform);
	}

	glm::mat4 Scene::GetWorldSpaceTransformMatrix(Entity entity)
	{
		ZONG_PROFILE_FUNC();

		glm::mat4 transform(1.0f);

		Entity parent = TryGetEntityWithUUID(entity.GetParentUUID());
		if (parent)
			transform = GetWorldSpaceTransformMatrix(parent);

		return transform * entity.Transform().GetTransform();
	}

	// TODO: Definitely cache this at some point
	TransformComponent Scene::GetWorldSpaceTransform(Entity entity)
	{
		ZONG_PROFILE_FUNC();

		glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);
		TransformComponent transformComponent;
		transformComponent.SetTransform(transform);
		return transformComponent;
	}

	void Scene::ParentEntity(Entity entity, Entity parent)
	{
		ZONG_PROFILE_FUNC();

		if (parent.IsDescendantOf(entity))
		{
			UnparentEntity(parent);

			Entity newParent = TryGetEntityWithUUID(entity.GetParentUUID());
			if (newParent)
			{
				UnparentEntity(entity);
				ParentEntity(parent, newParent);
			}
		}
		else
		{
			Entity previousParent = TryGetEntityWithUUID(entity.GetParentUUID());

			if (previousParent)
				UnparentEntity(entity);
		}

		entity.SetParentUUID(parent.GetUUID());
		parent.Children().push_back(entity.GetUUID());

		ConvertToLocalSpace(entity);
	}

	void Scene::UnparentEntity(Entity entity, bool convertToWorldSpace)
	{
		ZONG_PROFILE_FUNC();

		Entity parent = TryGetEntityWithUUID(entity.GetParentUUID());
		if (!parent)
			return;

		auto& parentChildren = parent.Children();
		parentChildren.erase(std::remove(parentChildren.begin(), parentChildren.end(), entity.GetUUID()), parentChildren.end());

		if (convertToWorldSpace)
			ConvertToWorldSpace(entity);

		entity.SetParentUUID(0);
	}

	// Copy to runtime
	void Scene::CopyTo(Ref<Scene>& target)
	{
		ZONG_PROFILE_FUNC();

		// TODO(Yan): hack to prevent Box2D bodies from being created on copy via entt signals
		target->m_IsEditorScene = true;
		target->m_Name = m_Name;

		// Environment
		target->m_Light = m_Light;
		target->m_LightMultiplier = m_LightMultiplier;

		target->m_Environment = m_Environment;
		target->m_SkyboxLod = m_SkyboxLod;

		std::unordered_map<UUID, entt::entity> enttMap;
		auto idComponents = m_Registry.view<IDComponent>();
		for (auto entity : idComponents)
		{
			auto uuid = m_Registry.get<IDComponent>(entity).ID;
			auto name = m_Registry.get<TagComponent>(entity).Tag;
			Entity e = target->CreateEntityWithID(uuid, name);
			enttMap[uuid] = e.m_EntityHandle;
		}

		CopyComponent<PrefabComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<TagComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<TransformComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<RelationshipComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<MeshComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<StaticMeshComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<AnimationComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<DirectionalLightComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<PointLightComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<SpotLightComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<SkyLightComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<ScriptComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<CameraComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<SpriteRendererComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<TextComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<RigidBody2DComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<BoxCollider2DComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<CircleCollider2DComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<RigidBodyComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<CharacterControllerComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<FixedJointComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<CompoundColliderComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<BoxColliderComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<SphereColliderComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<CapsuleColliderComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<MeshColliderComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<AudioComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<AudioListenerComponent>(target->m_Registry, m_Registry, enttMap);

		target->SetPhysics2DGravity(GetPhysics2DGravity());

		// Sort IdComponent by by entity handle (which is essentially the order in which they were created)
		// This ensures a consistent ordering when iterating IdComponent (for example: when rendering scene hierarchy panel)
		target->SortEntities();

		target->m_ViewportWidth = m_ViewportWidth;
		target->m_ViewportHeight = m_ViewportHeight;

		target->m_IsEditorScene = false;
	}

	Ref<Scene> Scene::GetScene(UUID uuid)
	{
		if (s_ActiveScenes.find(uuid) != s_ActiveScenes.end())
			return s_ActiveScenes.at(uuid);

		return {};
	}

	float Scene::GetPhysics2DGravity() const
	{
		return m_Registry.get<Box2DWorldComponent>(m_SceneEntity).World->GetGravity().y;
	}

	void Scene::SetPhysics2DGravity(float gravity)
	{
		m_Registry.get<Box2DWorldComponent>(m_SceneEntity).World->SetGravity({ 0.0f, gravity });
	}

	Ref<PhysicsScene> Scene::GetPhysicsScene() const
	{
		if (!m_Registry.has<PhysicsSceneComponent>(m_SceneEntity))
			return nullptr;
		return m_Registry.get<PhysicsSceneComponent>(m_SceneEntity).PhysicsWorld;
	}

	void Scene::OnSceneTransition(AssetHandle scene)
	{
		if (m_OnSceneTransitionCallback)
			m_OnSceneTransitionCallback(scene);

		// Debug
		if (!m_OnSceneTransitionCallback)
		{
			ZONG_CORE_WARN("Cannot transition scene - no callback set!");
		}
	}

	// For copied AnimationComponent, need to create an independent AnimationGraph instance.
	void Scene::DuplicateAnimationInstance(Entity dst, Entity src)
	{
		ZONG_CORE_ASSERT(dst.HasComponent<AnimationComponent>(), "Destination entity does not have AnimationComponent!");
		{
			auto& animDst = dst.GetComponent<AnimationComponent>();
			if (animDst.AnimationGraph)
			{
				auto animGraphAsset = AssetManager::GetAsset<AnimationGraphAsset>(animDst.AnimationGraphHandle);
				if (animGraphAsset && animGraphAsset->IsValid())
				{
					animDst.AnimationGraph = animGraphAsset->CreateInstance();
				}
				auto& animSrc = src.GetComponent<AnimationComponent>();

				for (auto [id, srcValue] : animSrc.AnimationGraph->Ins)
				{
					try
					{
						auto dstValue = animDst.AnimationGraph->InValue(id);
						if (dstValue.isInt32())        dstValue.set(srcValue.getInt32());
						else if (dstValue.isInt64())   dstValue.set(srcValue.getInt64());
						else if (dstValue.isFloat32()) dstValue.set(srcValue.getFloat32());
						else if (dstValue.isFloat64()) dstValue.set(srcValue.getInt64());
					}
					catch (const std::out_of_range&)
					{
						;
					}
				}
			}
		}
	}

	Ref<Scene> Scene::CreateEmpty()
	{
		return Ref<Scene>::Create("Empty", false, false);
	}

	static void InsertMeshMaterials(Ref<MeshSource> meshSource, std::unordered_set<AssetHandle>& assetList)
	{
		// Mesh materials
		const auto& materials = meshSource->GetMaterials();
		for (auto material : materials)
		{
			Ref<Texture2D> albedoTexture = material->GetTexture2D("u_AlbedoTexture");
			if (albedoTexture && albedoTexture->Handle) // White texture has Handle == 0
				assetList.insert(albedoTexture->Handle);

			Ref<Texture2D> normalTexture = material->GetTexture2D("u_NormalTexture");
			if (normalTexture && albedoTexture->Handle)
				assetList.insert(normalTexture->Handle);

			Ref<Texture2D> metalnessTexture = material->GetTexture2D("u_MetalnessTexture");
			if (metalnessTexture && albedoTexture->Handle)
				assetList.insert(metalnessTexture->Handle);

			Ref<Texture2D> roughnessTexture = material->GetTexture2D("u_RoughnessTexture");
			if (roughnessTexture && albedoTexture->Handle)
				assetList.insert(roughnessTexture->Handle);
		}
	}

	std::unordered_set<AssetHandle> Scene::GetAssetList()
	{
		std::unordered_set<AssetHandle> assetList;
		std::unordered_set<AssetHandle> missingAssets; // TODO(Yan): debug only

		// MeshComponent
		{
			auto view = m_Registry.view<MeshComponent>();
			for (auto entity : view)
			{
				auto& mc = m_Registry.get<MeshComponent>(entity);
				if (mc.Mesh)
				{
					if (AssetManager::IsMemoryAsset(mc.Mesh))
						continue;

					if (AssetManager::IsAssetHandleValid(mc.Mesh))
					{
						assetList.insert(mc.Mesh);

						// MeshSource is required too
						Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(mc.Mesh);
						if (!mesh)
							continue;

						Ref<MeshSource> meshSource = mesh->GetMeshSource();
						if (meshSource && AssetManager::IsAssetHandleValid(meshSource->Handle))
						{
							assetList.insert(meshSource->Handle);
							InsertMeshMaterials(meshSource, assetList);
						}
					}
					else
					{
						missingAssets.insert(mc.Mesh);
					}
				}

				if (mc.MaterialTable)
				{
					auto& materialAssets = mc.MaterialTable->GetMaterials();
					for (auto& [index, materialAssetHandle] : materialAssets)
					{
						if (AssetManager::IsAssetHandleValid(materialAssetHandle))
						{
							assetList.insert(materialAssetHandle);

							Ref<MaterialAsset> materialAsset = AssetManager::GetAsset<MaterialAsset>(materialAssetHandle);

							std::array<Ref<Texture2D>, 4> textures = {
								materialAsset->GetAlbedoMap(),
								materialAsset->GetNormalMap(),
								materialAsset->GetMetalnessMap(),
								materialAsset->GetRoughnessMap()
							};

							// Textures
							for (auto texture : textures)
							{
								if (texture)
									assetList.insert(texture->Handle);
							}
						}
						else
						{
							missingAssets.insert(materialAssetHandle);
						}
					}
				}
			}
		}

		// StaticMeshComponent
		{
			auto view = m_Registry.view<StaticMeshComponent>();
			for (auto entity : view)
			{
				auto& mc = m_Registry.get<StaticMeshComponent>(entity);
				if (mc.StaticMesh)
				{
					if (AssetManager::IsMemoryAsset(mc.StaticMesh))
						continue;

					if (AssetManager::IsAssetHandleValid(mc.StaticMesh))
					{
						assetList.insert(mc.StaticMesh);

						// MeshSource is required too
						Ref<StaticMesh> mesh = AssetManager::GetAsset<StaticMesh>(mc.StaticMesh);
						Ref<MeshSource> meshSource = mesh->GetMeshSource();
						if (meshSource && AssetManager::IsAssetHandleValid(meshSource->Handle))
						{
							assetList.insert(meshSource->Handle);
							InsertMeshMaterials(meshSource, assetList);
						}
					}
					else
					{
						missingAssets.insert(mc.StaticMesh);
					}
				}

				if (mc.MaterialTable)
				{
					auto& materialAssets = mc.MaterialTable->GetMaterials();
					for (auto& [index, materialAssetHandle] : materialAssets)
					{
						if (AssetManager::IsAssetHandleValid(materialAssetHandle))
						{
							assetList.insert(materialAssetHandle);

							Ref<MaterialAsset> materialAsset = AssetManager::GetAsset<MaterialAsset>(materialAssetHandle);

							std::array<Ref<Texture2D>, 4> textures = {
								materialAsset->GetAlbedoMap(),
								materialAsset->GetNormalMap(),
								materialAsset->GetMetalnessMap(),
								materialAsset->GetRoughnessMap()
							};

							// Textures
							for (auto texture : textures)
							{
								if (texture)
									assetList.insert(texture->Handle);
							}
						}
						else
						{
							missingAssets.insert(materialAssetHandle);
						}
					}
				}
			}
		}

		// AnimationComponent
		{
			auto view = m_Registry.view<AnimationComponent>();
			for (auto entity : view)
			{
				const auto& ac = m_Registry.get<AnimationComponent>(entity);
				if (AssetManager::IsMemoryAsset(ac.AnimationGraphHandle))
					continue;

				if (AssetManager::IsAssetHandleValid(ac.AnimationGraphHandle) && ac.AnimationGraph)
				{
					assetList.insert(ac.AnimationGraphHandle);

					auto animationGraph = AssetManager::GetAsset<AnimationGraphAsset>(ac.AnimationGraphHandle);
					auto skeletonHandle = animationGraph->GetSkeletonHandle();
					if (auto skeletonAsset = AssetManager::GetAsset<SkeletonAsset>(skeletonHandle))
					{
						if (skeletonAsset->GetMeshSource() && skeletonAsset->GetMeshSource()->HasSkeleton())
						{
							assetList.insert(skeletonAsset->Handle);

							// MeshSource is required also
							Ref<MeshSource> meshSource = skeletonAsset->GetMeshSource();
							if (meshSource && AssetManager::IsAssetHandleValid(meshSource->Handle))
							{
								assetList.insert(meshSource->Handle);
								// Don't need materials for skeleton mesh source
								//InsertMeshMaterials(meshSource, assetList);
							}
						}
						else
						{
							missingAssets.insert(skeletonAsset->Handle);
						}
					}

					const auto& animationHandles = animationGraph->GetAnimationHandles();
					for (auto animationHandle : animationHandles)
					{
						if (auto animationAsset = AssetManager::GetAsset<AnimationAsset>(animationHandle); animationAsset->GetAnimation())
						{
							assetList.insert(animationAsset->Handle);

							// MeshSources are required also
							for (auto meshSource : { animationAsset->GetAnimationSource(), animationAsset->GetSkeletonSource() })
							{
								if (meshSource && AssetManager::IsAssetHandleValid(meshSource->Handle))
								{
									assetList.insert(meshSource->Handle);
									// Don't need materials for skeleton or animations
									//InsertMeshMaterials(source, assetList);
								}
							}
						}
						else
						{
							missingAssets.insert(animationAsset->Handle);
						}
					}
				}
				else
				{
					missingAssets.insert(ac.AnimationGraphHandle);
				}
			}
		}

		// ScriptComponent
		{
			auto view = m_Registry.view<ScriptComponent>();
			for (auto entity : view)
			{
				const auto& sc = m_Registry.get<ScriptComponent>(entity);
				if (sc.ScriptClassHandle && false) // These are referenced script classes (no need to load)
				{
					if (AssetManager::IsMemoryAsset(sc.ScriptClassHandle))
						continue;

					if (AssetManager::IsAssetHandleValid(sc.ScriptClassHandle))
					{
						assetList.insert(sc.ScriptClassHandle);
					}
					else
					{
						missingAssets.insert(sc.ScriptClassHandle);
					}
				}

				for (auto fieldID : sc.FieldIDs)
				{
					FieldInfo* fieldInfo = ScriptCache::GetFieldByID(fieldID);
					Ref<FieldStorageBase> storage = ScriptEngine::GetFieldStorage(Entity{ entity, this }, fieldID);
					if (FieldUtils::IsAsset(fieldInfo->Type))
					{
						if (!fieldInfo->IsArray())
						{
							Ref<FieldStorage> fieldStorage = storage.As<FieldStorage>();
							AssetHandle handle = fieldStorage->GetValue<UUID>();
							if (AssetManager::IsMemoryAsset(handle))
								continue;

							if (AssetManager::IsAssetHandleValid(handle))
							{
								assetList.insert(handle);
							}
							else
							{
								missingAssets.insert(handle);
							}
						}
						else
						{
							Ref<ArrayFieldStorage> arrayFieldStorage = storage.As<ArrayFieldStorage>();

							for (uint32_t i = 0; i < arrayFieldStorage->GetLength(); i++)
							{
								AssetHandle handle = arrayFieldStorage->GetValue<UUID>(i);

								if (AssetManager::IsMemoryAsset(handle))
									continue;

								if (AssetManager::IsAssetHandleValid(handle))
									assetList.insert(handle);
								else
									missingAssets.insert(handle);
							}
						}
					}
				}
			}
		}

		// SpriteRendererComponent
		{
			auto view = m_Registry.view<SpriteRendererComponent>();
			for (auto entity : view)
			{
				const auto& src = m_Registry.get<SpriteRendererComponent>(entity);
				if (src.Texture)
				{
					if (AssetManager::IsMemoryAsset(src.Texture))
						continue;

					if (AssetManager::IsAssetHandleValid(src.Texture))
					{
						assetList.insert(src.Texture);
					}
					else
					{
						missingAssets.insert(src.Texture);
					}
				}
			}
		}

		// TextComponent
		{
			auto view = m_Registry.view<TextComponent>();
			for (auto entity : view)
			{
				const auto& tc = m_Registry.get<TextComponent>(entity);
				if (tc.FontHandle)
				{
					if (AssetManager::IsMemoryAsset(tc.FontHandle))
						continue;

					if (AssetManager::IsAssetHandleValid(tc.FontHandle))
					{
						assetList.insert(tc.FontHandle);
					}
					else
					{
						missingAssets.insert(tc.FontHandle);
					}
				}
			}
		}

		// BoxColliderComponent
		{
			auto view = m_Registry.view<BoxColliderComponent>();
			for (auto entity : view)
			{
				const auto& bcc = m_Registry.get<BoxColliderComponent>(entity);
			}
		}

		// SphereColliderComponent
		{
			auto view = m_Registry.view<SphereColliderComponent>();
			for (auto entity : view)
			{
				const auto& scc = m_Registry.get<SphereColliderComponent>(entity);
			}
		}

		// CapsuleColliderComponent
		{
			auto view = m_Registry.view<CapsuleColliderComponent>();
			for (auto entity : view)
			{
				const auto& ccc = m_Registry.get<CapsuleColliderComponent>(entity);
			}
		}

		// MeshColliderComponent
		{
			auto view = m_Registry.view<MeshColliderComponent>();
			for (auto entity : view)
			{
				const auto& mcc = m_Registry.get<MeshColliderComponent>(entity);
				if (mcc.ColliderAsset)
				{
					if (AssetManager::IsMemoryAsset(mcc.ColliderAsset))
						continue;

					if (AssetManager::IsAssetHandleValid(mcc.ColliderAsset))
					{
						assetList.insert(mcc.ColliderAsset);
					}
					else
					{
						missingAssets.insert(mcc.ColliderAsset);
					}
				}
			}
		}

		// SkyLightComponent
		{
			auto view = m_Registry.view<SkyLightComponent>();
			for (auto entity : view)
			{
				const auto& slc = m_Registry.get<SkyLightComponent>(entity);
				if (slc.SceneEnvironment)
				{
					if (AssetManager::IsMemoryAsset(slc.SceneEnvironment))
						continue;

					if (AssetManager::IsAssetHandleValid(slc.SceneEnvironment))
					{
						assetList.insert(slc.SceneEnvironment);
					}
					else
					{
						missingAssets.insert(slc.SceneEnvironment);
					}
				}
			}
		}

		// Prefabs
		if (false)
		{
			SceneSerializer serializer(nullptr);
			const auto& sceneMetadata = Project::GetEditorAssetManager()->GetMetadata(Handle);
			serializer.DeserializeReferencedPrefabs(Project::GetAssetDirectory() / sceneMetadata.FilePath, assetList);
		}

		//ZONG_CORE_WARN("{} assets ({} missing)", assetList.size(), missingAssets.size());
		return assetList;
	}

}

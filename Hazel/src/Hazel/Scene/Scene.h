#pragma once

#include "Entity.h"

#include "Hazel/Core/TimeStep.h"
#include "Hazel/Core/UUID.h"

#include "Hazel/Editor/EditorCamera.h"

#include "Hazel/Renderer/Mesh.h"
#include "Hazel/Renderer/SceneEnvironment.h"

#include "Hazel/Script/ScriptEntityStorage.hpp"

namespace Hazel {

	namespace AnimationGraph {
		struct AnimationGraph;
	}

	class SceneRenderer;
	class Renderer2D;
	class Prefab;
	class PhysicsScene;

	struct DirLight
	{
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };

		float Intensity = 1.0f;
	};

	struct DirectionalLight
	{
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
		float Intensity = 0.0f;
		float ShadowAmount = 1.0f;
		// C++ only
		bool CastShadows = true;
	};

	struct PointLight
	{
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		float Intensity = 0.0f;
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
		float MinRadius = 0.001f;
		float Radius = 25.0f;
		float Falloff = 1.f;
		float SourceSize = 0.1f;
		bool CastsShadows = true;
		char Padding[3]{ 0, 0, 0 };
	};

	struct SpotLight
	{
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		float Intensity = 0.0f;
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		float AngleAttenuation = 0.0f;
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
		float Range = 0.1f;
		float Angle = 0.0f;
		float Falloff = 1.0f;
		bool SoftShadows = true;
		char Padding0[3]{ 0, 0, 0 };
		bool CastsShadows = true;
		char Padding1[3]{ 0, 0, 0 };
	};

	struct LightEnvironment
	{
		static constexpr size_t MaxDirectionalLights = 4;

		DirectionalLight DirectionalLights[MaxDirectionalLights];
		std::vector<PointLight> PointLights;
		std::vector<SpotLight> SpotLights;
		[[nodiscard]] uint32_t GetPointLightsSize() const { return (uint32_t)(PointLights.size() * sizeof (PointLight)); }
		[[nodiscard]] uint32_t GetSpotLightsSize() const { return (uint32_t)(SpotLights.size() * sizeof (SpotLight)); }
	};

	using EntityMap = std::unordered_map<UUID, Entity>;

	class PhysicsScene;

	struct SceneSpecification
	{
		std::string Name = "UntitledScene";
		bool IsEditorScene = false;
		bool Initalize = true;
	};

	class Scene : public Asset
	{
	public:
		struct PerformanceTimers
		{
			float ScriptUpdate = 0.0f;
			float ScriptLateUpdate = 0.0f;
			float PhysicsStep = 0.0f;
		};
	public:
		Scene(const std::string& name = "UntitledScene", bool isEditorScene = false, bool initalize = true);
		~Scene();
		
		void OnUpdateRuntime(Timestep ts);
		void OnUpdateEditor(Timestep ts);

		void OnRenderRuntime(Ref<SceneRenderer> renderer, Timestep ts);
		void OnRenderEditor(Ref<SceneRenderer> renderer, Timestep ts, const EditorCamera& editorCamera);
		void OnRenderSimulation(Ref<SceneRenderer> renderer, Timestep ts, const EditorCamera& editorCamera);

		void OnAssetReloaded(AssetHandle assetHandle);
		void OnAnimationGraphReloaded(AssetHandle animationGraphHandle);
		void OnMeshSourceReloaded(AssetHandle meshSourceHandle);
		void OnMeshReloaded(AssetHandle meshHandle);

		void RenderAnimationDebug(Ref<SceneRenderer> renderer, bool runtime);
		void RenderPhysicsDebug(Ref<SceneRenderer> renderer, bool runtime);
		void Render2DPhysicsDebug(Ref<SceneRenderer> renderer, Ref<Renderer2D> renderer2D, bool runtime);

		void OnEvent(Event& e);

		// Runtime
		void OnRuntimeStart();
		void OnRuntimeStop();

		void OnSimulationStart();
		void OnSimulationStop();

		void SetViewportSize(uint32_t width, uint32_t height);

		const Ref<Environment>& GetEnvironment() const { return m_Environment; }

		DirLight& GetLight() { return m_Light; }
		const DirLight& GetLight() const { return m_Light; }

		Entity GetMainCameraEntity();

		float& GetSkyboxLod() { return m_SkyboxLod; }
		float GetSkyboxLod() const { return m_SkyboxLod; }

		std::vector<UUID> GetAllChildren(Entity entity) const;

		Entity CreateEntity(const std::string& name = "");
		Entity CreateChildEntity(Entity parent, const std::string& name = "");
		Entity CreateEntityWithID(UUID uuid, const std::string& name = "", bool shouldSort = true);
		void SubmitToDestroyEntity(Entity entity);
		void DestroyEntity(Entity entity, bool excludeChildren = false, bool first = true);
		void DestroyEntity(UUID entityID, bool excludeChildren = false, bool first = true);

		void ResetTransformsToMesh(Entity entity, bool resetChildren);

		Entity DuplicateEntity(Entity entity);
		Entity CreatePrefabEntity(Entity entity, Entity parent, const glm::vec3* translation = nullptr, const glm::vec3* rotation = nullptr, const glm::vec3* scale = nullptr);
		
		Entity Instantiate(Ref<Prefab> prefab, const glm::vec3* translation = nullptr, const glm::vec3* rotation = nullptr, const glm::vec3* scale = nullptr);
		Entity InstantiateChild(Ref<Prefab> prefab, Entity parent, const glm::vec3* translation = nullptr, const glm::vec3* rotation = nullptr, const glm::vec3* scale = nullptr);
		Entity InstantiateMesh(Ref<Mesh> mesh);
		void RebuildMeshEntityHierarchy(Entity parent);
		Entity InstantiateStaticMesh(Ref<StaticMesh> mesh);

		std::vector<UUID> FindBoneEntityIds(Entity entity, Entity rootEntity, const Skeleton* skeleton);

		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

		// return entity with id as specified. entity is expected to exist (runtime error if it doesn't)
		Entity GetEntityWithUUID(UUID id) const;

		// return entity with id as specified, or empty entity if cannot be found - caller must check
		Entity TryGetEntityWithUUID(UUID id) const;

		// return entity with tag as specified, or empty entity if cannot be found - caller must check
		Entity TryGetEntityWithTag(const std::string& tag);

		// return descendant entity with tag as specified, or empty entity if cannot be found - caller must check
		// descendant could be immediate child, or deeper in the hierachy
		Entity TryGetDescendantEntityWithTag(Entity entity, const std::string& tag);

		void ConvertToLocalSpace(Entity entity);
		void ConvertToWorldSpace(Entity entity);
		glm::mat4 GetWorldSpaceTransformMatrix(Entity entity);
		TransformComponent GetWorldSpaceTransform(Entity entity);

		void ParentEntity(Entity entity, Entity parent);
		void UnparentEntity(Entity entity, bool convertToWorldSpace = true);

		void CopyTo(Ref<Scene>& target);

		UUID GetUUID() const { return m_SceneID; }

		static Ref<Scene> GetScene(UUID uuid);

		bool IsEditorScene() const { return m_IsEditorScene; }
		bool IsPlaying() const { return m_IsPlaying; }

		float GetPhysics2DGravity() const;
		void SetPhysics2DGravity(float gravity);

		Ref<PhysicsScene> GetPhysicsScene() const;

		void OnSceneTransition(AssetHandle scene);

		float GetTimeScale() const { return m_TimeScale; }
		void SetTimeScale(float timeScale) { m_TimeScale = timeScale; }

		static AssetType GetStaticType() { return AssetType::Scene; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		const std::string& GetName() const { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }

		void SetSceneTransitionCallback(const std::function<void(AssetHandle)>& callback) { m_OnSceneTransitionCallback = callback; }
		void SetEntityDestroyedCallback(const std::function<void(Entity)>& callback) { m_OnEntityDestroyedCallback = callback; }

		uint32_t GetViewportWidth() const { return m_ViewportWidth; }
		uint32_t GetViewportHeight() const { return m_ViewportHeight; }

		const EntityMap& GetEntityMap() const { return m_EntityIDMap; }
		std::unordered_set<AssetHandle> GetAssetList();

		const PerformanceTimers& GetPerformanceTimers() const { return m_PerformanceTimers; }

		template<typename TComponent>
		void CopyComponentIfExists(entt::entity dst, entt::registry& dstRegistry, entt::entity src)
		{
			if (m_Registry.has<TComponent>(src))
			{
				auto& srcComponent = m_Registry.get<TComponent>(src);
				dstRegistry.emplace_or_replace<TComponent>(dst, srcComponent);
			}
		}

		template<typename TComponent>
		static void CopyComponentFromScene(Entity dst, Ref<Scene> dstScene, Entity src, Ref<Scene> srcScene)
		{
			srcScene->CopyComponentIfExists<TComponent>((entt::entity)dst, dstScene->m_Registry, (entt::entity)src);
		}

		void DuplicateAnimationInstance(Entity dst, Entity src);

		ScriptStorage& GetScriptStorage() { return m_ScriptStorage; }
		const ScriptStorage& GetScriptStorage() const { return m_ScriptStorage; }

	public:
		static Ref<Scene> CreateEmpty();

	private:
		void OnAudioComponentConstruct(entt::registry& registry, entt::entity entity);
		void OnAudioComponentDestroy(entt::registry& registry, entt::entity entity);	
		void OnMeshColliderComponentConstruct(entt::registry& registry, entt::entity entity);
		void OnRigidBody2DComponentConstruct(entt::registry& registry, entt::entity entity);
		void OnRigidBody2DComponentDestroy(entt::registry& registry, entt::entity entity);
		void OnBoxCollider2DComponentConstruct(entt::registry& registry, entt::entity entity);
		void OnCircleCollider2DComponentConstruct(entt::registry& registry, entt::entity entity);
		void OnScriptComponentDestroy(entt::registry& registry, entt::entity entity);
		void OnRigidBodyComponentConstruct(entt::registry& registry, entt::entity entity);
		void OnRigidBodyComponentDestroy(entt::registry& registry, entt::entity entity);

		void BuildMeshEntityHierarchy(Entity parent, Ref<Mesh> mesh, const MeshNode& node);
		void BuildBoneEntityIds(Entity entity);
		void BuildMeshBoneEntityIds(Entity entity, Entity rootEntity);
		void BuildAnimationBoneEntityIds(Entity entity, Entity rootEntity);

		void SortEntities();

		template<typename Fn>
		void SubmitPostUpdateFunc(Fn&& func)
		{
			m_PostUpdateQueue.emplace_back(func);
		}

		std::vector<glm::mat4> GetModelSpaceBoneTransforms(const std::vector<UUID>& boneEntityIds, Ref<MeshSource> meshSource);
		void UpdateAnimation(Timestep ts, bool isRuntime, AnimationGraph::AnimationGraph::HandleOutgoingEventFn* animationEventHandler);

		bool RenderSkeletonDebug(Ref<SceneRenderer> renderer, Entity entity, Ref<StaticMesh> boneDebugMesh, Ref<MeshSource> boneDebugMeshSource, std::unordered_set<UUID> renderedEntities, UUID selectedEntityId);

	private:
		UUID m_SceneID;
		std::size_t m_LastSerializeHash = 0; // used by auto-save to determine if scene has changed
		entt::entity m_SceneEntity = entt::null;
		entt::registry m_Registry;

		std::function<void(AssetHandle)> m_OnSceneTransitionCallback;
		std::function<void(Entity)> m_OnEntityDestroyedCallback;

		std::string m_Name;
		bool m_IsEditorScene = false;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		EntityMap m_EntityIDMap;

		DirLight m_Light;
		float m_LightMultiplier = 0.3f;

		LightEnvironment m_LightEnvironment;

		Ref<Environment> m_Environment;
		float m_EnvironmentIntensity = 0.0f;

		std::vector<std::function<void()>> m_PostUpdateQueue;

		float m_SkyboxLod = 1.0f;
		bool m_IsPlaying = false;
		bool m_ShouldSimulate = false;

		float m_TimeScale = 1.0f;

		PerformanceTimers m_PerformanceTimers;

		ScriptStorage m_ScriptStorage;

		friend class Entity;
		friend class Prefab;
		friend class Physics2D;
		friend class SceneRenderer;
		friend class SceneSerializer;
		friend class PrefabSerializer;
		friend class SceneHierarchyPanel;
		friend class ECSDebugPanel;
	};

}

#include "EntityTemplates.h"

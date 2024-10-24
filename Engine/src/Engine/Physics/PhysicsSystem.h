#pragma once

#include "PhysicsSettings.h"
#include "MeshColliderCache.h"
#include "PhysicsAPI.h"

#include "Engine/Core/Events/SceneEvents.h"

namespace Engine {

	class PhysicsSystem
	{
	public:
		static void Init();
		static void Shutdown();

		static PhysicsSettings& GetSettings() { return s_PhysicsSettings; }
		static MeshColliderCache& GetMeshCache() { return s_PhysicsMeshCache; }
		static Ref<MeshCookingFactory> GetMeshCookingFactory() { return s_PhysicsAPI->GetMeshCookingFactory(); }
		static const std::string& GetLastErrorMessage();

		static Ref<MeshColliderAsset> GetOrCreateColliderAsset(Entity entity, MeshColliderComponent& component);

		static Ref<PhysicsScene> CreatePhysicsScene(const Ref<Scene>& scene);

		static PhysicsAPI* GetAPI() { return s_PhysicsAPI; }
	private:
		static void OnEvent(Event& event);

	private:
		static PhysicsAPI* s_PhysicsAPI;
		inline static PhysicsSettings s_PhysicsSettings;
		inline static MeshColliderCache s_PhysicsMeshCache;

	private:
		friend class JoltScene;
	};

}

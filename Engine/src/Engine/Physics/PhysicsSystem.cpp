#include "pch.h"
#include "PhysicsSystem.h"

#include "JoltPhysics/JoltAPI.h"

#include "Engine/Core/Application.h"
#include "Engine/Asset/AssetManager.h"

namespace Hazel {


	static PhysicsAPI* InitPhysicsAPI()
	{
		switch (PhysicsAPI::Current())
		{
			case PhysicsAPIType::Jolt: return hnew JoltAPI();
		}

		ZONG_CORE_VERIFY(false, "Unknown PhysicsAPI");
		return nullptr;
	}

	void PhysicsSystem::Init()
	{
		s_PhysicsAPI = InitPhysicsAPI();
		s_PhysicsAPI->Init();

		s_PhysicsMeshCache.Init();

		Application::Get().AddEventCallback(OnEvent);
	}

	void PhysicsSystem::Shutdown()
	{
		s_PhysicsMeshCache.Clear();
		s_PhysicsAPI->Shutdown();
		hdelete s_PhysicsAPI;
	}

	const std::string& PhysicsSystem::GetLastErrorMessage() { return s_PhysicsAPI->GetLastErrorMessage(); }

	Ref<MeshColliderAsset> PhysicsSystem::GetOrCreateColliderAsset(Entity entity, MeshColliderComponent& component)
	{
		Ref<MeshColliderAsset> colliderAsset = AssetManager::GetAsset<MeshColliderAsset>(component.ColliderAsset);

		if (colliderAsset)
			return colliderAsset;

		if (entity.HasComponent<MeshComponent>())
		{
			auto& mc = entity.GetComponent<MeshComponent>();
			component.ColliderAsset = AssetManager::CreateMemoryOnlyAsset<MeshColliderAsset>(mc.Mesh);
			component.SubmeshIndex = mc.SubmeshIndex;
		}
		else if (entity.HasComponent<StaticMeshComponent>())
		{
			component.ColliderAsset = AssetManager::CreateMemoryOnlyAsset<MeshColliderAsset>(entity.GetComponent<StaticMeshComponent>().StaticMesh);
		}

		colliderAsset = AssetManager::GetAsset<MeshColliderAsset>(component.ColliderAsset);

		if (colliderAsset && !PhysicsSystem::GetMeshCache().Exists(colliderAsset))
			s_PhysicsAPI->GetMeshCookingFactory()->CookMesh(component.ColliderAsset);

		return colliderAsset;
	}

	Ref<PhysicsScene> PhysicsSystem::CreatePhysicsScene(const Ref<Scene>& scene) { return s_PhysicsAPI->CreateScene(scene); }

	void PhysicsSystem::OnEvent(Event& event)
	{
		/*EventDispatcher dispatcher(event);

#ifdef ZONG_DEBUG
		dispatcher.Dispatch<ScenePreStartEvent>([](ScenePreStartEvent& e)
		{
			if (s_PhysicsSettings.DebugOnPlay && !PhysXDebugger::IsDebugging())
				PhysXDebugger::StartDebugging((Project::GetActive()->GetProjectDirectory() / "PhysXDebugInfo").string(), s_PhysicsSettings.DebugType == PhysicsDebugType::LiveDebug);
			return false;
		});

		dispatcher.Dispatch<ScenePreStopEvent>([](ScenePreStopEvent& e)
		{
			if (s_PhysicsSettings.DebugOnPlay)
				PhysXDebugger::StopDebugging();
			return false;
		});
#endif*/
	}

	PhysicsAPI* PhysicsSystem::s_PhysicsAPI = nullptr;

}

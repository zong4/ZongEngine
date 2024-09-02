#include "hzpch.h"
#include "ContactListener2D.h"
#include "Hazel/Scene/Scene.h"

namespace Hazel {

	void ContactListener2D::BeginContact(b2Contact* contact)
	{
		/*Ref<Scene> scene = ScriptEngine::GetSceneContext();

		if (!scene->IsPlaying())
			return;

		UUID aID = (UUID)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
		UUID bID = (UUID)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

		Entity a = scene->GetEntityWithUUID(aID);
		Entity b = scene->GetEntityWithUUID(bID);

		auto callOnCollision2DBegin = [](Entity entity, Entity other)
		{
			if (!entity.HasComponent<ScriptComponent>())
				return;

			const auto& sc = entity.GetComponent<ScriptComponent>();
			if (!ScriptEngine::IsModuleValid(sc.ScriptClassHandle) || !ScriptEngine::IsEntityInstantiated(entity))
				return;

			ScriptEngine::CallMethod(sc.ManagedInstance, "OnCollision2DBeginInternal", other.GetUUID());
		};

		callOnCollision2DBegin(a, b);
		callOnCollision2DBegin(b, a);*/
	}

	void ContactListener2D::EndContact(b2Contact* contact)
	{
		/*Ref<Scene> scene = ScriptEngine::GetSceneContext();

		if (!scene->IsPlaying())
			return;

		UUID aID = (UUID)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
		UUID bID = (UUID)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

		Entity a = scene->GetEntityWithUUID(aID);
		Entity b = scene->GetEntityWithUUID(bID);

		auto callOnCollision2DEnd = [](Entity entity, Entity other)
		{
			if (!entity.HasComponent<ScriptComponent>())
				return;

			const auto& sc = entity.GetComponent<ScriptComponent>();
			if (!ScriptEngine::IsModuleValid(sc.ScriptClassHandle) || !ScriptEngine::IsEntityInstantiated(entity))
				return;

			ScriptEngine::CallMethod(sc.ManagedInstance, "OnCollision2DEndInternal", other.GetUUID());
		};

		callOnCollision2DEnd(a, b);
		callOnCollision2DEnd(b, a);*/
	}

}


#include "hzpch.h"
#include "PhysicsScene.h"
#include "PhysicsSystem.h"
#include "Hazel/Asset/AssetManager.h"

#include "Hazel/ImGui/ImGui.h"
#include "Hazel/Debug/Profiler.h"

#include "Hazel/Script/ScriptEngine.h"

#include <glm/glm.hpp>

namespace Hazel {

	PhysicsScene::PhysicsScene(const Ref<Scene>& scene)
		: m_EntityScene(scene)
	{
		m_FixedTimeStep = PhysicsSystem::GetSettings().FixedTimestep;

		m_OverlapHitBuffer.reserve(s_OverlapHitBufferSize);
	}

	PhysicsScene::~PhysicsScene()
	{
		m_EntityScene = nullptr;
	}

	Ref<PhysicsBody> PhysicsScene::GetEntityBodyByID(UUID entityID) const
	{
		if (auto iter = m_RigidBodies.find(entityID); iter != m_RigidBodies.end())
			return iter->second;

		return nullptr;
	}

	Ref<CharacterController> PhysicsScene::GetCharacterController(UUID entityID) const
	{
		if (auto iter = m_CharacterControllers.find(entityID); iter != m_CharacterControllers.end())
			return iter->second;

		return nullptr;
	}

	void PhysicsScene::AddRadialImpulse(const glm::vec3& origin, float radius, float strength, EFalloffMode falloff, bool velocityChange)
	{
		SphereOverlapInfo sphereOverlapInfo;
		sphereOverlapInfo.Origin = origin;
		sphereOverlapInfo.Radius = radius;

		SceneQueryHit* hitActors;
		int32_t hitCount = OverlapShape(&sphereOverlapInfo, &hitActors);

		if (hitCount == 0)
			return;

		for (int32_t i = 0; i < hitCount; i++)
		{
			auto entityBody = GetEntityBodyByID(hitActors[i].HitEntity);

			if (!entityBody || !entityBody->IsDynamic())
				continue;

			entityBody->AddRadialImpulse(origin, radius, strength, falloff, velocityChange);
		}
	}

	void PhysicsScene::OnContactEvent(ContactType type, UUID entityA, UUID entityB)
	{
		size_t index = m_NextContactEventIndex.load();

		if (index == MaxContactEvents)
		{
			m_NextContactEventIndex = 0;
			index = 0;
		}

		auto& contactEvent = m_ContactEvents[index];
		contactEvent.Type = type;
		contactEvent.EntityA = entityA;
		contactEvent.EntityB = entityB;

		m_NextContactEventIndex++;
	}

	static void CallScriptContactCallback(const char* methodName, Entity entityA, Entity entityB)
	{
		if (!entityA.HasComponent<ScriptComponent>())
			return;

		auto& sc = entityA.GetComponent<ScriptComponent>();

		const auto& scriptEngine = ScriptEngine::GetInstance();

		if (!scriptEngine.IsValidScript(sc.ScriptID) || !sc.Instance.IsValid())
			return;

		sc.Instance.Invoke(methodName, uint64_t(entityB.GetUUID()));
	}

	void PhysicsScene::SubStepStrategy(float ts)
	{
		HZ_PROFILE_SCOPE_DYNAMIC("PhysicsSystem::SubStepStrategy");

		if (m_Accumulator > m_FixedTimeStep)
			m_Accumulator = 0.0f;

		m_Accumulator += ts;
		if (m_Accumulator < m_FixedTimeStep)
		{
			m_CollisionSteps = 0;
			return;
		}

		m_CollisionSteps = (uint32_t)(m_Accumulator / m_FixedTimeStep);
		m_Accumulator -= (float)m_CollisionSteps * m_FixedTimeStep;
	}

	void PhysicsScene::PreSimulate(float ts)
	{
		HZ_PROFILE_FUNC("PhysicsScene::PreSimulate");

		SubStepStrategy(ts);

		auto view = m_EntityScene->GetAllEntitiesWith<RigidBodyComponent, ScriptComponent>();

		const auto& scriptEngine = ScriptEngine::GetInstance();

		for (auto enttID : view)
		{
			auto& scriptComponent = view.get<ScriptComponent>(enttID);

			if (!scriptComponent.Instance.IsValid() || !scriptEngine.IsValidScript(scriptComponent.ScriptID))
				continue;

			scriptComponent.Instance.Invoke("OnPhysicsUpdate", 0.0f);
		}

		for (auto& [entityID, characterController] : m_CharacterControllers)
			characterController->PreSimulate(ts);

		// Synchronize transforms for (static) bodies that have been moved by gameplay
		SynchronizePendingBodyTransforms();

		// For each kinematic body, call MoveKinematic() so that the kinematic body will end up at the Hazel Entity's position and rotation.
		// Note that kinematic bodies will always move the requisite amount.  They are _not_ stopped by collisions (even with immovable objects).
		// They will, however, push other (dynamic) bodies out of the way.
		for (auto& [entityID, body] : m_RigidBodies)
		{
			if (body->IsKinematic())
			{
				Entity entity = m_EntityScene->GetEntityWithUUID(entityID);
				auto tc = m_EntityScene->GetWorldSpaceTransform(entity);

				// moving physics bodies is expensive.  make sure its worth it.
				glm::vec3 currentBodyTranslation = body->GetTranslation();
				glm::quat currentBodyRotation = body->GetRotation();
				if (glm::any(glm::epsilonNotEqual(currentBodyTranslation, tc.Translation, 0.00001f)) || glm::any(glm::epsilonNotEqual(currentBodyRotation, tc.GetRotation(), 0.00001f)))
				{
					// Note (0x): Jolt does not awaken sleeping kinematic bodies when they are moved.  Wake them up (otherwise the body will not move).
					if (body->IsSleeping())
					{
						body->SetSleepState(false);
					}

					glm::vec3 targetTranslation = tc.Translation;
					glm::quat targetRotation = tc.GetRotation();
					if (glm::dot(currentBodyRotation, targetRotation) < 0.0f)
					{
						targetRotation = -targetRotation;
					}

					if (glm::any(glm::epsilonNotEqual(currentBodyRotation, targetRotation, 0.000001f)))
					{
						glm::vec3 currentBodyEuler = glm::eulerAngles(currentBodyRotation);
						glm::vec3 targetBodyEuler = glm::eulerAngles(tc.GetRotation());

						glm::quat rotation = tc.GetRotation() * glm::conjugate(currentBodyRotation);
						glm::vec3 rotationEuler = glm::eulerAngles(rotation);
					}

					body->MoveKinematic(tc.Translation, targetRotation, ts);
				}
			}
		}
	}

	void PhysicsScene::SynchronizePendingBodyTransforms()
	{
		// Synchronize bodies that requested it explicitly
		for (auto body : m_BodiesScheduledForSync)
		{
			if (!body.IsValid())
				continue;

			SynchronizeBodyTransform(body);
		}

		m_BodiesScheduledForSync.clear();
	}

	void PhysicsScene::PostSimulate()
	{
		HZ_PROFILE_FUNC("PhysicsScene::PostSimulate");

		for (auto& [entityID, characterController] : m_CharacterControllers)
			characterController->PostSimulate();

		PhysicsSystem::GetAPI()->GetCaptureManager()->CaptureFrame();

		{
			size_t eventCount = m_NextContactEventIndex.load();
			for (size_t i = 0; i < eventCount; i++)
			{
				const auto& contactEvent = m_ContactEvents[i];
				Entity entityA = m_EntityScene->TryGetEntityWithUUID(contactEvent.EntityA);
				Entity entityB = m_EntityScene->TryGetEntityWithUUID(contactEvent.EntityB);

				if (!entityA || !entityB)
					continue;

				switch (contactEvent.Type)
				{
					case ContactType::CollisionBegin:
					{
						CallScriptContactCallback("OnCollisionBeginInternal", entityA, entityB);
						CallScriptContactCallback("OnCollisionBeginInternal", entityB, entityA);
						break;
					}
					case ContactType::CollisionEnd:
					{
						CallScriptContactCallback("OnCollisionEndInternal", entityA, entityB);
						CallScriptContactCallback("OnCollisionEndInternal", entityB, entityA);
						break;
					}
					case ContactType::TriggerBegin:
					{
						CallScriptContactCallback("OnTriggerBeginInternal", entityA, entityB);
						CallScriptContactCallback("OnTriggerBeginInternal", entityB, entityA);
						break;
					}
					case ContactType::TriggerEnd:
					{
						CallScriptContactCallback("OnTriggerEndInternal", entityA, entityB);
						CallScriptContactCallback("OnTriggerEndInternal", entityB, entityA);
						break;
					}
				}

				m_NextContactEventIndex--;
			}
		}
	}

	void PhysicsScene::CreateRigidBodies()
	{
		std::vector<UUID> compoundedEntities;

		// Create CompoundColliderComponent for entities that have a MeshColliderComponent with a CollisionComplexity of Default
		// We do this because we have two mesh shapes generated, one convex and one triangle, the convex shape is used for simulation
		// and the triangle shape is used for scene queries (e.g raycasts)
		{
			auto view = m_EntityScene->GetAllEntitiesWith<MeshColliderComponent>();
			for (auto enttID : view)
			{
				Entity entity = { enttID, m_EntityScene.Raw() };

				if (entity.HasComponent<CompoundColliderComponent>())
					continue;

				const auto& meshColliderComponent = view.get<MeshColliderComponent>(enttID);
				if (meshColliderComponent.CollisionComplexity == ECollisionComplexity::Default)
				{
					auto& compoundColliderComponent = entity.AddComponent<CompoundColliderComponent>();
					compoundColliderComponent.IncludeStaticChildColliders = false;
					compoundColliderComponent.IsImmutable = true;
					compoundColliderComponent.CompoundedColliderEntities.push_back(entity.GetUUID());
				}
			}
		}

		{
			auto view = m_EntityScene->GetAllEntitiesWith<CompoundColliderComponent>();
			for (auto enttID : view)
			{
				Entity entity = { enttID, m_EntityScene.Raw() };
				const auto& compoundColliderComponent = entity.GetComponent<CompoundColliderComponent>();
				for (auto entityID : compoundColliderComponent.CompoundedColliderEntities)
					compoundedEntities.push_back(entityID);
			}
		}

		// Add RigidBodyComponent's for entities that have don't have a RigidBodyComponent but have colliders
		{
			auto view = m_EntityScene->GetAllEntitiesWith<TransformComponent>();
			for (auto enttID : view)
			{
				Entity entity = { enttID, m_EntityScene.Raw() };

				if (entity.HasComponent<RigidBodyComponent>())
					continue;

				if (entity.HasComponent<CharacterControllerComponent>())
					continue;

				if (!entity.HasAny<BoxColliderComponent, SphereColliderComponent, CapsuleColliderComponent, MeshColliderComponent>())
					continue;

				auto found = std::find_if(compoundedEntities.begin(), compoundedEntities.end(), [&entity](const auto& entityID)
				{
					return entityID == entity.GetUUID();
				});

				if (found != compoundedEntities.end() && !entity.HasComponent<CompoundColliderComponent>())
					continue;

				auto& rigidBodyComponent = entity.AddComponent<RigidBodyComponent>();
				rigidBodyComponent.BodyType = EBodyType::Static;
			}
		}

		// Create RigidBodies for entities that have an explicit RigidBodyComponent
		{
			auto view = m_EntityScene->GetAllEntitiesWith<RigidBodyComponent>();

			for (auto enttID : view)
			{
				Entity entity = { enttID, m_EntityScene.Raw() };

				auto found = std::find_if(compoundedEntities.begin(), compoundedEntities.end(), [&entity](const auto& entityID)
				{
					return entityID == entity.GetUUID();
				});

				if (found != compoundedEntities.end() && !entity.HasComponent<CompoundColliderComponent>())
					continue;

				CreateBody(entity, BodyAddType::AddImmediate);
			}
		}
	}

	void PhysicsScene::CreateCharacterControllers()
	{
		auto view = m_EntityScene->GetAllEntitiesWith<CharacterControllerComponent>();

		for (auto enttID : view)
		{
			Entity entity = { enttID, m_EntityScene.Raw() };
			CreateCharacterController(entity);
		}
	}

}

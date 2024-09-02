#pragma once

#include "PhysicsSettings.h"
#include "PhysicsBody.h"
#include "CharacterController.h"
#include "PhysicsContactCallback.h"
#include "SceneQueries.h"

// NOTE(Peter): Allocates roughly 800 bytes once, and then reuses
#define OVERLAP_MAX_COLLIDERS 50

#include <mutex>
#include <deque>

namespace Hazel {

	//class PhysicsRaycastExcludeEntityFilter;

	enum class BodyAddType { AddBulk, AddImmediate };

	static constexpr size_t s_OverlapHitBufferSize = 50;

	class PhysicsScene : public RefCounted
	{
	public:
		PhysicsScene(const Ref<Scene>& scene);
		virtual ~PhysicsScene();

		virtual void Destroy() = 0;

		virtual void Simulate(float ts) = 0;

		virtual glm::vec3 GetGravity() const = 0;
		virtual void SetGravity(const glm::vec3& gravity) = 0;

		Ref<PhysicsBody> GetEntityBodyByID(UUID entityID) const;
		Ref<PhysicsBody> GetEntityBody(Entity entity) const { return GetEntityBodyByID(entity.GetUUID()); }
		virtual Ref<PhysicsBody> CreateBody(Entity entity, BodyAddType addType = BodyAddType::AddImmediate) = 0;
		virtual void DestroyBody(Entity entity) = 0;

		virtual void SetBodyType(Entity entity, EBodyType bodyType) = 0;

		Ref<CharacterController> GetCharacterController(UUID entityID) const;
		Ref<CharacterController> GetCharacterController(Entity entity) const { return GetCharacterController(entity.GetUUID()); }
		virtual Ref<CharacterController> CreateCharacterController(Entity entity) = 0;
		virtual void DestroyCharacterController(Entity entity) = 0;

		const Ref<Scene>& GetEntityScene() const { return m_EntityScene; }
		Ref<Scene> GetEntityScene() { return m_EntityScene; }

		virtual void OnEvent(Event& event) {}
		virtual void OnScenePostStart() {}

		//// Geometry Queries ////
		virtual bool CastRay(const RayCastInfo* rayCastInfo, SceneQueryHit& outHit) = 0;
		virtual bool CastShape(const ShapeCastInfo* shapeCastInfo, SceneQueryHit& outHit) = 0;
		virtual int32_t OverlapShape(const ShapeOverlapInfo* shapeOverlapInfo, SceneQueryHit** outHit) = 0;

		//// Radial Impulse ////
		void AddRadialImpulse(const glm::vec3& origin, float radius, float strength, EFalloffMode falloff, bool velocityChange);

		virtual void Teleport(Entity entity, const glm::vec3& targetPosition, const glm::quat& targetRotation, bool force = false) = 0;

		void MarkForSynchronization(Ref<PhysicsBody> body)
		{
			m_BodiesScheduledForSync.push_back(body);
		}

	protected:
		void PreSimulate(float ts);
		void PostSimulate();

		void CreateRigidBodies();
		void CreateCharacterControllers();
		void SynchronizePendingBodyTransforms();

		void OnContactEvent(ContactType type, UUID entityA, UUID entityB);

		virtual void SynchronizeBodyTransform(WeakRef<PhysicsBody> body) = 0;

	private:
		void SubStepStrategy(float ts);

	protected:
		Ref<Scene> m_EntityScene;
		std::unordered_map<UUID, Ref<PhysicsBody>> m_RigidBodies;
		std::unordered_map<UUID, Ref<CharacterController>> m_CharacterControllers;

		std::vector<WeakRef<PhysicsBody>> m_BodiesScheduledForSync;

		std::vector<SceneQueryHit> m_OverlapHitBuffer;

		float m_FixedTimeStep = 1.0f / 60.0f;
		float m_Accumulator = 0.0f;
		uint32_t m_CollisionSteps = 1;

	private:
		static constexpr size_t MaxContactEvents = 10000;
		struct ContactEvent { ContactType Type = ContactType::None; UUID EntityA; UUID EntityB; };

		std::mutex m_ContactEventsMutex;
		ContactEvent m_ContactEvents[MaxContactEvents];
		std::atomic<size_t> m_NextContactEventIndex = 0;

	private:
		friend class Scene;
	};

}

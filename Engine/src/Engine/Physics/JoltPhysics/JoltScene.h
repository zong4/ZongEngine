#pragma once

#include "Engine/Physics/PhysicsScene.h"

#include "JoltUtils.h"
#include "JoltLayerInterface.h"
#include "JoltContactListener.h"
#include "JoltBody.h"

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyFilter.h>

namespace Engine {

	class HZBroadPhaseLayerInterface;

	class JoltScene : public PhysicsScene
	{
	public:
		JoltScene(const Ref<Scene>& scene);
		~JoltScene();

		virtual void Destroy() override;

		void Simulate(float ts) override;

		glm::vec3 GetGravity() const override { return JoltUtils::FromJoltVector(m_JoltSystem->GetGravity()); }
		void SetGravity(const glm::vec3& gravity) override { m_JoltSystem->SetGravity(JoltUtils::ToJoltVector(gravity)); }

		virtual Ref<PhysicsBody> CreateBody(Entity entity, BodyAddType addType = BodyAddType::AddImmediate) override;
		virtual void DestroyBody(Entity entity) override;

		virtual void SetBodyType(Entity entity, EBodyType bodyType) override;

		virtual void OnScenePostStart() override;

		virtual bool CastRay(const RayCastInfo* rayCastInfo, SceneQueryHit& outHit) override;
		virtual bool CastShape(const ShapeCastInfo* shapeCastInfo, SceneQueryHit& outHit) override;
		virtual int32_t OverlapShape(const ShapeOverlapInfo* shapeOverlapInfo, SceneQueryHit** outHits) override;

		virtual void Teleport(Entity entity, const glm::vec3& targetPosition, const glm::quat& targetRotation, bool force = false) override;

		void MarkKinematic(Ref<JoltBody> body);
		void UnmarkKinematic(Ref<JoltBody> body);

		Ref<CharacterController> CreateCharacterController(Entity entity) override;
		void DestroyCharacterController(Entity entity) override;

	protected:
		void SynchronizeBodyTransform(WeakRef<PhysicsBody> body) override;

	public:
		static JPH::BodyInterface& GetBodyInterface(bool inShouldLock = true);
		static const JPH::BodyLockInterface& GetBodyLockInterface(bool inShouldLock = true);
		static JPH::PhysicsSystem& GetJoltSystem();

	private:
		void ProcessBulkStorage();

	private:
		std::unique_ptr<JPH::PhysicsSystem> m_JoltSystem;
		std::unique_ptr<JoltLayerInterface> m_JoltLayerInterface;
		std::unique_ptr<JoltContactListener> m_JoltContactListener;

		JPH::BodyID* m_BulkAddBuffer = nullptr;
		size_t m_BulkBufferCount = 0;
		size_t m_BodiesAddedSinceOptimization = 0;

		JPH::BodyID* m_OverlapIDBuffer = nullptr;
		size_t m_OverlapIDBufferCount = 20;

		std::vector<WeakRef<JoltBody>> m_KinematicBodies;

		inline static JPH::Plane s_AirPlane = JPH::Plane::sFromPointAndNormal(JPH::Vec3(0.0f, 100000.0f, 0.0f), JPH::Vec3(0.0f, 1.0f, 0.0f));

	private:
		static constexpr size_t s_BroadPhaseOptimizationThreashold = 500;
		static constexpr size_t s_MaxBulkBodyIDBufferSize = 2500;

	};

	struct JoltRayCastBodyFilter : public JPH::BodyFilter
	{
		JoltRayCastBodyFilter(JoltScene* scene, const ExcludedEntityMap& excludedEntities);
		virtual ~JoltRayCastBodyFilter() = default;

		/// Filter function. Returns true if we should collide with inBodyID
		virtual bool ShouldCollide(const JPH::BodyID& inBodyID) const;

		/// Filter function. Returns true if we should collide with inBody (this is called after the body is locked and makes it possible to filter based on body members)
		virtual bool ShouldCollideLocked(const JPH::Body& inBody) const;

	private:
		std::unordered_set<JPH::BodyID> m_ExcludedBodies;
	};

}

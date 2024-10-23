#include "pch.h"
#include "JoltScene.h"
#include "JoltAPI.h"
#include "JoltUtils.h"
#include "JoltShapes.h"
#include "JoltCaptureManager.h"
#include "JoltCharacterController.h"
#include "JoltMaterial.h"

#include "Engine/Physics/PhysicsSystem.h"
#include "Engine/Physics/PhysicsLayer.h"

#include "Engine/Core/Application.h"
#include "Engine/Core/Timer.h"
#include "Engine/Debug/Profiler.h"

#include "Engine/Core/Events/SceneEvents.h"
#include "Engine/Core/Input.h"

#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Core/Profiler.h>

namespace Hazel {

	static JoltScene* s_CurrentInstance = nullptr;

	class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
		{
			const auto& layerData1 = PhysicsLayerManager::GetLayer(inLayer1);
			const auto& layerData2 = PhysicsLayerManager::GetLayer((uint8_t)inLayer2);

			if (layerData1.LayerID == layerData2.LayerID)
				return layerData1.CollidesWithSelf;

			return layerData1.CollidesWith & layerData2.BitValue;
		}
	};

	class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
	{
	public:
		bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
		{
			const auto& layerData1 = PhysicsLayerManager::GetLayer(inObject1);
			const auto& layerData2 = PhysicsLayerManager::GetLayer(inObject2);

			if (layerData1.LayerID == layerData2.LayerID)
				return layerData1.CollidesWithSelf;

			return layerData1.CollidesWith & layerData2.BitValue;
		}
	};

	static ObjectLayerPairFilterImpl s_ObjectVsObjectLayerFilter;
	static ObjectVsBroadPhaseLayerFilterImpl s_ObjectVsBroadPhaseLayerFilter;

	JoltScene::JoltScene(const Ref<Scene>& scene)
		: PhysicsScene(scene)
	{
		ZONG_CORE_VERIFY(s_CurrentInstance == nullptr, "Shouldn't have multiple instances of a physics scene!");
		s_CurrentInstance = this;

		const auto& settings = PhysicsSystem::GetSettings();

		m_JoltSystem = std::make_unique<JPH::PhysicsSystem>();
		m_JoltLayerInterface = std::make_unique<JoltLayerInterface>();

		m_JoltSystem->Init(20240, 0, 65536, 20240, *m_JoltLayerInterface, s_ObjectVsBroadPhaseLayerFilter, s_ObjectVsObjectLayerFilter);
		m_JoltSystem->SetGravity(JoltUtils::ToJoltVector(settings.Gravity));

		JPH::PhysicsSettings joltSettings;
		joltSettings.mNumPositionSteps = settings.PositionSolverIterations;
		joltSettings.mNumVelocitySteps = settings.VelocitySolverIterations;
		m_JoltSystem->SetPhysicsSettings(joltSettings);

		m_JoltContactListener = std::make_unique<JoltContactListener>(&m_JoltSystem->GetBodyLockInterfaceNoLock(), [this](ContactType contactType, Entity entityA, Entity entityB)
		{
			OnContactEvent(contactType, entityA, entityB);
		});

		m_BulkAddBuffer = new JPH::BodyID[s_MaxBulkBodyIDBufferSize];
		m_BulkBufferCount = 0;

		m_OverlapIDBuffer = new JPH::BodyID[m_OverlapIDBufferCount];

		m_JoltSystem->SetContactListener(m_JoltContactListener.get());
		CreateRigidBodies();
		CreateCharacterControllers();
	}

	JoltScene::~JoltScene()
	{
		Destroy();
	}

	void JoltScene::Destroy()
	{
		if (m_BulkAddBuffer == nullptr)
			return;

		// NOTE(Peter): We don't technically have to this, but explicitly cleaning things up is more consistent, and means we explicitly control
		//				the order that things are destroyed in
		m_BulkBufferCount = 0;
		delete[] m_BulkAddBuffer;
		m_BulkAddBuffer = nullptr;

		m_RigidBodies.clear();
		m_JoltContactListener.reset();
		m_JoltLayerInterface.reset();
		m_JoltSystem.reset();

		s_CurrentInstance = nullptr;
	}

	void JoltScene::Simulate(float ts)
	{
		ZONG_PROFILE_FUNC("JoltScene::Simulate");

		ProcessBulkStorage();

		PreSimulate(ts);

		JoltAPI* api = (JoltAPI*)PhysicsSystem::GetAPI();

		if (m_CollisionSteps > 0)
		{
			ZONG_PROFILE_SCOPE_DYNAMIC("JoltSystem::Update");
			m_JoltSystem->Update(m_FixedTimeStep, m_CollisionSteps, 1, api->GetTempAllocator(), api->GetJobThreadPool());
		}
		
		for (auto& [entityID, characterController] : m_CharacterControllers)
			characterController.As<JoltCharacterController>()->Simulate(ts);

		{
			ZONG_PROFILE_SCOPE_DYNAMIC("JoltScene::SynchronizeTransform");
			const auto& bodyLockInterface = m_JoltSystem->GetBodyLockInterface();
			JPH::BodyIDVector activeBodies;
			m_JoltSystem->GetActiveBodies(activeBodies);
			JPH::BodyLockMultiWrite activeBodiesLock(bodyLockInterface, activeBodies.data(), static_cast<int32_t>(activeBodies.size()));
			for (int32_t i = 0; i < activeBodies.size(); i++)
			{
				JPH::Body* body = activeBodiesLock.GetBody(i);

				if (body == nullptr)
					continue;

				// Apply air resistance
				//body->ApplyBuoyancyImpulse(s_AirPlane, 0.075f, 0.15f, 0.01f, JPH::Vec3::sZero(), m_JoltSystem->GetGravity(), m_FixedTimeStep);

				// Synchronize the transform
				Entity entity = m_EntityScene->TryGetEntityWithUUID(body->GetUserData());

				if (!entity)
					continue;

				auto rigidBody = m_RigidBodies.at(entity.GetUUID());

				auto& transformComponent = entity.GetComponent<TransformComponent>();
				glm::vec3 scale = transformComponent.Scale;
				transformComponent.Translation = JoltUtils::FromJoltVector(body->GetPosition());

				if (!rigidBody->IsAllRotationLocked())
					transformComponent.SetRotation(JoltUtils::FromJoltQuat(body->GetRotation()));

				m_EntityScene->ConvertToLocalSpace(entity);
				transformComponent.Scale = scale;
			}

			for (auto& [entityID, characterController] : m_CharacterControllers)
			{
				auto joltCharacterController = characterController.As<JoltCharacterController>();
				Entity entity = m_EntityScene->GetEntityWithUUID(entityID);
				auto& transformComponent = entity.GetComponent<TransformComponent>();
				glm::vec3 scale = transformComponent.Scale;
				transformComponent.Translation = JoltUtils::FromJoltVector(joltCharacterController->m_Controller->GetPosition());
				transformComponent.SetRotation(JoltUtils::FromJoltQuat(joltCharacterController->m_Controller->GetRotation()));

				m_EntityScene->ConvertToLocalSpace(entity);
				transformComponent.Scale = scale;
			}
		}

		PostSimulate();
	}

	Ref<PhysicsBody> JoltScene::CreateBody(Entity entity, BodyAddType addType)
	{
		if (!entity.HasAny<CompoundColliderComponent, BoxColliderComponent, SphereColliderComponent, CapsuleColliderComponent, MeshColliderComponent>())
			return nullptr;

		if (auto existingBody = GetEntityBody(entity))
			return existingBody;

		JPH::BodyInterface& bodyInterface = m_JoltSystem->GetBodyInterface();
		Ref<JoltBody> rigidBody = Ref<JoltBody>::Create(bodyInterface, entity);

		if (rigidBody->m_BodyID.IsInvalid())
			return nullptr;

		switch (addType)
		{
			case BodyAddType::AddBulk:
				/*{
					if (m_BulkBufferCount == s_MaxBulkBodyIDBufferSize)
					{
						ProcessBulkStorage();
						m_BulkBufferCount = 0;
					}

					m_BulkAddBuffer[m_BulkBufferCount++] = rigidBody->GetBodyID();
					break;
				}*/
			case BodyAddType::AddImmediate:
			{
				//ZONG_CORE_INFO_TAG("Physics", "Adding body immediately.");
				bodyInterface.AddBody(rigidBody->m_BodyID, JPH::EActivation::Activate);
				break;
			}
		}

		m_RigidBodies[entity.GetUUID()] = rigidBody;
		return rigidBody;
	}

	void JoltScene::DestroyBody(Entity entity)
	{
		auto it = m_RigidBodies.find(entity.GetUUID());

		if (it == m_RigidBodies.end())
			return;

		it->second.As<JoltBody>()->Release();
		m_RigidBodies.erase(it);
	}

	void JoltScene::SetBodyType(Entity entity, EBodyType bodyType)
	{
		auto entityBody = GetEntityBody(entity);
		ZONG_CORE_VERIFY(entityBody);

		JPH::BodyLockWrite bodyLock(m_JoltSystem->GetBodyLockInterface(), entityBody.As<JoltBody>()->m_BodyID);
		ZONG_CORE_VERIFY(bodyLock.Succeeded());
		JPH::Body& body = bodyLock.GetBody();

		if (bodyType == EBodyType::Static && body.IsActive())
			m_JoltSystem->GetBodyInterfaceNoLock().DeactivateBody(body.GetID());

		body.SetMotionType(JoltUtils::ToJoltMotionType(bodyType));
	}

	void JoltScene::OnScenePostStart()
	{
		ProcessBulkStorage();
	}

	bool JoltScene::CastRay(const RayCastInfo* rayCastInfo, SceneQueryHit& outHit)
	{
		outHit.Clear();

		JPH::RayCast ray;
		ray.mOrigin = JoltUtils::ToJoltVector(rayCastInfo->Origin);
		ray.mDirection = JoltUtils::ToJoltVector(glm::normalize(rayCastInfo->Direction)) * rayCastInfo->MaxDistance;

		JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> hitCollector;
		JPH::RayCastSettings rayCastSettings;
		m_JoltSystem->GetNarrowPhaseQuery().CastRay(JPH::RRayCast(ray), rayCastSettings, hitCollector, {}, {}, JoltRayCastBodyFilter(this, rayCastInfo->ExcludedEntities));

		for (const auto& [entityID, characterController] : m_CharacterControllers)
		{
			auto joltCharacterController = characterController.As<JoltCharacterController>();
			auto centerOfMassTransform = joltCharacterController->m_Controller->GetCenterOfMassTransform();
			JPH::BodyID bodyID((uint64_t(entityID) >> 32) & 0xFFFFFFFF);
			JPH::TransformedShape transformedShape(centerOfMassTransform.GetTranslation(), centerOfMassTransform.GetRotation().GetQuaternion(), joltCharacterController->m_Controller->GetShape(), bodyID);
			transformedShape.CastRay(JPH::RRayCast(ray), rayCastSettings, hitCollector);
		}

		if (!hitCollector.HadHit())
			return false;

		// Check if we hit any of the Character Controllers
		for (const auto& [entityID, characterController] : m_CharacterControllers)
		{
			JPH::BodyID bodyID((uint64_t(entityID) >> 32) & 0xFFFFFFFF);

			if (hitCollector.mHit.mBodyID != bodyID)
				continue;

			auto joltCharacterController = characterController.As<JoltCharacterController>();

			JPH::Vec3 hitPosition = ray.GetPointOnRay(hitCollector.mHit.mFraction);
			JPH::RMat44 inverseCOM = joltCharacterController->m_Controller->GetCenterOfMassTransform().Inversed();
			const auto* shape = joltCharacterController->m_Controller->GetShape();
			JPH::Vec3 surfaceNormal = inverseCOM.Multiply3x3Transposed(shape->GetSurfaceNormal(hitCollector.mHit.mSubShapeID2, JPH::Vec3(inverseCOM * hitPosition))).Normalized();

			outHit.HitEntity = entityID;
			outHit.Position = JoltUtils::FromJoltVector(hitPosition);
			outHit.Normal = JoltUtils::FromJoltVector(surfaceNormal);
			outHit.Distance = glm::distance(rayCastInfo->Origin, outHit.Position);

			auto* controllerShape = reinterpret_cast<PhysicsShape*>(shape->GetUserData());

			if (controllerShape == nullptr)
				controllerShape = reinterpret_cast<PhysicsShape*>(shape->GetSubShapeUserData(hitCollector.mHit.mSubShapeID2));

			outHit.HitCollider = controllerShape;

			return true;
		}

		JPH::BodyLockRead bodyLock(m_JoltSystem->GetBodyLockInterface(), hitCollector.mHit.mBodyID);
		if (bodyLock.Succeeded())
		{
			const JPH::Body& body = bodyLock.GetBody();

			JPH::Vec3 hitPosition = ray.GetPointOnRay(hitCollector.mHit.mFraction);

			outHit.HitEntity = body.GetUserData();
			outHit.Position = JoltUtils::FromJoltVector(hitPosition);
			outHit.Normal = JoltUtils::FromJoltVector(body.GetWorldSpaceSurfaceNormal(hitCollector.mHit.mSubShapeID2, hitPosition));
			outHit.Distance = glm::distance(rayCastInfo->Origin, outHit.Position);
			outHit.HitCollider = reinterpret_cast<PhysicsShape*>(body.GetShape()->GetSubShapeUserData(hitCollector.mHit.mSubShapeID2));

			return true;
		}

		return false;
	}

	bool JoltScene::CastShape(const ShapeCastInfo* shapeCastInfo, SceneQueryHit& outHit)
	{
		JPH::Ref<JPH::Shape> shape = nullptr;

		switch (shapeCastInfo->GetCastType())
		{
		case ShapeCastType::Box:
		{
			const auto* boxCastInfo = reinterpret_cast<const BoxCastInfo*>(shapeCastInfo);
			shape = new JPH::BoxShape(JoltUtils::ToJoltVector(boxCastInfo->HalfExtent));
			break;
		}
		case ShapeCastType::Sphere:
		{
			const auto* sphereCastInfo = reinterpret_cast<const SphereCastInfo*>(shapeCastInfo);
			shape = new JPH::SphereShape(sphereCastInfo->Radius);
			break;
		}
		case ShapeCastType::Capsule:
		{
			const auto* capsuleCastInfo = reinterpret_cast<const CapsuleCastInfo*>(shapeCastInfo);
			shape = new JPH::CapsuleShape(capsuleCastInfo->HalfHeight, capsuleCastInfo->Radius);
			break;
		}
		default:
			ZONG_CORE_VERIFY(false, "Cannot cast mesh shapes!");
		}

		ZONG_CORE_VERIFY(shape, "Failed to create shape?");

		JPH::ShapeCast shapeCast = JPH::ShapeCast::sFromWorldTransform(
			shape,
			JPH::Vec3(1.0f, 1.0f, 1.0f),
			JPH::Mat44::sTranslation(JoltUtils::ToJoltVector(shapeCastInfo->Origin)),
			JoltUtils::ToJoltVector(glm::normalize(shapeCastInfo->Direction)) * shapeCastInfo->MaxDistance
		);

		JPH::ShapeCastSettings shapeCastSettings;
		JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> shapeCastCollector;
		m_JoltSystem->GetNarrowPhaseQuery().CastShape(JPH::RShapeCast(shapeCast), shapeCastSettings, JPH::RVec3::sZero(), shapeCastCollector, {}, {}, JoltRayCastBodyFilter(this, shapeCastInfo->ExcludedEntities));

		for (const auto& [entityID, characterController] : m_CharacterControllers)
		{
			auto joltCharacterController = characterController.As<JoltCharacterController>();
			auto centerOfMassTransform = joltCharacterController->m_Controller->GetCenterOfMassTransform();
			JPH::BodyID bodyID((uint64_t(entityID) >> 32) & 0xFFFFFFFF);
			JPH::TransformedShape transformedShape(centerOfMassTransform.GetTranslation(), centerOfMassTransform.GetRotation().GetQuaternion(), joltCharacterController->m_Controller->GetShape(), bodyID);
			transformedShape.CastShape(JPH::RShapeCast(shapeCast), shapeCastSettings, JPH::RVec3::sZero(), shapeCastCollector);
		}

		if (!shapeCastCollector.HadHit())
			return false;

		// Check if we hit any of the Character Controllers
		for (const auto& [entityID, characterController] : m_CharacterControllers)
		{
			JPH::BodyID bodyID((uint64_t(entityID) >> 32) & 0xFFFFFFFF);

			if (shapeCastCollector.mHit.mBodyID2 != bodyID)
				continue;

			auto joltCharacterController = characterController.As<JoltCharacterController>();

			glm::vec3 hitPosition = shapeCastInfo->Origin + shapeCastCollector.mHit.mFraction * shapeCastInfo->Direction;

			JPH::RMat44 inverseCOM = joltCharacterController->m_Controller->GetCenterOfMassTransform().Inversed();
			const auto* shape = joltCharacterController->m_Controller->GetShape();
			JPH::Vec3 surfaceNormal = inverseCOM.Multiply3x3Transposed(shape->GetSurfaceNormal(shapeCastCollector.mHit.mSubShapeID2, JPH::Vec3(inverseCOM * JoltUtils::ToJoltVector(hitPosition)))).Normalized();

			outHit.HitEntity = entityID;
			outHit.Position = hitPosition;
			outHit.Normal = JoltUtils::FromJoltVector(surfaceNormal);
			outHit.Distance = glm::distance(shapeCastInfo->Origin, hitPosition);

			auto* controllerShape = reinterpret_cast<PhysicsShape*>(shape->GetUserData());

			if (controllerShape == nullptr)
				controllerShape = reinterpret_cast<PhysicsShape*>(shape->GetSubShapeUserData(shapeCastCollector.mHit.mSubShapeID2));

			outHit.HitCollider = controllerShape;
		
			shapeCastCollector.Reset();
			return true;
		}

		JPH::BodyLockRead bodyLock(m_JoltSystem->GetBodyLockInterface(), shapeCastCollector.mHit.mBodyID2);
		if (bodyLock.Succeeded())
		{
			const JPH::Body& body = bodyLock.GetBody();

			glm::vec3 hitPosition = shapeCastInfo->Origin + shapeCastCollector.mHit.mFraction * shapeCastInfo->Direction;

			outHit.HitEntity = body.GetUserData();
			outHit.Position = hitPosition;
			outHit.Normal = JoltUtils::FromJoltVector(body.GetWorldSpaceSurfaceNormal(shapeCastCollector.mHit.mSubShapeID2, JoltUtils::ToJoltVector(hitPosition)));
			outHit.Distance = glm::distance(shapeCastInfo->Origin, hitPosition);
			outHit.HitCollider = reinterpret_cast<PhysicsShape*>(body.GetShape()->GetSubShapeUserData(shapeCastCollector.mHit.mSubShapeID2));

			return true;
		}

		return false;
	}

	int32_t JoltScene::OverlapShape(const ShapeOverlapInfo* shapeOverlapInfo, SceneQueryHit** outHits)
	{
		m_OverlapHitBuffer.clear();

		JPH::Ref<JPH::Shape> shape = nullptr;

		switch (shapeOverlapInfo->GetCastType())
		{
			case ShapeCastType::Box:
			{
				const auto* boxCastInfo = reinterpret_cast<const BoxOverlapInfo*>(shapeOverlapInfo);
				shape = new JPH::BoxShape(JoltUtils::ToJoltVector(boxCastInfo->HalfExtent));
				break;
			}
			case ShapeCastType::Sphere:
			{
				const auto* sphereCastInfo = reinterpret_cast<const SphereOverlapInfo*>(shapeOverlapInfo);
				shape = new JPH::SphereShape(sphereCastInfo->Radius);
				break;
			}
			case ShapeCastType::Capsule:
			{
				const auto* capsuleCastInfo = reinterpret_cast<const CapsuleOverlapInfo*>(shapeOverlapInfo);
				shape = new JPH::CapsuleShape(capsuleCastInfo->HalfHeight, capsuleCastInfo->Radius);
				break;
			}
			default:
				ZONG_CORE_VERIFY(false, "Cannot overlap mesh shapes!");
		}

		ZONG_CORE_VERIFY(shape, "Failed to create shape?");

		JPH::Mat44 worldTransform = JPH::Mat44::sTranslation(JoltUtils::ToJoltVector(shapeOverlapInfo->Origin));
		JPH::Vec3 shapeScale = JPH::Vec3(1.0f, 1.0f, 1.0f);

		JPH::CollideShapeSettings settings;
		JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
		m_JoltSystem->GetNarrowPhaseQuery().CollideShape(shape, shapeScale, worldTransform, settings, JPH::RVec3::sZero(), collector, {}, {}, JoltRayCastBodyFilter(this, shapeOverlapInfo->ExcludedEntities));

		size_t numBodies = collector.mHits.size();

		// Resize body id buffer if needed
		if (numBodies > m_OverlapIDBufferCount)
		{
			delete[] m_OverlapIDBuffer;
			m_OverlapIDBuffer = new JPH::BodyID[numBodies * 2];
			m_OverlapIDBufferCount = numBodies * 2;
		}

		memset(m_OverlapIDBuffer, 0, m_OverlapIDBufferCount * sizeof(JPH::BodyID));

		// Lock all bodies in collector.mHits
		for (size_t i = 0; i < numBodies; i++)
			m_OverlapIDBuffer[i] = collector.mHits[i].mBodyID2;

		{
			JPH::BodyLockMultiRead bodyLock(m_JoltSystem->GetBodyLockInterface(), m_OverlapIDBuffer, numBodies);
			for (size_t i = 0; i < numBodies; i++)
			{
				const JPH::Body* body = bodyLock.GetBody(i);

				if (body == nullptr)
					continue;

				glm::vec3 hitPosition = JoltUtils::FromJoltVector(collector.mHits[i].mContactPointOn2);

				auto& hitInfo = m_OverlapHitBuffer.emplace_back();
				hitInfo.HitEntity = body->GetUserData();
				hitInfo.Position = hitPosition;
				hitInfo.Normal = JoltUtils::FromJoltVector(body->GetWorldSpaceSurfaceNormal(collector.mHits[i].mSubShapeID2, JoltUtils::ToJoltVector(hitPosition)));
				hitInfo.Distance = glm::distance(shapeOverlapInfo->Origin, hitPosition);
				hitInfo.HitCollider = reinterpret_cast<PhysicsShape*>(body->GetShape()->GetSubShapeUserData(collector.mHits[i].mSubShapeID2));
			}
		}

		*outHits = m_OverlapHitBuffer.data();
		return int32_t(m_OverlapHitBuffer.size());
	}

	void JoltScene::Teleport(Entity entity, const glm::vec3& targetPosition, const glm::quat& targetRotation, bool force /*= false*/)
	{
		auto body = GetEntityBody(entity).As<JoltBody>();

		if (!body)
			return;

		m_JoltSystem->GetBodyInterface().SetPositionAndRotationWhenChanged(body->m_BodyID, JoltUtils::ToJoltVector(targetPosition), JoltUtils::ToJoltQuat(targetRotation), JPH::EActivation::Activate);
	}

	void JoltScene::MarkKinematic(Ref<JoltBody> body)
	{
		auto it = std::find(m_KinematicBodies.begin(), m_KinematicBodies.end(), body);

		if (it != m_KinematicBodies.end())
			return;

		m_KinematicBodies.push_back(body);
	}

	void JoltScene::UnmarkKinematic(Ref<JoltBody> body)
	{
		auto it = std::find(m_KinematicBodies.begin(), m_KinematicBodies.end(), body);

		if (it == m_KinematicBodies.end())
			return;

		m_KinematicBodies.erase(it);
	}

	Ref<CharacterController> JoltScene::CreateCharacterController(Entity entity)
	{
		ZONG_CORE_VERIFY(entity.HasComponent<CharacterControllerComponent>());

		if (!entity.HasAny<BoxColliderComponent, CapsuleColliderComponent, SphereColliderComponent>() || entity.HasComponent<MeshColliderComponent>())
			return nullptr;

		Ref<JoltCharacterController> characterController = Ref<JoltCharacterController>::Create(entity);
		m_CharacterControllers[entity.GetUUID()] = characterController;
		return characterController;
	}

	void JoltScene::DestroyCharacterController(Entity entity)
	{
		if (auto it = m_CharacterControllers.find(entity.GetUUID()); it != m_CharacterControllers.end())
			m_CharacterControllers.erase(it);
	}

	JPH::BodyInterface& JoltScene::GetBodyInterface(bool inShouldLock)
	{
		ZONG_CORE_VERIFY(s_CurrentInstance != nullptr);
		return inShouldLock ? s_CurrentInstance->m_JoltSystem->GetBodyInterface() : s_CurrentInstance->m_JoltSystem->GetBodyInterfaceNoLock();
	}

	const JPH::BodyLockInterface& JoltScene::GetBodyLockInterface(bool inShouldLock)
	{
		ZONG_CORE_VERIFY(s_CurrentInstance != nullptr);

		if (inShouldLock)
			return s_CurrentInstance->m_JoltSystem->GetBodyLockInterface();
		else
			return s_CurrentInstance->m_JoltSystem->GetBodyLockInterfaceNoLock();
	}

	JPH::PhysicsSystem& JoltScene::GetJoltSystem() { return *s_CurrentInstance->m_JoltSystem.get(); }

	void JoltScene::ProcessBulkStorage()
	{
		ZONG_PROFILE_FUNC();

		if (m_BulkBufferCount == 0)
			return;

		ZONG_CORE_INFO_TAG("Physics", "Adding {} bodies in bulk!", m_BulkBufferCount);

		// NOTE(Peter): This could be made to happen on a separate background thread, possibly even using Jolt's job system.
		//				The only consideration we'd have to make is that m_BulkAddBuffer can't change until AddBodiesFinalize has finished.
		//				This would require us to have CreateBody wait if we're trying to add bodies in bulk. We could possibly
		//				make this work by having a "back" buffer.
		JPH::BodyInterface::AddState addState = m_JoltSystem->GetBodyInterface().AddBodiesPrepare(m_BulkAddBuffer, static_cast<int32_t>(m_BulkBufferCount));
		m_JoltSystem->GetBodyInterface().AddBodiesFinalize(m_BulkAddBuffer, static_cast<int32_t>(m_BulkBufferCount), addState, JPH::EActivation::Activate);

		m_BodiesAddedSinceOptimization += m_BulkBufferCount;
		m_BulkBufferCount = 0;

		// NOTE(Peter): Optimizing the broadphase after a lot of bodies have been inserted has the advantage of better structured quad trees.
		//				The downside is that it can be a relatively heavy operation, especially if there's lots of bodies.
		//				Optimizing does happen on a background thread, but it does mean JoltSystem::Update has to wait for the new quad trees to be built.
		if (m_BodiesAddedSinceOptimization >= s_BroadPhaseOptimizationThreashold)
		{
			ZONG_CORE_INFO_TAG("Physics", "Optimizing Broad Phase!");
			m_JoltSystem->OptimizeBroadPhase();
			m_BodiesAddedSinceOptimization = 0;
		}
	}

	// NOTE(Peter): Technically inefficient since we'd be locking for every call
	//				But I'm sure it's fine for the most part, this shouldn't be a hot path
	void JoltScene::SynchronizeBodyTransform(WeakRef<PhysicsBody> body)
	{
		ZONG_PROFILE_FUNC();

		if (!body.IsValid() || !body->GetEntity())
			return;

		auto joltBody = body.As<JoltBody>();

		JPH::BodyLockRead bodyLock(JoltScene::GetBodyLockInterface(), joltBody->m_BodyID);
		ZONG_CORE_VERIFY(bodyLock.Succeeded());
		const JPH::Body& bodyRef = bodyLock.GetBody();

		Entity entity = m_EntityScene->GetEntityWithUUID(bodyRef.GetUserData());
		auto& transformComponent = entity.GetComponent<TransformComponent>();
		glm::vec3 scale = transformComponent.Scale;
		transformComponent.Translation = JoltUtils::FromJoltVector(bodyRef.GetPosition());
		transformComponent.SetRotation(JoltUtils::FromJoltQuat(bodyRef.GetRotation()));

		m_EntityScene->ConvertToLocalSpace(entity);
		transformComponent.Scale = scale;
	}

	/////////// RayCast Filter for determining if a body should be excluded from the ray cast result ///////////
	JoltRayCastBodyFilter::JoltRayCastBodyFilter(JoltScene* scene, const ExcludedEntityMap& excludedEntities)
	{
		m_ExcludedBodies.reserve(excludedEntities.size());

		for (UUID entityID : excludedEntities)
		{
			Ref<PhysicsBody> entityBody = scene->GetEntityBodyByID(entityID);
			
			if (!entityBody)
				continue;

			Ref<JoltBody> joltBody = entityBody.As<JoltBody>();
			m_ExcludedBodies.insert(joltBody->GetBodyID());
		}
	}

	bool JoltRayCastBodyFilter::ShouldCollide(const JPH::BodyID& inBodyID) const
	{
		return m_ExcludedBodies.find(inBodyID) == m_ExcludedBodies.end();
	}

	bool JoltRayCastBodyFilter::ShouldCollideLocked(const JPH::Body& inBody) const
	{
		return ShouldCollide(inBody.GetID());
	}

}

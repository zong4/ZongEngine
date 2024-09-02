#include "hzpch.h"
#include "JoltBody.h"

#include "JoltUtils.h"
#include "JoltShapes.h"
#include "JoltScene.h"
#include "JoltLayerInterface.h"

#include "Hazel/Physics/PhysicsSystem.h"

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/MutableCompoundShape.h>

#include <magic_enum.hpp>
using namespace magic_enum::bitwise_operators;

namespace Hazel {

	JoltBody::JoltBody(JPH::BodyInterface& bodyInterface, Entity entity)
		: PhysicsBody(entity)
	{
		const auto& rigidBodyComponent = entity.GetComponent<RigidBodyComponent>();

		switch (rigidBodyComponent.BodyType)
		{
			case EBodyType::Static:
			{
				CreateStaticBody(bodyInterface);
				break;
			}
			case EBodyType::Dynamic:
			case EBodyType::Kinematic:
			{
				CreateDynamicBody(bodyInterface);
				break;
			}
		}

		m_OldMotionType = JoltUtils::ToJoltMotionType(rigidBodyComponent.BodyType);
	}

	JoltBody::~JoltBody()
	{
		
	}

	void JoltBody::SetCollisionLayer(uint32_t layerID)
	{
		JoltScene::GetBodyInterface().SetObjectLayer(m_BodyID, JPH::ObjectLayer(layerID));
	}

	bool JoltBody::IsStatic() const { return JoltScene::GetBodyInterface().GetMotionType(m_BodyID) == JPH::EMotionType::Static; }
	bool JoltBody::IsDynamic() const { return JoltScene::GetBodyInterface().GetMotionType(m_BodyID) == JPH::EMotionType::Dynamic; }
	bool JoltBody::IsKinematic() const { return JoltScene::GetBodyInterface().GetMotionType(m_BodyID) == JPH::EMotionType::Kinematic; }


	void JoltBody::MoveKinematic(const glm::vec3& targetPosition, const glm::quat& targetRotation, float deltaSeconds)
	{
		JPH::BodyLockWrite bodyLock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(bodyLock.Succeeded());

		JPH::Body& body = bodyLock.GetBody();

		if (body.GetMotionType() != JPH::EMotionType::Kinematic)
		{
			HZ_CONSOLE_LOG_ERROR("Cannot call MoveKinematic() on a non-kinematic body!");
			return;
		}

		body.MoveKinematic(JoltUtils::ToJoltVector(targetPosition), JoltUtils::ToJoltQuat(targetRotation), deltaSeconds);
	}


	void JoltBody::Rotate(const glm::vec3& inRotationTimesDeltaTime)
	{
		JPH::BodyLockWrite bodyLock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(bodyLock.Succeeded());

		JPH::Body& body = bodyLock.GetBody();

		if (body.GetMotionType() != JPH::EMotionType::Kinematic)
		{
			HZ_CONSOLE_LOG_ERROR("Cannot call Rotate() on a non-kinematic body!");
			return;
		}

		body.AddRotationStep(JoltUtils::ToJoltVector(inRotationTimesDeltaTime));
	}


	void JoltBody::SetGravityEnabled(bool isEnabled)
	{
		if (!IsDynamic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot set gravity on a non-dynamic body!");
			return;
		}

		JPH::BodyLockWrite lock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(lock.Succeeded());
		lock.GetBody().GetMotionProperties()->SetGravityFactor(isEnabled ? 1.0f : 0.0f);
	}


	void JoltBody::AddForce(const glm::vec3& force, EForceMode forceMode, bool forceWake /*= true*/)
	{
		if (!IsDynamic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot apply force to a non-dynamic body!");
			return;
		}

		JPH::BodyLockWrite lock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(lock.Succeeded());
		auto& body = lock.GetBody();

		switch (forceMode)
		{
			case EForceMode::Force:
				body.AddForce(JoltUtils::ToJoltVector(force));
				break;
			case EForceMode::Impulse:
				body.AddImpulse(JoltUtils::ToJoltVector(force));
				break;
			case EForceMode::VelocityChange:
			{
				body.SetLinearVelocityClamped(body.GetLinearVelocity() + JoltUtils::ToJoltVector(force));

				// Don't try to activate the body if the velocity is near zero
				if (body.GetLinearVelocity().IsNearZero())
					return;

				break;
			}
			case EForceMode::Acceleration:
			{
				// Acceleration can be applied by adding it as a force divided by the inverse mass, since that negates
				// the mass inclusion in the integration steps (which is what differentiates a force being applied vs. an acceleration)
				body.AddForce(JoltUtils::ToJoltVector(force) / body.GetMotionProperties()->GetInverseMass());
				break;
			}
		}

		// Use the non-locking version of the body interface. We've already grabbed a lock for the body
		if (body.IsInBroadPhase() && !body.IsActive())
			JoltScene::GetBodyInterface(false).ActivateBody(m_BodyID);
	}


	void JoltBody::AddForce(const glm::vec3& force, const glm::vec3& location, EForceMode forceMode, bool forceWake /*= true*/)
	{
		if (!IsDynamic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot apply force to a non-dynamic body!");
			return;
		}

		JPH::BodyLockWrite lock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(lock.Succeeded());
		auto& body = lock.GetBody();

		switch (forceMode)
		{
			case EForceMode::Force:
				body.AddForce(JoltUtils::ToJoltVector(force), JoltUtils::ToJoltVector(location));
				break;
			case EForceMode::Impulse:
				body.AddImpulse(JoltUtils::ToJoltVector(force), JoltUtils::ToJoltVector(location));
				break;
			case EForceMode::VelocityChange:
			{
				// NOTE(Peter): Can't change the velocity at a specific point as far as I can tell...
				body.SetLinearVelocityClamped(body.GetLinearVelocity() + JoltUtils::ToJoltVector(force));

				// Don't try to activate the body if the velocity is near zero
				if (body.GetLinearVelocity().IsNearZero())
					return;

				break;
			}
			case EForceMode::Acceleration:
			{
				// Acceleration can be applied by adding it as a force divided by the inverse mass, since that negates
				// the mass inclusion in the integration steps (which is what differentiates a force being applied vs. an acceleration)
				body.AddForce(JoltUtils::ToJoltVector(force) / body.GetMotionProperties()->GetInverseMass(), JoltUtils::ToJoltVector(location));
				break;
			}
		}

		// Use the non-locking version of the body interface. We've already grabbed a lock for the body
		if (body.IsInBroadPhase() && !body.IsActive())
			JoltScene::GetBodyInterface(false).ActivateBody(m_BodyID);
	}


	void JoltBody::AddTorque(const glm::vec3& torque, bool forceWake /*= true*/)
	{
		if (!IsDynamic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot apply torque to a non-dynamic body!");
			return;
		}

		JoltScene::GetBodyInterface().AddTorque(m_BodyID, JoltUtils::ToJoltVector(torque));
	}


	void JoltBody::AddRadialImpulse(const glm::vec3& origin, float radius, float strength, EFalloffMode falloff, bool velocityChange)
	{
		if (!IsDynamic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot apply radial impulse to a non-dynamic body!");
			return;
		}

		JPH::BodyLockRead readLock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(readLock.Succeeded());

		const JPH::Body& bodyRead = readLock.GetBody();

		glm::vec3 centerOfMassPosition = JoltUtils::FromJoltVector(bodyRead.GetCenterOfMassPosition());

		readLock.ReleaseLock();

		glm::vec3 direction = centerOfMassPosition - origin;

		float distance = glm::length(direction);

		if (distance > radius)
			return;

		direction = glm::normalize(direction);

		float impulseMagnitude = strength;

		if (falloff == EFalloffMode::Linear)
			impulseMagnitude *= (1.0f - (distance / radius));

		glm::vec3 impulse = direction * impulseMagnitude;

		JPH::BodyLockWrite writeLock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(writeLock.Succeeded());
		JPH::Body& bodyWrite = writeLock.GetBody();

		if (velocityChange)
		{
			JPH::Vec3 linearVelocity = bodyWrite.GetLinearVelocity() + JoltUtils::ToJoltVector(impulse);
			bodyWrite.SetLinearVelocityClamped(linearVelocity);

			if (linearVelocity.IsNearZero())
				return; // Return so that we don't active the body
		}
		else
		{
			bodyWrite.AddImpulse(JoltUtils::ToJoltVector(impulse));
		}

		if (!bodyWrite.IsActive())
			JoltScene::GetBodyInterface(false).ActivateBody(m_BodyID);
	}


	void JoltBody::ChangeTriggerState(bool isTrigger)
	{
		JPH::BodyLockWrite lock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(lock.Succeeded());
		lock.GetBody().SetIsSensor(isTrigger);
	}

	bool JoltBody::IsTrigger() const
	{
		JPH::BodyLockRead lock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(lock.Succeeded());
		return lock.GetBody().IsSensor();
	}

	float JoltBody::GetMass() const
	{
		if (IsStatic())
		{
			HZ_CONSOLE_LOG_ERROR("Static body does not have mass!");
			return 0.0f;
		}

		JPH::BodyLockRead lock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(lock.Succeeded());
		const JPH::Body& body = lock.GetBody();
		return 1.0f / body.GetMotionProperties()->GetInverseMass();
	}

	void JoltBody::SetMass(float mass)
	{
		if (IsStatic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot set mass on a static body!");
			return;
		}

		JPH::BodyLockWrite lock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(lock.Succeeded());

		JPH::Body& body = lock.GetBody();
		JPH::MassProperties massProperties = body.GetShape()->GetMassProperties();
		massProperties.ScaleToMass(mass);
		massProperties.mInertia(3, 3) = 1.0f;
		body.GetMotionProperties()->SetMassProperties(JPH::EAllowedDOFs::All, massProperties);
	}

	void JoltBody::SetLinearDrag(float inLinearDrag)
	{
		if (IsStatic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot set linear drag on a static body!");
			return;
		}

		JPH::BodyLockWrite lock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(lock.Succeeded());
		JPH::Body& body = lock.GetBody();
		body.GetMotionProperties()->SetLinearDamping(inLinearDrag);
	}

	void JoltBody::SetAngularDrag(float inAngularDrag)
	{
		if (IsStatic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot set angular drag on a static body!");
			return;
		}

		JPH::BodyLockWrite lock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(lock.Succeeded());
		JPH::Body& body = lock.GetBody();
		body.GetMotionProperties()->SetAngularDamping(inAngularDrag);
	}

	glm::vec3 JoltBody::GetLinearVelocity() const
	{
		if (IsStatic())
		{
			return glm::vec3(0.0f);
		}

		return JoltUtils::FromJoltVector(JoltScene::GetBodyInterface().GetLinearVelocity(m_BodyID));
	}

	void JoltBody::SetLinearVelocity(const glm::vec3& inVelocity)
	{
		if (!IsDynamic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot set linear velocity on a non-dynamic body!  Move this body via the entity's transform component.");
			return;
		}

		JoltScene::GetBodyInterface().SetLinearVelocity(m_BodyID, JoltUtils::ToJoltVector(inVelocity));
	}

	glm::vec3 JoltBody::GetAngularVelocity() const
	{
		if (IsStatic())
		{
			return glm::vec3(0.0f);
		}

		return JoltUtils::FromJoltVector(JoltScene::GetBodyInterface().GetAngularVelocity(m_BodyID));
	}

	void JoltBody::SetAngularVelocity(const glm::vec3& inVelocity)
	{
		if (!IsDynamic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot set angular velocity on a non-dynamic body!  Rotate this body via the entity's transform component.");
			return;
		}

		JPH::BodyLockWrite lock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(lock.Succeeded());
		auto& body = lock.GetBody();
		auto velocity = JoltUtils::ToJoltVector(inVelocity);
		body.SetAngularVelocityClamped(velocity);

		if (!body.IsActive() && !velocity.IsNearZero() && body.IsInBroadPhase())
			JoltScene::GetBodyInterface(false).ActivateBody(m_BodyID);
	}

	float JoltBody::GetMaxLinearVelocity() const
	{
		if (!IsDynamic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot get max linear velocity on a non-dynamic body!");
			return 0.0f;
		}

		JPH::BodyLockRead lock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(lock.Succeeded());
		return lock.GetBody().GetMotionProperties()->GetMaxLinearVelocity();
	}

	void JoltBody::SetMaxLinearVelocity(float inMaxVelocity)
	{
		if (!IsDynamic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot set max linear velocity on a non-dynamic body!");
			return;
		}

		JPH::BodyLockWrite lock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(lock.Succeeded());
		lock.GetBody().GetMotionProperties()->SetMaxLinearVelocity(inMaxVelocity);
	}

	float JoltBody::GetMaxAngularVelocity() const
	{
		if (!IsDynamic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot get max angular velocity on a non-dynamic body!");
			return 0.0f;
		}

		JPH::BodyLockRead lock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(lock.Succeeded());
		return lock.GetBody().GetMotionProperties()->GetMaxAngularVelocity();
	}

	void JoltBody::SetMaxAngularVelocity(float inMaxVelocity)
	{
		if (!IsDynamic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot set max angular velocity on a non-dynamic body!");
			return;
		}

		JPH::BodyLockWrite lock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(lock.Succeeded());
		lock.GetBody().GetMotionProperties()->SetMaxAngularVelocity(inMaxVelocity);
	}

	bool JoltBody::IsSleeping() const
	{
		if (IsStatic())
		{
			return false;
		}

		JPH::BodyLockRead lock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(lock.Succeeded());
		return !lock.GetBody().IsActive();
	}

	void JoltBody::SetSleepState(bool inSleep)
	{
		if (IsStatic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot change sleep state of a static body!");
			return;
		}

		JPH::BodyLockWrite lock(JoltScene::GetBodyLockInterface(), m_BodyID);

		if (!lock.Succeeded())
			return;

		auto& body = lock.GetBody();

		if (inSleep)
		{
			JoltScene::GetBodyInterface(false).DeactivateBody(m_BodyID);
		}
		else if (body.IsInBroadPhase())
		{
			JoltScene::GetBodyInterface(false).ActivateBody(m_BodyID);
		}
	}


	void JoltBody::SetCollisionDetectionMode(ECollisionDetectionType collisionDetectionMode)
	{
		JoltScene::GetBodyInterface().SetMotionQuality(m_BodyID, JoltUtils::ToJoltMotionQuality(collisionDetectionMode));
	}


	glm::vec3 JoltBody::GetTranslation() const
	{
		return JoltUtils::FromJoltVector(JoltScene::GetBodyInterface().GetPosition(m_BodyID));
	}


	glm::quat JoltBody::GetRotation() const
	{
		return JoltUtils::FromJoltQuat(JoltScene::GetBodyInterface().GetRotation(m_BodyID));
	}


	void JoltBody::SetTranslation(const glm::vec3& translation)
	{
		JPH::BodyLockWrite bodyLock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(bodyLock.Succeeded());
		JPH::Body& body = bodyLock.GetBody();

		if (!body.IsStatic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot set translation on a non-static body!");
			return;
		}

		JoltScene::GetBodyInterface(false).SetPosition(m_BodyID, JoltUtils::ToJoltVector(translation), JPH::EActivation::DontActivate);

		// Make sure this change takes effect in the entity TransformComponent
		auto entityScene = Scene::GetScene(m_Entity.GetSceneUUID());
		entityScene->GetPhysicsScene()->MarkForSynchronization(this);
	}


	void JoltBody::SetRotation(const glm::quat& rotation)
	{
		JPH::BodyLockWrite bodyLock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(bodyLock.Succeeded());
		JPH::Body& body = bodyLock.GetBody();

		if (!body.IsStatic())
		{
			HZ_CONSOLE_LOG_ERROR("Cannot set rotation on a non-static body!");
			return;
		}

		JoltScene::GetBodyInterface(false).SetRotation(m_BodyID, JoltUtils::ToJoltQuat(rotation), JPH::EActivation::DontActivate);

		// Make sure this change takes effect in the entity TransformComponent
		auto entityScene = Scene::GetScene(m_Entity.GetSceneUUID());
		entityScene->GetPhysicsScene()->MarkForSynchronization(this);
	}


	void JoltBody::OnAxisLockUpdated(bool forceWake)
	{
		JPH::BodyLockWrite bodyLock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(bodyLock.Succeeded());

		if (m_AxisLockConstraint)
		{
			JoltScene::GetJoltSystem().RemoveConstraint(m_AxisLockConstraint);
			m_AxisLockConstraint = nullptr;
		}

		// Don't recreate if we have no locked axes
		if (m_LockedAxes != EActorAxis::None)
			CreateAxisLockConstraint(bodyLock.GetBody());
	}

	void JoltBody::CreateStaticBody(JPH::BodyInterface& bodyInterface)
	{
		Ref<Scene> scene = Scene::GetScene(m_Entity.GetSceneUUID());
		HZ_CORE_VERIFY(scene, "No scene active?");

		const TransformComponent worldTransform = scene->GetWorldSpaceTransform(m_Entity);
		const auto& rigidBodyComponent = m_Entity.GetComponent<RigidBodyComponent>();

		CreateCollisionShapesForEntity(m_Entity);

		if (m_Shapes.empty())
			return;

		JPH::Shape* firstShape = nullptr;

		if (m_Shapes.find(ShapeType::CompoundShape) != m_Shapes.end())
		{
			firstShape = static_cast<JPH::Shape*>(m_Shapes.at(ShapeType::CompoundShape)[0]->GetNativeShape());
		}
		else if (m_Shapes.find(ShapeType::MutableCompoundShape) != m_Shapes.end())
		{
			firstShape = static_cast<JPH::Shape*>(m_Shapes.at(ShapeType::MutableCompoundShape)[0]->GetNativeShape());
		}
		else
		{
			for (const auto& [shapeType, shapeStorage] : m_Shapes)
			{
				firstShape = static_cast<JPH::Shape*>(shapeStorage[0]->GetNativeShape());
				break;
			}
		}

		if (firstShape == nullptr)
		{
			HZ_CORE_INFO_TAG("Physics", "Failed to create static PhysicsBody, no collision shape provided!");
			m_Shapes.clear();
			return;
		}

		JPH::BodyCreationSettings bodySettings(
			firstShape,
			JoltUtils::ToJoltVector(worldTransform.Translation),
			JoltUtils::ToJoltQuat(glm::normalize(worldTransform.GetRotation())),
			JPH::EMotionType::Static,
			JPH::ObjectLayer(rigidBodyComponent.LayerID)
		);
		bodySettings.mIsSensor = rigidBodyComponent.IsTrigger;
		bodySettings.mAllowDynamicOrKinematic = rigidBodyComponent.EnableDynamicTypeChange;
		bodySettings.mAllowSleeping = false;

		JPH::Body* body = bodyInterface.CreateBody(bodySettings);

		if (body == nullptr)
		{
			HZ_CORE_ERROR_TAG("Physics", "Failed to create PhysicsBody! This means we don't have enough space to store more RigidBodies!");
			return;
		}

		body->SetUserData(m_Entity.GetUUID());
		m_BodyID = body->GetID();
	}

	void JoltBody::CreateDynamicBody(JPH::BodyInterface& bodyInterface)
	{
		Ref<Scene> scene = Scene::GetScene(m_Entity.GetSceneUUID());
		HZ_CORE_VERIFY(scene, "No scene active?");

		const TransformComponent worldTransform = scene->GetWorldSpaceTransform(m_Entity);
		auto& rigidBodyComponent = m_Entity.GetComponent<RigidBodyComponent>();

		CreateCollisionShapesForEntity(m_Entity);

		if (m_Shapes.empty())
			return;

		JPH::Shape* firstShape = nullptr;

		if (m_Shapes.find(ShapeType::CompoundShape) != m_Shapes.end())
		{
			firstShape = static_cast<JPH::Shape*>(m_Shapes.at(ShapeType::CompoundShape)[0]->GetNativeShape());
		}
		else if (m_Shapes.find(ShapeType::MutableCompoundShape) != m_Shapes.end())
		{
			firstShape = static_cast<JPH::Shape*>(m_Shapes.at(ShapeType::MutableCompoundShape)[0]->GetNativeShape());
		}
		else
		{
			for (const auto& [shapeType, shapeStorage] : m_Shapes)
			{
				firstShape = static_cast<JPH::Shape*>(shapeStorage[0]->GetNativeShape());
				break;
			}
		}

		if (firstShape == nullptr)
		{
			HZ_CORE_INFO_TAG("Physics", "Failed to create dynamic PhysicsBody, no collision shape provided!");
			m_Shapes.clear();
			return;
		}

		if (!PhysicsLayerManager::IsLayerValid(rigidBodyComponent.LayerID))
		{
			rigidBodyComponent.LayerID = 0; // Use the default layer
			HZ_CONSOLE_LOG_WARN("Entity '{}' has a RigidBodyComponent with an invalid layer set! Using the Default layer.", m_Entity.Name());
		}

		JPH::BodyCreationSettings bodySettings(
			firstShape,
			JoltUtils::ToJoltVector(worldTransform.Translation),
			JoltUtils::ToJoltQuat(glm::normalize(worldTransform.GetRotation())),
			JoltUtils::ToJoltMotionType(rigidBodyComponent.BodyType),
			JPH::ObjectLayer(rigidBodyComponent.LayerID)
		);
		bodySettings.mLinearDamping = rigidBodyComponent.LinearDrag;
		bodySettings.mAngularDamping = rigidBodyComponent.AngularDrag;
		bodySettings.mIsSensor = rigidBodyComponent.IsTrigger;
		bodySettings.mGravityFactor = rigidBodyComponent.DisableGravity ? 0.0f : 1.0f;
		bodySettings.mLinearVelocity = JoltUtils::ToJoltVector(rigidBodyComponent.InitialLinearVelocity);
		bodySettings.mAngularVelocity = JoltUtils::ToJoltVector(rigidBodyComponent.InitialAngularVelocity);
		bodySettings.mMaxLinearVelocity = rigidBodyComponent.MaxLinearVelocity;
		bodySettings.mMaxAngularVelocity = rigidBodyComponent.MaxAngularVelocity;
		bodySettings.mMotionQuality = JoltUtils::ToJoltMotionQuality(rigidBodyComponent.CollisionDetection);
		bodySettings.mAllowSleeping = true;

		JPH::Body* body = bodyInterface.CreateBody(bodySettings);
		if (body == nullptr)
		{
			HZ_CORE_ERROR_TAG("Physics", "Failed to create PhysicsBody! This means we don't have enough space to store more RigidBodies!");
			return;
		}

		body->SetUserData(m_Entity.GetUUID());
		m_BodyID = body->GetID();

		// Only create the constraint if it's actually needed
		if (rigidBodyComponent.LockedAxes != EActorAxis::None)
			CreateAxisLockConstraint(*body);
	}

	void JoltBody::CreateAxisLockConstraint(JPH::Body& body)
	{
		JPH::SixDOFConstraintSettings constraintSettings;
		constraintSettings.mPosition1 = constraintSettings.mPosition2 = body.GetCenterOfMassPosition();

		if ((m_LockedAxes & EActorAxis::TranslationX) != EActorAxis::None)
			constraintSettings.MakeFixedAxis(JPH::SixDOFConstraintSettings::TranslationX);

		if ((m_LockedAxes & EActorAxis::TranslationY) != EActorAxis::None)
			constraintSettings.MakeFixedAxis(JPH::SixDOFConstraintSettings::TranslationY);

		if ((m_LockedAxes & EActorAxis::TranslationZ) != EActorAxis::None)
			constraintSettings.MakeFixedAxis(JPH::SixDOFConstraintSettings::TranslationZ);

		if ((m_LockedAxes & EActorAxis::RotationX) != EActorAxis::None)
			constraintSettings.MakeFixedAxis(JPH::SixDOFConstraintSettings::RotationX);

		if ((m_LockedAxes & EActorAxis::RotationY) != EActorAxis::None)
			constraintSettings.MakeFixedAxis(JPH::SixDOFConstraintSettings::RotationY);

		if ((m_LockedAxes & EActorAxis::RotationZ) != EActorAxis::None)
			constraintSettings.MakeFixedAxis(JPH::SixDOFConstraintSettings::RotationZ);

		m_AxisLockConstraint = (JPH::SixDOFConstraint*)constraintSettings.Create(JPH::Body::sFixedToWorld, body);

		JoltScene::GetJoltSystem().AddConstraint(m_AxisLockConstraint);
	}

	void JoltBody::Release()
	{
		JPH::BodyLockWrite lock(JoltScene::GetBodyLockInterface(), m_BodyID);
		HZ_CORE_VERIFY(lock.Succeeded());
		auto& body = lock.GetBody();

		if (m_AxisLockConstraint != nullptr)
		{
			JoltScene::GetJoltSystem().RemoveConstraint(m_AxisLockConstraint);
			m_AxisLockConstraint = nullptr;
		}

		if (body.IsInBroadPhase())
			JoltScene::GetBodyInterface(false).RemoveBody(m_BodyID);
		else
			JoltScene::GetBodyInterface(false).DeactivateBody(m_BodyID);

		m_Shapes.clear();
		JoltScene::GetBodyInterface(false).DestroyBody(m_BodyID);
	}

}

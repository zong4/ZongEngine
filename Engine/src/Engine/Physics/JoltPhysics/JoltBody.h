#pragma once

#include "Engine/Physics/PhysicsBody.h"

#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>

namespace Hazel {

	class JoltBody : public PhysicsBody
	{
	public:
		JoltBody(JPH::BodyInterface& bodyInterface, Entity entity);
		~JoltBody();

		virtual void SetCollisionLayer(uint32_t layerID) override;

		virtual bool IsStatic() const override;
		virtual bool IsDynamic() const override;
		virtual bool IsKinematic() const override;

		virtual void SetGravityEnabled(bool isEnabled) override;

		virtual void AddForce(const glm::vec3& force, EForceMode forceMode = EForceMode::Force, bool forceWake = true) override;
		virtual void AddForce(const glm::vec3& force, const glm::vec3& location, EForceMode forceMode = EForceMode::Force, bool forceWake = true) override;
		virtual void AddTorque(const glm::vec3& torque, bool forceWake = true) override;

		virtual void ChangeTriggerState(bool isTrigger) override;
		virtual bool IsTrigger() const override;

		virtual float GetMass() const override;
		virtual void SetMass(float mass) override;

		virtual void SetLinearDrag(float inLinearDrag) override;
		virtual void SetAngularDrag(float inAngularDrag) override;

		virtual glm::vec3 GetLinearVelocity() const override;
		virtual void SetLinearVelocity(const glm::vec3& inVelocity) override;

		virtual glm::vec3 GetAngularVelocity() const override;
		virtual void SetAngularVelocity(const glm::vec3& inVelocity) override;

		virtual float GetMaxLinearVelocity() const override;
		virtual void SetMaxLinearVelocity(float inMaxVelocity) override;

		virtual float GetMaxAngularVelocity() const override;
		virtual void SetMaxAngularVelocity(float inMaxVelocity) override;

		virtual bool IsSleeping() const override;
		virtual void SetSleepState(bool inSleep) override;

		virtual void AddRadialImpulse(const glm::vec3& origin, float radius, float strength, EFalloffMode falloff, bool velocityChange) override;

		virtual void SetCollisionDetectionMode(ECollisionDetectionType collisionDetectionMode) override;

		JPH::BodyID GetBodyID() const { return m_BodyID; }

		virtual void MoveKinematic(const glm::vec3& targetPosition, const glm::quat& targetRotation, float deltaSeconds) override;

		virtual void SetTranslation(const glm::vec3& translation) override;
		virtual glm::vec3 GetTranslation() const override;

		virtual void SetRotation(const glm::quat& rotation) override;
		virtual glm::quat GetRotation() const override;

		virtual void Rotate(const glm::vec3& inRotationTimesDeltaTime) override;

	private:
		virtual void OnAxisLockUpdated(bool forceWake) override;

	private:
		void CreateStaticBody(JPH::BodyInterface& bodyInterface);
		void CreateDynamicBody(JPH::BodyInterface& bodyInterface);

		void CreateAxisLockConstraint(JPH::Body& body);

		void Release();

	private:
		JPH::BodyID m_BodyID;
		JPH::EMotionType m_OldMotionType = JPH::EMotionType::Static;

		glm::vec3 m_KinematicTargetPosition = { 0.0f, 0.0f, 0.0f };
		glm::quat m_KinematicTargetRotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		float m_KinematicTargetTime = 0.0f;

		JPH::SixDOFConstraint* m_AxisLockConstraint = nullptr;

	private:
		friend class JoltScene;
	};

}

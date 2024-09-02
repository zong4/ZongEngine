#pragma once

#include "Hazel/Physics/CharacterController.h"
#include "Hazel/Physics/PhysicsContactCallback.h"
#include "Hazel/Physics/PhysicsShapes.h"

#include <Jolt/Core/Reference.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Body/BodyManager.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

namespace Hazel {

	class JoltCharacterController : public CharacterController, JPH::CharacterContactListener
	{
	public:
		JoltCharacterController(Entity entity, const ContactCallbackFn& contactCallback);
		~JoltCharacterController();

		virtual void SetGravityEnabled(bool enableGravity) override { m_HasGravity = enableGravity; }
		virtual bool IsGravityEnabled() const override { return m_HasGravity; }

		virtual void SetSlopeLimit(float slopeLimit) override;
		virtual void SetStepOffset(float stepOffset) override;

		virtual void SetTranslation(const glm::vec3& inTranslation) override;
		virtual void SetRotation(const glm::quat& inRotation) override;

		virtual bool IsGrounded() const override;

		virtual void SetControlMovementInAir(bool controlMovementInAir) override { m_ControlMovementInAir = controlMovementInAir; }
		virtual bool CanControlMovementInAir() const override { return m_ControlMovementInAir; }
		virtual void SetControlRotationInAir(bool controlRotationInAir) override { m_ControlRotationInAir = controlRotationInAir; }
		virtual bool CanControlRotationInAir() const override { return m_ControlRotationInAir; }

		virtual ECollisionFlags GetCollisionFlags() const override { return m_CollisionFlags; }

		virtual void Move(const glm::vec3& displacement) override;
		virtual void Rotate(const glm::quat& rotation) override;
		virtual void Jump(float jumpPower) override;

		virtual glm::vec3 GetLinearVelocity() const override;
		virtual void SetLinearVelocity(const glm::vec3& linearVelocity) override;

	private:
		virtual void PreSimulate(float deltaTime) override;
		virtual void Simulate(float deltaTime) override;
		virtual void PostSimulate() override;

		void Create();

		void OnAdjustBodyVelocity(const JPH::CharacterVirtual* inCharacter, const JPH::Body& inBody2, JPH::Vec3& ioLinearVelocity, JPH::Vec3& ioAngularVelocity) override;
		bool OnContactValidate(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2) override;
		void OnContactAdded(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings) override;
		void OnContactSolve(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::Vec3Arg inContactVelocity, const JPH::PhysicsMaterial* inContactMaterial, JPH::Vec3Arg inCharacterVelocity, JPH::Vec3& ioNewCharacterVelocity) override;

		void HandleTrigger(Ref<Scene> scene, const JPH::BodyID bodyID2);
		void HandleCollision(Ref<Scene> scene, const JPH::BodyID bodyID2);

	private:
		JPH::BodyIDVector m_TriggeredBodies;
		JPH::BodyIDVector m_StillTriggeredBodies;
		JPH::BodyIDVector m_CollidedBodies;
		JPH::BodyIDVector m_StillCollidedBodies;

		JPH::Quat m_Rotation = JPH::Quat::sIdentity();    // rotation (if any) to be applied during next physics update (comes from Rotate() calls)
		JPH::Vec3 m_Displacement = JPH::Vec3::sZero();    // displacement (if any) to be applied during next physics update (comes from Move() calls)
		JPH::Vec3 m_DesiredVelocity = JPH::Vec3::sZero(); // desired velocity for next physics update is calculated during PreSimulate()

		Entity m_Entity;

		ContactCallbackFn m_ContactEventCallback;

		JPH::Ref<JPH::CharacterVirtual> m_Controller;
		Ref<PhysicsShape> m_Shape;

		float m_JumpPower = 0.0f;
		float m_StepOffset = 0.0f;

		uint32_t m_CollisionLayer;
		ECollisionFlags m_CollisionFlags = ECollisionFlags::None;

		bool m_HasGravity = true;
		bool m_ControlMovementInAir = false;
		bool m_ControlRotationInAir = false;
		bool m_AllowSliding = false;

	private:
		friend class JoltScene;
	};
}

#pragma once

#include "Engine/Physics/CharacterController.h"
#include "Engine/Physics/PhysicsShapes.h"

#include <Jolt/Physics/Character/CharacterVirtual.h>

namespace Hazel {

	class JoltCharacterController : public CharacterController
	{
	public:
		JoltCharacterController(Entity entity);
		~JoltCharacterController();

		virtual void SetGravityEnabled(bool enableGravity) override { m_HasGravity = enableGravity; }
		virtual bool IsGravityEnabled() const override { return m_HasGravity; }

		virtual void SetSlopeLimit(float slopeLimit) override;
		virtual void SetStepOffset(float stepOffset) override;

		virtual void SetTranslation(const glm::vec3& inTranslation) override;
		virtual void SetRotation(const glm::quat& inRotation) override;

		virtual bool IsGrounded() const override;

		virtual ECollisionFlags GetCollisionFlags() const override { return m_CollisionFlags; }

		virtual void Move(const glm::vec3& displacement) override;
		virtual void Jump(float jumpPower) override;

		virtual glm::vec3 GetLinearVelocity() const override;
		virtual void SetLinearVelocity(const glm::vec3& linearVelocity) override;

	private:
		virtual void PreSimulate(float deltaTime) override;
		virtual void Simulate(float deltaTime) override;

		void Create();

		class Listener : public JPH::CharacterContactListener
		{
		public:
			Listener(JoltCharacterController* characterController)
				: m_CharacterController(characterController) {}

			bool OnContactValidate(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2);
			void OnContactAdded(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings);

		private:
			JoltCharacterController* m_CharacterController;
		};

	private:
		Entity m_Entity;
		Listener m_Listener;

		JPH::Ref<JPH::CharacterVirtual> m_Controller;
		Ref<PhysicsShape> m_Shape;

		float m_JumpPower = 0.0f;
		glm::vec3 m_Displacement = {};   // displacement (if any) for next update (comes from Move() calls)
		glm::vec3 m_DesiredVelocity = {};
		bool m_HasGravity = true;

		float m_StepOffset = 0.0f;
		uint32_t m_CollisionLayer;

		ECollisionFlags m_CollisionFlags = ECollisionFlags::None;

	private:
		friend class JoltScene;
	};
}

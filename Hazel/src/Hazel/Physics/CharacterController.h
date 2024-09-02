#pragma once

#include "Hazel/Core/Ref.h"
#include "Hazel/Scene/Entity.h"

#include "PhysicsTypes.h"

#include <glm/glm.hpp>

namespace Hazel {

	class CharacterController : public RefCounted
	{
	public:
		virtual ~CharacterController() = default;

		virtual void SetGravityEnabled(bool enableGravity) = 0;
		virtual bool IsGravityEnabled() const = 0;

		virtual void SetSlopeLimit(float slopeLimit) = 0;
		virtual void SetStepOffset(float stepOffset) = 0;

		// instantly set the translation/rotation of the character. aka "teleport"
		virtual void SetTranslation(const glm::vec3& inTranslation) = 0;
		virtual void SetRotation(const glm::quat& inRotation) = 0;

		virtual bool IsGrounded() const = 0;

		virtual void SetControlMovementInAir(bool controlMovementInAir) = 0;
		virtual bool CanControlMovementInAir() const = 0;
		virtual void SetControlRotationInAir(bool controlRotationInAir) = 0;
		virtual bool CanControlRotationInAir() const = 0;

		virtual ECollisionFlags GetCollisionFlags() const = 0;

		// Incrementally move/rotate/jump the character during physics simulation
		// Unless you know you want to teleport the character, prefer these functions over SetTranslation/SetRotation
		virtual void Move(const glm::vec3& displacement) = 0;
		virtual void Rotate(const glm::quat& rotation) = 0;
		virtual void Jump(float jumpPower) = 0;

		virtual glm::vec3 GetLinearVelocity() const = 0;
		virtual void SetLinearVelocity(const glm::vec3& inVelocity) = 0;

	private:
		virtual void PreSimulate(float deltaTime) {}
		virtual void Simulate(float deltaTime) = 0;
		virtual void PostSimulate() {}

	private:
		friend class PhysicsScene;
	};

}

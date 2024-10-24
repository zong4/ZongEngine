#pragma once

#include "Engine/Core/Ref.h"
#include "Engine/Scene/Entity.h"

#include "PhysicsTypes.h"

#include <glm/glm.hpp>

namespace Engine {

	class CharacterController : public RefCounted
	{
	public:
		virtual ~CharacterController() = default;

		virtual void SetGravityEnabled(bool enableGravity) = 0;
		virtual bool IsGravityEnabled() const = 0;

		virtual void SetSlopeLimit(float slopeLimit) = 0;
		virtual void SetStepOffset(float stepOffset) = 0;

		virtual void SetTranslation(const glm::vec3& inTranslation) = 0;
		virtual void SetRotation(const glm::quat& inRotation) = 0;

		virtual bool IsGrounded() const = 0;

		virtual ECollisionFlags GetCollisionFlags() const = 0;

		virtual void Move(const glm::vec3& displacement) = 0;
		virtual void Jump(float jumpPower) = 0;

		virtual glm::vec3 GetLinearVelocity() const = 0;
		virtual void SetLinearVelocity(const glm::vec3& inVelocity) = 0;

	private:
		virtual void PreSimulate(float deltaTime) {}
		virtual void Simulate(float deltaTime) = 0;

	private:
		friend class PhysicsScene;
	};

}

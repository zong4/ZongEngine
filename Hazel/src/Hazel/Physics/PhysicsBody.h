#pragma once

#include "Hazel/Scene/Scene.h"
#include "PhysicsShapes.h"

namespace Hazel {

	using ShapeArray = std::vector<Ref<PhysicsShape>>;

	class PhysicsBody : public RefCounted
	{
	public:
		PhysicsBody(Entity entity)
			: m_Entity(entity), m_LockedAxes(EActorAxis::None)
		{
			const auto& rigidBodyComponent = entity.GetComponent<RigidBodyComponent>();
			m_LockedAxes = rigidBodyComponent.LockedAxes;
		}

		virtual ~PhysicsBody() = default;

		Entity GetEntity() { return m_Entity; }
		const Entity& GetEntity() const { return m_Entity; }

		uint32_t GetShapeCount(ShapeType type) const
		{
			if (auto it = m_Shapes.find(type); it != m_Shapes.end())
				return uint32_t(it->second.size());

			return 0;
		}

		const ShapeArray& GetShapes(ShapeType type) const
		{
			if (auto it = m_Shapes.find(type); it != m_Shapes.end())
				return it->second;

			return ShapeArray();
		}

		Ref<PhysicsShape> GetShape(ShapeType type, uint32_t index) const
		{
			if (m_Shapes.find(type) == m_Shapes.end() || index >= uint32_t(m_Shapes.at(type).size()))
				return nullptr;

			return m_Shapes.at(type).at(index);
		}

		virtual void SetCollisionLayer(uint32_t layerID) = 0;

		virtual bool IsStatic() const = 0;
		virtual bool IsDynamic() const = 0;
		virtual bool IsKinematic() const = 0;

		virtual void SetGravityEnabled(bool isEnabled) = 0;

		virtual void MoveKinematic(const glm::vec3& targetPosition, const glm::quat& targetRotation, float deltaSeconds) = 0;

		virtual void AddForce(const glm::vec3& force, EForceMode forceMode = EForceMode::Force, bool forceWake = true) = 0;
		virtual void AddForce(const glm::vec3& force, const glm::vec3& location, EForceMode forceMode = EForceMode::Force, bool forceWake = true) = 0;
		virtual void AddTorque(const glm::vec3& torque, bool forceWake = true) = 0;

		virtual void ChangeTriggerState(bool isTrigger) = 0;
		virtual bool IsTrigger() const = 0;

		virtual float GetMass() const = 0;
		virtual void SetMass(float mass) = 0;

		virtual void SetLinearDrag(float inLinearDrag) = 0;
		virtual void SetAngularDrag(float inAngularDrag) = 0;

		virtual glm::vec3 GetLinearVelocity() const = 0;
		virtual void SetLinearVelocity(const glm::vec3& inVelocity) = 0;

		virtual glm::vec3 GetAngularVelocity() const = 0;
		virtual void SetAngularVelocity(const glm::vec3& inVelocity) = 0;

		virtual float GetMaxLinearVelocity() const = 0;
		virtual void SetMaxLinearVelocity(float inMaxVelocity) = 0;

		virtual float GetMaxAngularVelocity() const = 0;
		virtual void SetMaxAngularVelocity(float inMaxVelocity) = 0;

		virtual bool IsSleeping() const = 0;
		virtual void SetSleepState(bool inSleep) = 0;

		virtual void AddRadialImpulse(const glm::vec3& origin, float radius, float strength, EFalloffMode falloff, bool velocityChange) = 0;

		virtual void SetCollisionDetectionMode(ECollisionDetectionType collisionDetectionMode) = 0;

		void SetAxisLock(EActorAxis axis, bool locked, bool forceWake);
		bool IsAxisLocked(EActorAxis axis) const;
		EActorAxis GetLockedAxes() const { return m_LockedAxes; }
		bool IsAllRotationLocked() const { return IsAxisLocked(EActorAxis::RotationX) && IsAxisLocked(EActorAxis::RotationY) && IsAxisLocked(EActorAxis::RotationZ); }

		/// <summary>
		/// Sets the new position of the body. Only affects static bodies.
		/// </summary>
		/// <param name="translation">The new position of this body</param>
		virtual void SetTranslation(const glm::vec3& translation) = 0;
		virtual glm::vec3 GetTranslation() const = 0;

		/// <summary>
		/// Sets the new rotation of the body. Only affects static bodies.
		/// </summary>
		/// <param name="rotation">The new rotation of this body</param>
		virtual void SetRotation(const glm::quat& rotation) = 0;
		virtual glm::quat GetRotation() const = 0;

		virtual void Rotate(const glm::vec3& inRotationTimesDeltaTime) = 0;

	private:
		virtual void OnAxisLockUpdated(bool forceWake) = 0;

	protected:
		void CreateCollisionShapesForEntity(Entity entity, bool ignoreCompoundShapes = false);

	protected:
		Entity m_Entity;

		std::unordered_map<ShapeType, ShapeArray> m_Shapes;

		EActorAxis m_LockedAxes;
	};

}

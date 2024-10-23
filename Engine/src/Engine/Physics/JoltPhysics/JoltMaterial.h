#pragma once

#include "Engine/Scene/Components.h"

#include <Jolt/Physics/Collision/PhysicsMaterial.h>

namespace Hazel {

	class JoltMaterial : public JPH::PhysicsMaterial
	{
	public:
		JoltMaterial() = default;
		JoltMaterial(float friction, float restitution)
			: m_Friction(friction), m_Restitution(restitution)
		{}

		float GetFriction() const { return m_Friction; }
		void SetFriction(float friction) { m_Friction = friction; }

		float GetRestitution() const { return m_Restitution; }
		void SetRestitution(float restitution) { m_Restitution = restitution; }

		inline static JoltMaterial* FromColliderMaterial(const ColliderMaterial& colliderMaterial)
		{
			return new JoltMaterial(colliderMaterial.Friction, colliderMaterial.Restitution);
		}

	private:
		float m_Friction;
		float m_Restitution;

	};

}

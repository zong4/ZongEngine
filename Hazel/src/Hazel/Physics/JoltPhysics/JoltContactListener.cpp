#include "hzpch.h"
#include "JoltContactListener.h"
#include "JoltMaterial.h"

#include "Hazel/Script/ScriptEngine.h"

#include <thread>

namespace Hazel {

	void JoltContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		if (!ScriptEngine::GetSceneContext()->IsPlaying())
			return;

		OverrideFrictionAndRestitution(inBody1, inBody2, inManifold, ioSettings);

		if (ioSettings.mIsSensor)
			OnTriggerBegin(inBody1, inBody2, inManifold, ioSettings); 
		else
			OnCollisionBegin(inBody1, inBody2, inManifold, ioSettings);
	}

	void JoltContactListener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		OverrideFrictionAndRestitution(inBody1, inBody2, inManifold, ioSettings);
	}

	void JoltContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
	{
		if (!ScriptEngine::GetSceneContext()->IsPlaying())
			return;

		JPH::Body* body1 = m_BodyLockInterface->TryGetBody(inSubShapePair.GetBody1ID());
		JPH::Body* body2 = m_BodyLockInterface->TryGetBody(inSubShapePair.GetBody2ID());

		if (body1 == nullptr || body2 == nullptr)
			return;

		if (body1->IsSensor() || body2->IsSensor())
		{
			OnTriggerEnd(*body1, *body2);
		}
		else
		{
			OnCollisionEnd(*body1, *body2);
		}
	}

	void JoltContactListener::OnCollisionBegin(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		Entity entity1 = ScriptEngine::GetSceneContext()->TryGetEntityWithUUID(inBody1.GetUserData());
		Entity entity2 = ScriptEngine::GetSceneContext()->TryGetEntityWithUUID(inBody2.GetUserData());

		if (!entity1 || !entity2)
			return;

		m_ContactEventCallback(ContactType::CollisionBegin, entity1, entity2);
	}

	void JoltContactListener::OnCollisionEnd(const JPH::Body& inBody1, const JPH::Body& inBody2)
	{
		Entity entity1 = ScriptEngine::GetSceneContext()->TryGetEntityWithUUID(inBody1.GetUserData());
		Entity entity2 = ScriptEngine::GetSceneContext()->TryGetEntityWithUUID(inBody2.GetUserData());

		if (!entity1 || !entity2)
			return;

		m_ContactEventCallback(ContactType::CollisionEnd, entity1, entity2);
	}

	void JoltContactListener::OnTriggerBegin(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		Entity entity1 = ScriptEngine::GetSceneContext()->TryGetEntityWithUUID(inBody1.GetUserData());
		Entity entity2 = ScriptEngine::GetSceneContext()->TryGetEntityWithUUID(inBody2.GetUserData());

		if (!entity1 || !entity2)
			return;

		m_ContactEventCallback(ContactType::TriggerBegin, entity1, entity2);
	}

	void JoltContactListener::OnTriggerEnd(const JPH::Body& inBody1, const JPH::Body& inBody2)
	{
		Entity entity1 = ScriptEngine::GetSceneContext()->TryGetEntityWithUUID(inBody1.GetUserData());
		Entity entity2 = ScriptEngine::GetSceneContext()->TryGetEntityWithUUID(inBody2.GetUserData());

		if (!entity1 || !entity2)
			return;

		m_ContactEventCallback(ContactType::TriggerEnd, entity1, entity2);
	}

	void JoltContactListener::OverrideFrictionAndRestitution(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		float friction1, restitution1, friction2, restitution2;
		GetFrictionAndRestitution(inBody1, inManifold.mSubShapeID1, friction1, restitution1);
		GetFrictionAndRestitution(inBody2, inManifold.mSubShapeID2, friction2, restitution2);

		ioSettings.mCombinedFriction = friction1 * friction2;
		ioSettings.mCombinedRestitution = glm::max(restitution1, restitution2);
	}

	void JoltContactListener::GetFrictionAndRestitution(const JPH::Body& inBody, const JPH::SubShapeID& inSubShapeID, float& outFriction, float& outRestitution) const
	{
		const JPH::PhysicsMaterial* material = inBody.GetShape()->GetMaterial(inSubShapeID);

		if (material == JPH::PhysicsMaterial::sDefault)
		{
			outFriction = inBody.GetFriction();
			outRestitution = inBody.GetRestitution();
		}
		else
		{
			// Hazel Material
			const JoltMaterial* customMaterial = static_cast<const JoltMaterial*>(material);
			outFriction = customMaterial->GetFriction();
			outRestitution = customMaterial->GetRestitution();
		}
	}

}

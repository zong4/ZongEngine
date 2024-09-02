#include "hzpch.h"
#include "JoltContactListener.h"
#include "JoltMaterial.h"

#include "Hazel/Script/ScriptEngine.h"

namespace Hazel {

	void JoltContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();

		if (!scene->IsPlaying())
			return;

		OverrideFrictionAndRestitution(inBody1, inBody2, inManifold, ioSettings);

		if (ioSettings.mIsSensor)
			OnTriggerBegin(scene, inBody1, inBody2, inManifold, ioSettings);
		else
			OnCollisionBegin(scene, inBody1, inBody2, inManifold, ioSettings);
	}

	void JoltContactListener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		OverrideFrictionAndRestitution(inBody1, inBody2, inManifold, ioSettings);
	}

	void JoltContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
	{
		Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
		
		if (!scene->IsPlaying())
			return;

		JPH::Body* body1 = m_BodyLockInterface->TryGetBody(inSubShapePair.GetBody1ID());
		JPH::Body* body2 = m_BodyLockInterface->TryGetBody(inSubShapePair.GetBody2ID());

		if (body1 == nullptr || body2 == nullptr)
			return;

		if (body1->IsSensor() || body2->IsSensor())
		{
			OnTriggerEnd(scene, *body1, *body2);
		}
		else
		{
			OnCollisionEnd(scene, *body1, *body2);
		}
	}

	void JoltContactListener::OnCollisionBegin(Ref<Scene> scene, const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		m_ContactEventCallback(ContactType::CollisionBegin, inBody1.GetUserData(), inBody2.GetUserData());
	}

	void JoltContactListener::OnCollisionEnd(Ref<Scene> scene, const JPH::Body& inBody1, const JPH::Body& inBody2)
	{
		m_ContactEventCallback(ContactType::CollisionEnd, inBody1.GetUserData(), inBody2.GetUserData());
	}

	void JoltContactListener::OnTriggerBegin(Ref<Scene> scene, const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		m_ContactEventCallback(ContactType::TriggerBegin, inBody1.GetUserData(), inBody2.GetUserData());
	}

	void JoltContactListener::OnTriggerEnd(Ref<Scene> scene, const JPH::Body& inBody1, const JPH::Body& inBody2)
	{
		m_ContactEventCallback(ContactType::TriggerEnd, inBody1.GetUserData(), inBody2.GetUserData());
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

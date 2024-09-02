#pragma once

#include "Hazel/Physics/PhysicsContactCallback.h"

#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>

namespace Hazel {

	class Scene;

	class JoltContactListener : public JPH::ContactListener
	{
	public:
		JoltContactListener(const JPH::BodyLockInterfaceNoLock* bodyLockInterface, const ContactCallbackFn& contactCallback)
			: m_BodyLockInterface(bodyLockInterface), m_ContactEventCallback(contactCallback) { }

		/// Called after detecting a collision between a body pair, but before calling OnContactAdded and before adding the contact constraint.
		/// If the function returns false, the contact will not be added and any other contacts between this body pair will not be processed.
		/// This function will only be called once per PhysicsSystem::Update per body pair and may not be called again the next update
		/// if a contact persists and no new contact pairs between sub shapes are found.
		/// This is a rather expensive time to reject a contact point since a lot of the collision detection has happened already, make sure you
		/// filter out the majority of undesired body pairs through the ObjectLayerPairFilter that is registered on the PhysicsSystem.
		/// Note that this callback is called when all bodies are locked, so don't use any locking functions!
		/// The order of body 1 and 2 is undefined, but when one of the two bodies is dynamic it will be body 1
		virtual JPH::ValidateResult	OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::CollideShapeResult& inCollisionResult) { return JPH::ValidateResult::AcceptAllContactsForThisBodyPair; }

		/// Called whenever a new contact point is detected.
		/// Note that this callback is called when all bodies are locked, so don't use any locking functions!
		/// Body 1 and 2 will be sorted such that body 1 ID < body 2 ID, so body 1 may not be dynamic.
		/// Note that only active bodies will report contacts, as soon as a body goes to sleep the contacts between that body and all other
		/// bodies will receive an OnContactRemoved callback, if this is the case then Body::IsActive() will return false during the callback.
		/// When contacts are added, the constraint solver has not run yet, so the collision impulse is unknown at that point.
		/// The velocities of inBody1 and inBody2 are the velocities before the contact has been resolved, so you can use this to
		/// estimate the collision impulse to e.g. determine the volume of the impact sound to play.
		virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings);

		/// Called whenever a contact is detected that was also detected last update.
		/// Note that this callback is called when all bodies are locked, so don't use any locking functions!
		/// Body 1 and 2 will be sorted such that body 1 ID < body 2 ID, so body 1 may not be dynamic.
		virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings);

		/// Called whenever a contact was detected last update but is not detected anymore.
		/// Note that this callback is called when all bodies are locked, so don't use any locking functions!
		/// Note that we're using BodyID's since the bodies may have been removed at the time of callback.
		/// Body 1 and 2 will be sorted such that body 1 ID < body 2 ID, so body 1 may not be dynamic.
		virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair);

	private:
		void OnCollisionBegin(Ref<Scene> scene, const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings);
		void OnCollisionEnd(Ref<Scene> scene, const JPH::Body& inBody1, const JPH::Body& inBody2);
		void OnTriggerBegin(Ref<Scene> scene, const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings);
		void OnTriggerEnd(Ref<Scene> scene, const JPH::Body& inBody1, const JPH::Body& inBody2);

	private:
		void OverrideFrictionAndRestitution(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings);
		void GetFrictionAndRestitution(const JPH::Body& inBody, const JPH::SubShapeID& inSubShapeID, float& outFriction, float& outRestitution) const;

	private:
		const JPH::BodyLockInterfaceNoLock* m_BodyLockInterface = nullptr;
		ContactCallbackFn m_ContactEventCallback;
	};

}

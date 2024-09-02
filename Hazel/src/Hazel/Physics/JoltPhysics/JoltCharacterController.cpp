#include "hzpch.h"
#include "JoltCharacterController.h"
#include "JoltUtils.h"
#include "JoltAPI.h"
#include "JoltScene.h"
#include "JoltMaterial.h"

#include "Hazel/Physics/PhysicsSystem.h"
#include "Hazel/Script/ScriptEngine.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

#ifdef JPH_DEBUG_RENDERER
#include <Jolt/Renderer/DebugRenderer.h>
#endif

#include "magic_enum.hpp"
using namespace magic_enum::bitwise_operators;

namespace Hazel {

	JoltCharacterController::JoltCharacterController(Entity entity, const ContactCallbackFn& contactCallback)
		: m_Entity(entity), m_ContactEventCallback(contactCallback)
	{
		Create();
	}

	JoltCharacterController::~JoltCharacterController()
	{
		m_Controller->Release();
	}

	void JoltCharacterController::SetSlopeLimit(float slopeLimit)
	{
		m_Controller->SetMaxSlopeAngle(glm::radians(slopeLimit));
	}

	void JoltCharacterController::SetStepOffset(float stepOffset)
	{
		m_StepOffset = stepOffset;
	}

	void JoltCharacterController::SetTranslation(const glm::vec3& inTranslation)
	{
		m_Controller->SetPosition(JoltUtils::ToJoltVector(inTranslation));
	}

	void JoltCharacterController::SetRotation(const glm::quat& inRotation)
	{
		m_Controller->SetRotation(JoltUtils::ToJoltQuat(inRotation));
	}

	bool JoltCharacterController::IsGrounded() const
	{
		return m_Controller->IsSupported();
	}

	void JoltCharacterController::Move(const glm::vec3& displacement)
	{
		if (m_Controller->IsSupported() || m_ControlMovementInAir)
		{
			m_Displacement += JoltUtils::ToJoltVector(displacement);
		}
	}

	void JoltCharacterController::Rotate(const glm::quat& rotation)
	{
		// avoid quat multiplication if rotation is close to identity
		if ((m_Controller->IsSupported() || m_ControlRotationInAir) && fabs(rotation.w - 1.0f) > 0.000001)
		{
			m_Rotation = m_Rotation * JoltUtils::ToJoltQuat(rotation);
		}
	}

	void JoltCharacterController::Jump(float jumpPower)
	{
		m_JumpPower = jumpPower;
	}

	glm::vec3 JoltCharacterController::GetLinearVelocity() const
	{
		return m_Controller->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround ? glm::zero<glm::vec3>() : JoltUtils::FromJoltVector(m_Controller->GetLinearVelocity());
	}

	void JoltCharacterController::SetLinearVelocity(const glm::vec3& linearVelocity)
	{
		m_Controller->SetLinearVelocity(JoltUtils::ToJoltVector(linearVelocity));
	}

	void JoltCharacterController::PreSimulate(float deltaTime)
	{
		if (deltaTime <= 0.0f)
			return;

		m_DesiredVelocity = m_Displacement / deltaTime;
		m_Controller->SetRotation((m_Controller->GetRotation() * m_Rotation).Normalized()); // re-normalize to avoid accumulating errors
		m_AllowSliding = !m_Controller->IsSupported() || !m_Displacement.IsNearZero();

		m_Controller->UpdateGroundVelocity();

		JPH::Vec3 currentVerticalVelocity = JPH::Vec3(0, m_Controller->GetLinearVelocity().GetY(), 0);
		JPH::Vec3 groundVelocity = m_Controller->GetGroundVelocity();

		bool jumping = (currentVerticalVelocity.GetY() - groundVelocity.GetY()) >= 0.1f;

		JPH::Vec3 newVelocity;
		if (IsGravityEnabled())
		{
			if (m_Controller->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround && (!m_Controller->IsSlopeTooSteep(m_Controller->GetGroundNormal())))
			{
				// When grounded, acquire velocity of ground
				newVelocity = groundVelocity;

				// Jump
				if (m_JumpPower > 0.0f && !jumping)
				{
					newVelocity += JPH::Vec3(0, m_JumpPower, 0);
					m_JumpPower = 0.0f;
				}
			}
			else
			{
				newVelocity = currentVerticalVelocity;
			}

			// Add gravity
			newVelocity += JoltUtils::ToJoltVector(PhysicsSystem::GetSettings().Gravity) * deltaTime;
		}
		else
		{
			newVelocity = JPH::Vec3::sZero();
		}

		if (m_Controller->IsSupported() || m_ControlMovementInAir)
		{
			newVelocity += m_DesiredVelocity;
		}
		else
		{
			// preserve current horizontal velocity
			JPH::Vec3 currentHorizontalVelocity = m_Controller->GetLinearVelocity() - currentVerticalVelocity;
			newVelocity += currentHorizontalVelocity;
		}

		// Update the velocity
		m_Controller->SetLinearVelocity(newVelocity);

		// TODO (0x): Collider switch if, for example, player changes stance
	}


	void JoltCharacterController::Simulate(float deltaTime)
	{
		JPH::Vec3 gravity = JoltUtils::ToJoltVector(PhysicsSystem::GetSettings().Gravity);
		const auto& characterControllerComponent = m_Entity.GetComponent<CharacterControllerComponent>();

		auto broadPhaseLayerFilter = JoltScene::GetJoltSystem().GetDefaultBroadPhaseLayerFilter(JPH::ObjectLayer(characterControllerComponent.LayerID));
		auto layerFilter = JoltScene::GetJoltSystem().GetDefaultLayerFilter(JPH::ObjectLayer(characterControllerComponent.LayerID));
		auto* tempAllocator = static_cast<JoltAPI*>(PhysicsSystem::GetAPI())->GetTempAllocator();

#ifdef JPH_DEBUG_RENDERER
		m_Controller->GetShape()->Draw(JPH::DebugRenderer::sInstance, m_Controller->GetCenterOfMassTransform(), JPH::Vec3::sReplicate(1.0f), JPH::Color::sGreen, false, true);
#endif

		m_Controller->ExtendedUpdate(deltaTime, gravity, { .mWalkStairsStepUp = {0.0f , m_StepOffset, 0.0f }, .mWalkStairsStepForwardTest = m_Controller->GetShape()->GetInnerRadius() }, broadPhaseLayerFilter, layerFilter, {}, {}, *tempAllocator);
	}


	void JoltCharacterController::PostSimulate()
	{
		if (m_Controller->IsSupported() || m_ControlMovementInAir)
		{
			m_Displacement = JPH::Vec3::sZero();
		}
		if(m_Controller->IsSupported() || m_ControlRotationInAir) {
			m_Rotation = JPH::Quat::sIdentity();
		}

		// raise "end" events for bodies in m_TriggeredBodies or m_CollidedBodies lists that are no longer in the m_StillTriggeredBodies and m_StillCollidedBodies lists
		Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
		if (scene->IsPlaying())
		{
			for (const auto& bodyID : m_TriggeredBodies)
			{
				if (std::find(m_StillTriggeredBodies.begin(), m_StillTriggeredBodies.end(), bodyID) == m_StillTriggeredBodies.end())
				{
					UUID otherEntityID = JoltScene::GetBodyInterface().GetUserData(bodyID);
					m_ContactEventCallback(ContactType::TriggerEnd, m_Entity.GetUUID(), otherEntityID);
				}
			}

			for (const auto& bodyID : m_CollidedBodies)
			{
				if (std::find(m_StillCollidedBodies.begin(), m_StillCollidedBodies.end(), bodyID) == m_StillCollidedBodies.end())
				{
					UUID otherEntityID = JoltScene::GetBodyInterface().GetUserData(bodyID);
					m_ContactEventCallback(ContactType::CollisionEnd, m_Entity.GetUUID(), otherEntityID);
				}
			}

			std::swap(m_TriggeredBodies, m_StillTriggeredBodies);
			std::swap(m_CollidedBodies, m_StillCollidedBodies);

			m_StillTriggeredBodies.clear();
			m_StillCollidedBodies.clear();
		}
	}


	void JoltCharacterController::Create()
	{
		Ref<Scene> scene = Scene::GetScene(m_Entity.GetSceneUUID());
		HZ_CORE_VERIFY(scene, "No scene active?");

		const auto transformComponent = scene->GetWorldSpaceTransform(m_Entity);
		const auto& characterControllerComponent = m_Entity.GetComponent<CharacterControllerComponent>();

		m_CollisionLayer = characterControllerComponent.LayerID;

		JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
		settings->mMaxSlopeAngle = glm::radians(characterControllerComponent.SlopeLimitDeg);
		m_StepOffset = characterControllerComponent.StepOffset;

		if (m_Entity.HasComponent<BoxColliderComponent>())
		{
			m_Shape = BoxShape::Create(m_Entity, 100.0f);
		}
		else if (m_Entity.HasComponent<CapsuleColliderComponent>())
		{
			m_Shape = CapsuleShape::Create(m_Entity, 100.0f);
		}
		else if (m_Entity.HasComponent<SphereColliderComponent>())
		{
			m_Shape = SphereShape::Create(m_Entity, 100.0f);
		}

		settings->mShape = static_cast<JPH::Shape*>(m_Shape->GetNativeShape());

		m_Controller = new JPH::CharacterVirtual(settings, JoltUtils::ToJoltVector(transformComponent.Translation), JoltUtils::ToJoltQuat(transformComponent.GetRotation()), &JoltScene::GetJoltSystem());
		m_Controller->SetListener(this);
		m_HasGravity = !characterControllerComponent.DisableGravity;
		m_ControlMovementInAir = characterControllerComponent.ControlMovementInAir;
		m_ControlRotationInAir = characterControllerComponent.ControlRotationInAir;
	}


	void JoltCharacterController::HandleTrigger(Ref<Scene> scene, const JPH::BodyID bodyID2)
	{
		if (std::find(m_TriggeredBodies.begin(), m_TriggeredBodies.end(), bodyID2) == m_TriggeredBodies.end())
		{
			m_ContactEventCallback(ContactType::TriggerBegin, m_Entity.GetUUID(), JoltScene::GetBodyInterface(false).GetUserData(bodyID2));
		}
		m_StillTriggeredBodies.push_back(bodyID2);
	}


	void JoltCharacterController::HandleCollision(Ref<Scene> scene, const JPH::BodyID bodyID2)
	{
		if (std::find(m_CollidedBodies.begin(), m_CollidedBodies.end(), bodyID2) == m_CollidedBodies.end())
		{
			m_ContactEventCallback(ContactType::CollisionBegin, m_Entity.GetUUID(), JoltScene::GetBodyInterface(false).GetUserData(bodyID2));
		}
		m_StillCollidedBodies.push_back(bodyID2);
	}


	void JoltCharacterController::OnAdjustBodyVelocity(const JPH::CharacterVirtual* inCharacter, const JPH::Body& inBody2, JPH::Vec3& ioLinearVelocity, JPH::Vec3& ioAngularVelocity)
	{
		// TODO (0x): marshal this call out to game play, and get result back again as appropriate for contacted inBody2
	}


	bool JoltCharacterController::OnContactValidate(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2)
	{
		auto objectLayer = JoltScene::GetBodyInterface().GetObjectLayer(inBodyID2);
		const auto& characterLayer = PhysicsLayerManager::GetLayer(/*m_CharacterController->*/m_CollisionLayer);
		const auto& bodyLayer = PhysicsLayerManager::GetLayer(objectLayer);

		if (characterLayer.LayerID == bodyLayer.LayerID)
			return characterLayer.CollidesWithSelf;

		return characterLayer.CollidesWith & bodyLayer.BitValue;
	}


	void JoltCharacterController::OnContactAdded(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings)
	{
		m_CollisionFlags = ECollisionFlags::None;

		// for now, any dynamic body can push the character.  TODO (0x): add settings to control this
		bool isSensor = false;
		bool isStatic = true;
		JPH::BodyLockRead lock(JoltScene::GetBodyLockInterface(), inBodyID2);
		if (lock.Succeeded())
		{
			isSensor = lock.GetBody().IsSensor();
			isStatic = lock.GetBody().IsStatic();
		}

		ioSettings.mCanPushCharacter = !isSensor && !isStatic;

		if (inContactNormal.GetY() < 0.0f)
			m_CollisionFlags |= ECollisionFlags::Below;
		else if (inContactNormal.GetY() > 0.0f)
			m_CollisionFlags |= ECollisionFlags::Above;

		if (inContactNormal.GetX() != 0.0f || inContactNormal.GetZ() != 0.0f)
			m_CollisionFlags |= ECollisionFlags::Sides;

		// If we encounter an object that can push us, enable sliding
		if (ioSettings.mCanPushCharacter)
		{
			m_AllowSliding = true;
		}

		Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();

		if (!scene->IsPlaying())
			return;

		if (isSensor)
		{
			HandleTrigger(scene, inBodyID2);
		}
		else
		{
			HandleCollision(scene, inBodyID2);
		}
	}


	void JoltCharacterController::OnContactSolve(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::Vec3Arg inContactVelocity, const JPH::PhysicsMaterial* inContactMaterial, JPH::Vec3Arg inCharacterVelocity, JPH::Vec3& ioNewCharacterVelocity)
	{
		// Don't allow the player to slide down static not-too-steep surfaces unless they are actively moving
		if (!m_AllowSliding && inContactVelocity.IsNearZero() && !inCharacter->IsSlopeTooSteep(inContactNormal))
		{
			ioNewCharacterVelocity = JPH::Vec3::sZero();
		}
	}

}

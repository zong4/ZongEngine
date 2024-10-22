#include "hzpch.h"
#include "JoltCharacterController.h"
#include "JoltUtils.h"
#include "JoltAPI.h"
#include "JoltScene.h"
#include "JoltMaterial.h"

#include "Hazel/Physics/PhysicsSystem.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

#include "magic_enum.hpp"
using namespace magic_enum::bitwise_operators;

namespace Hazel {

	JoltCharacterController::JoltCharacterController(Entity entity)
		: m_Entity(entity), m_Listener(this)
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
		return m_Controller->GetGroundState() != JPH::CharacterBase::EGroundState::InAir;
	}

	void JoltCharacterController::Move(const glm::vec3& displacement)
	{
		m_Displacement += displacement;
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
		m_Displacement = { 0.0f, 0.0f, 0.0f };

		// Cancel movement in opposite direction of normal when sliding
		JPH::CharacterVirtual::EGroundState groundState = m_Controller->GetGroundState();
		JPH::Vec3 desiredVelocity = JoltUtils::ToJoltVector(m_DesiredVelocity);
		if (groundState == JPH::CharacterVirtual::EGroundState::OnSteepGround)
		{
			JPH::Vec3 normal = m_Controller->GetGroundNormal();
			normal.SetY(0.0f);
			float dot = normal.Dot(desiredVelocity);
			if (dot < 0.0f)
				desiredVelocity -= (dot * normal) / normal.LengthSq();
		}

		JPH::Vec3 currentVerticalVelocity = JPH::Vec3(0, m_Controller->GetLinearVelocity().GetY(), 0);
		JPH::Vec3 groundVelocity = m_Controller->GetGroundVelocity();

		bool isGrounded = groundState == JPH::CharacterVirtual::EGroundState::OnGround;
		bool notJumping = (currentVerticalVelocity.GetY() - groundVelocity.GetY()) < 0.1f;

		JPH::Vec3 newVelocity;
		if (isGrounded && notJumping)
		{
			// Assume velocity of ground when on ground
			newVelocity = groundVelocity;

			// Jump
			if (m_JumpPower > 0.0f)
			{
				newVelocity += JPH::Vec3(0, m_JumpPower, 0);
				m_JumpPower = 0.0f;
			}
		}
		else
		{
			newVelocity = currentVerticalVelocity;
		}

		// Gravity
		if (IsGravityEnabled())
			newVelocity += JoltUtils::ToJoltVector(PhysicsSystem::GetSettings().Gravity) * deltaTime;

		// Player input
		newVelocity += desiredVelocity;

		// Update the velocity
		m_Controller->SetLinearVelocity(newVelocity);
	}

	void JoltCharacterController::Simulate(float deltaTime)
	{
		JPH::Vec3 gravity = JoltUtils::ToJoltVector(PhysicsSystem::GetSettings().Gravity);
		const auto& characterControllerComponent = m_Entity.GetComponent<CharacterControllerComponent>();

		auto broadPhaseLayerFilter = JoltScene::GetJoltSystem().GetDefaultBroadPhaseLayerFilter(JPH::ObjectLayer(characterControllerComponent.LayerID));
		auto layerFilter = JoltScene::GetJoltSystem().GetDefaultLayerFilter(JPH::ObjectLayer(characterControllerComponent.LayerID));
		auto* tempAllocator = static_cast<JoltAPI*>(PhysicsSystem::GetAPI())->GetTempAllocator();

		m_Controller->ExtendedUpdate(deltaTime, gravity, { .mWalkStairsStepUp = {0.0f , m_StepOffset, 0.0f }, .mWalkStairsStepForwardTest = m_Controller->GetShape()->GetInnerRadius() }, broadPhaseLayerFilter, layerFilter, {}, {}, *tempAllocator);
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
		m_Controller->SetListener(&m_Listener);
		m_HasGravity = !characterControllerComponent.DisableGravity;
	}

	bool JoltCharacterController::Listener::OnContactValidate(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2)
	{
		auto objectLayer = JoltScene::GetBodyInterface().GetObjectLayer(inBodyID2);
		const auto& characterLayer = PhysicsLayerManager::GetLayer(m_CharacterController->m_CollisionLayer);
		const auto& bodyLayer = PhysicsLayerManager::GetLayer(objectLayer);

		if (characterLayer.LayerID == bodyLayer.LayerID)
			return characterLayer.CollidesWithSelf;

		return characterLayer.CollidesWith & bodyLayer.BitValue;
	}

	void JoltCharacterController::Listener::OnContactAdded(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings)
	{
		m_CharacterController->m_CollisionFlags = ECollisionFlags::None;

		ioSettings.mCanPushCharacter = false;

		if (inContactNormal.GetY() < 0.0f)
			m_CharacterController->m_CollisionFlags |= ECollisionFlags::Below;
		else if (inContactNormal.GetY() > 0.0f)
			m_CharacterController->m_CollisionFlags |= ECollisionFlags::Above;

		if (inContactNormal.GetX() != 0.0f || inContactNormal.GetZ() != 0.0f)
			m_CharacterController->m_CollisionFlags |= ECollisionFlags::Sides;
	}

}

#pragma once

#include "Engine/Asset/Asset.h"
#include "Engine/Scene/Components.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Physics/PhysicsTypes.h"
#include "Engine/Core/Input.h"

#include <glm/glm.hpp>

extern "C" {
	typedef struct _MonoString MonoString;
	typedef struct _MonoArray MonoArray;
	typedef struct _MonoReflectionType MonoReflectionType;
}

namespace Hazel::Audio
{
	class CommandID;
	struct Transform;
}

namespace Hazel {

	// Forward declarations
	class Noise;

	class ScriptGlue
	{
	public:
		static void RegisterGlue();

	private:
		static void RegisterComponentTypes();
		static void RegisterInternalCalls();
	};

	namespace InternalCalls {

#pragma region AssetHandle

		bool AssetHandle_IsValid(AssetHandle* assetHandle);

#pragma endregion

#pragma region Application

		void Application_Quit();
		uint32_t Application_GetWidth();
		uint32_t Application_GetHeight();
		MonoString* Application_GetDataDirectoryPath();
		MonoString* Application_GetSetting(MonoString* name, MonoString* defaultValue);
		int Application_GetSettingInt(MonoString* name, int defaultValue);
		float Application_GetSettingFloat(MonoString* name, float defaultValue);

#pragma endregion

#pragma region SceneManager

		bool SceneManager_IsSceneValid(MonoString* inScene);
		bool SceneManager_IsSceneIDValid(uint64_t sceneID);
		void SceneManager_LoadScene(AssetHandle* sceneHandle);
		void SceneManager_LoadSceneByID(uint64_t sceneID);
		uint64_t SceneManager_GetCurrentSceneID();
		MonoString* SceneManager_GetCurrentSceneName();

#pragma endregion

#pragma region Scene

		uint64_t Scene_FindEntityByTag(MonoString* tag);
		bool Scene_IsEntityValid(uint64_t entityID);
		uint64_t Scene_CreateEntity(MonoString* tag);
		uint64_t Scene_InstantiatePrefab(AssetHandle* prefabHandle);
		uint64_t Scene_InstantiatePrefabWithTranslation(AssetHandle* prefabHandle, glm::vec3* inTranslation);
		uint64_t Scene_InstantiatePrefabWithTransform(AssetHandle* prefabHandle, glm::vec3* inTranslation, glm::vec3* inRotation, glm::vec3* inScale);
		uint64_t Scene_InstantiateChildPrefabWithTranslation(uint64_t parentId, AssetHandle* prefabHandle, glm::vec3* inTranslation);
		uint64_t Scene_InstantiateChildPrefabWithTransform(uint64_t parentId, AssetHandle* prefabHandle, glm::vec3* inTranslation, glm::vec3* inRotation, glm::vec3* inScale);
		
		void Scene_DestroyEntity(uint64_t entityID);
		void Scene_DestroyAllChildren(uint64_t entityID);

		MonoArray* Scene_GetEntities();
		MonoArray* Scene_GetChildrenIDs(uint64_t entityID);
		
		void Scene_SetTimeScale(float timeScale);

#pragma endregion

#pragma region Entity

		uint64_t Entity_GetParent(uint64_t entityID);
		void Entity_SetParent(uint64_t entityID, uint64_t parentID);

		MonoArray* Entity_GetChildren(uint64_t entityID);

		void Entity_CreateComponent(uint64_t entityID, MonoReflectionType* componentType);
		bool Entity_HasComponent(uint64_t entityID, MonoReflectionType* componentType);
		bool Entity_RemoveComponent(uint64_t entityID, MonoReflectionType* componentType);

#pragma endregion

#pragma region TagComponent

		MonoString* TagComponent_GetTag(uint64_t entityID);
		void TagComponent_SetTag(uint64_t entityID, MonoString* inTag);

#pragma endregion

#pragma region TransformComponent

		struct Transform
		{
			glm::vec3 Translation = glm::vec3(0.0f);
			glm::vec3 Rotation = glm::vec3(0.0f);
			glm::vec3 Scale = glm::vec3(1.0f);
		};
		
		void TransformComponent_GetTransform(uint64_t entityID, Transform* outTransform);
		void TransformComponent_SetTransform(uint64_t entityID, Transform* inTransform);
		void TransformComponent_GetTranslation(uint64_t entityID, glm::vec3* outTranslation);
		void TransformComponent_SetTranslation(uint64_t entityID, glm::vec3* inTranslation);
		void TransformComponent_GetRotation(uint64_t entityID, glm::vec3* outRotation);
		void TransformComponent_SetRotation(uint64_t entityID, glm::vec3* inRotation);
		void TransformComponent_GetScale(uint64_t entityID, glm::vec3* outScale);
		void TransformComponent_SetScale(uint64_t entityID, glm::vec3* inScale);
		void TransformComponent_GetWorldSpaceTransform(uint64_t entityID, Transform* outTransform);
		void TransformComponent_GetTransformMatrix(uint64_t entityID, glm::mat4* outTransform);
		void TransformComponent_SetTransformMatrix(uint64_t entityID, glm::mat4* inTransform);
		void TransformComponent_SetRotationQuat(uint64_t entityID, glm::quat* inRotation);
		void TransformMultiply_Native(Transform* inA, Transform* inB, Transform* outResult);

#pragma endregion

#pragma region MeshComponent

		bool MeshComponent_GetMesh(uint64_t entityID, AssetHandle* outHandle);
		void MeshComponent_SetMesh(uint64_t entityID, AssetHandle* meshHandle);
		bool MeshComponent_HasMaterial(uint64_t entityID, int index);
		bool MeshComponent_GetMaterial(uint64_t entityID, int index, AssetHandle* outHandle);
		bool MeshComponent_GetIsRigged(uint64_t entityID);

#pragma endregion

#pragma region StaticMeshComponent

		bool StaticMeshComponent_GetMesh(uint64_t entityID, AssetHandle* outHandle);
		void StaticMeshComponent_SetMesh(uint64_t entityID, AssetHandle* meshHandle);
		bool StaticMeshComponent_HasMaterial(uint64_t entityID, int index);
		bool StaticMeshComponent_GetMaterial(uint64_t entityID, int index, AssetHandle* outHandle);
		void StaticMeshComponent_SetMaterial(uint64_t entityID, int index, uint64_t materialHandle);
		bool StaticMeshComponent_IsVisible(uint64_t entityID);
		void StaticMeshComponent_SetVisible(uint64_t entityID, bool visible);

#pragma endregion

#pragma region AnimationComponent
		uint32_t Identifier_Get(MonoString* inName);
		bool AnimationComponent_GetInputBool(uint64_t entityID, uint32_t inputID);
		void AnimationComponent_SetInputBool(uint64_t entityID, uint32_t inputId, bool value);
		int32_t AnimationComponent_GetInputInt(uint64_t entityID, uint32_t inputID);
		void AnimationComponent_SetInputInt(uint64_t entityID, uint32_t inputId, int32_t value);
		float AnimationComponent_GetInputFloat(uint64_t entityID, uint32_t inputID);
		void AnimationComponent_SetInputFloat(uint64_t entityID, uint32_t inputId, float value);
		void AnimationComponent_GetRootMotion(uint64_t entityID, Transform* outTransform);

#pragma endregion

#pragma region ScriptComponent

		MonoObject* ScriptComponent_GetInstance(uint64_t entityID);

#pragma endregion

#pragma region CameraComponent

		void CameraComponent_SetPerspective(uint64_t entityID, float inVerticalFOV, float inNearClip, float inFarClip);
		void CameraComponent_SetOrthographic(uint64_t entityID, float inSize, float inNearClip, float inFarClip);
		float CameraComponent_GetVerticalFOV(uint64_t entityID);
		void CameraComponent_SetVerticalFOV(uint64_t entityID, float verticalFOV);
		float CameraComponent_GetPerspectiveNearClip(uint64_t entityID);
		void CameraComponent_SetPerspectiveNearClip(uint64_t entityID, float inNearClip);
		float CameraComponent_GetPerspectiveFarClip(uint64_t entityID);
		void CameraComponent_SetPerspectiveFarClip(uint64_t entityID, float inFarClip);
		float CameraComponent_GetOrthographicSize(uint64_t entityID);
		void CameraComponent_SetOrthographicSize(uint64_t entityID, float inSize);
		float CameraComponent_GetOrthographicNearClip(uint64_t entityID);
		void CameraComponent_SetOrthographicNearClip(uint64_t entityID, float inNearClip);
		float CameraComponent_GetOrthographicFarClip(uint64_t entityID);
		void CameraComponent_SetOrthographicFarClip(uint64_t entityID, float inFarClip);
		CameraComponent::Type CameraComponent_GetProjectionType(uint64_t entityID);
		void CameraComponent_SetProjectionType(uint64_t entityID, CameraComponent::Type inType);
		bool CameraComponent_GetPrimary(uint64_t entityID);
		void CameraComponent_SetPrimary(uint64_t entityID, bool inValue);

#pragma endregion

#pragma region DirectionalLightComponent

		void DirectionalLightComponent_GetRadiance(uint64_t entityID, glm::vec3* outRadiance);
		void DirectionalLightComponent_SetRadiance(uint64_t entityID, glm::vec3* inRadiance);
		float DirectionalLightComponent_GetIntensity(uint64_t entityID);
		void DirectionalLightComponent_SetIntensity(uint64_t entityID, float intensity);
		bool DirectionalLightComponent_GetCastShadows(uint64_t entityID);
		void DirectionalLightComponent_SetCastShadows(uint64_t entityID, bool castShadows);
		bool DirectionalLightComponent_GetSoftShadows(uint64_t entityID);
		void DirectionalLightComponent_SetSoftShadows(uint64_t entityID, bool softShadows);
		float DirectionalLightComponent_GetLightSize(uint64_t entityID);
		void DirectionalLightComponent_SetLightSize(uint64_t entityID, float lightSize);

#pragma endregion

#pragma region PointLightComponent

		void PointLightComponent_GetRadiance(uint64_t entityID, glm::vec3* outRadiance);
		void PointLightComponent_SetRadiance(uint64_t entityID, glm::vec3* inRadiance);
		float PointLightComponent_GetIntensity(uint64_t entityID);
		void PointLightComponent_SetIntensity(uint64_t entityID, float intensity);
		float PointLightComponent_GetRadius(uint64_t entityID);
		void PointLightComponent_SetRadius(uint64_t entityID, float radius);
		float PointLightComponent_GetFalloff(uint64_t entityID);
		void PointLightComponent_SetFalloff(uint64_t entityID, float falloff);

#pragma endregion

#pragma region SpotLightComponent

		void SpotLightComponent_GetRadiance(uint64_t entityID, glm::vec3* outRadiance);
		void SpotLightComponent_SetRadiance(uint64_t entityID, glm::vec3* inRadiance);
		float SpotLightComponent_GetIntensity(uint64_t entityID);
		void SpotLightComponent_SetIntensity(uint64_t entityID, float intensity);
		float SpotLightComponent_GetRange(uint64_t entityID);
		void SpotLightComponent_SetRange(uint64_t entityID, float range);
		float SpotLightComponent_GetAngle(uint64_t entityID);
		void SpotLightComponent_SetAngle(uint64_t entityID, float angle);
		float SpotLightComponent_GetAngleAttenuation(uint64_t entityID);
		void SpotLightComponent_SetAngleAttenuation(uint64_t entityID, float angleAttenuation);
		float SpotLightComponent_GetFalloff(uint64_t entityID);
		void SpotLightComponent_SetFalloff(uint64_t entityID, float falloff);
		bool SpotLightComponent_GetCastsShadows(uint64_t entityID);
		void SpotLightComponent_SetCastsShadows(uint64_t entityID, bool castsShadows);
		bool SpotLightComponent_GetSoftShadows(uint64_t entityID);
		void SpotLightComponent_SetSoftShadows(uint64_t entityID, bool softShadows);
#pragma endregion

#pragma region SkyLightComponent

		float SkyLightComponent_GetIntensity(uint64_t entityID);
		void SkyLightComponent_SetIntensity(uint64_t entityID, float intensity);
		float SkyLightComponent_GetTurbidity(uint64_t entityID);
		void SkyLightComponent_SetTurbidity(uint64_t entityID, float turbidity);
		float SkyLightComponent_GetAzimuth(uint64_t entityID);
		void SkyLightComponent_SetAzimuth(uint64_t entityID, float azimuth);
		float SkyLightComponent_GetInclination(uint64_t entityID);
		void SkyLightComponent_SetInclination(uint64_t entityID, float inclination);

#pragma endregion

#pragma region SpriteRendererComponent

		void SpriteRendererComponent_GetColor(uint64_t entityID, glm::vec4 * outColor);
		void SpriteRendererComponent_SetColor(uint64_t entityID, glm::vec4 * inColor);
		float SpriteRendererComponent_GetTilingFactor(uint64_t entityID);
		void SpriteRendererComponent_SetTilingFactor(uint64_t entityID, float tilingFactor);
		void SpriteRendererComponent_GetUVStart(uint64_t entityID, glm::vec2 * outUVStart);
		void SpriteRendererComponent_SetUVStart(uint64_t entityID, glm::vec2 * inUVStart);
		void SpriteRendererComponent_GetUVEnd(uint64_t entityID, glm::vec2 * outUVEnd);
		void SpriteRendererComponent_SetUVEnd(uint64_t entityID, glm::vec2 * inUVEnd);
		
#pragma endregion

#pragma region RigidBody2DComponent

		RigidBody2DComponent::Type RigidBody2DComponent_GetBodyType(uint64_t entityID);
		void RigidBody2DComponent_SetBodyType(uint64_t entityID, RigidBody2DComponent::Type inType);

		void RigidBody2DComponent_GetTranslation(uint64_t entityID, glm::vec2* outTranslation);
		void RigidBody2DComponent_SetTranslation(uint64_t entityID, glm::vec2* inTranslation);
		float RigidBody2DComponent_GetRotation(uint64_t entityID);
		void RigidBody2DComponent_SetRotation(uint64_t entityID, float rotation);

		float RigidBody2DComponent_GetMass(uint64_t entityID);
		void RigidBody2DComponent_SetMass(uint64_t entityID, float mass);
		void RigidBody2DComponent_GetLinearVelocity(uint64_t entityID, glm::vec2* outVelocity);
		void RigidBody2DComponent_SetLinearVelocity(uint64_t entityID, glm::vec2* inVelocity);
		float RigidBody2DComponent_GetGravityScale(uint64_t entityID);
		void RigidBody2DComponent_SetGravityScale(uint64_t entityID, float gravityScale);
		void RigidBody2DComponent_ApplyLinearImpulse(uint64_t entityID, glm::vec2* inImpulse, glm::vec2* inOffset, bool wake);
		void RigidBody2DComponent_ApplyAngularImpulse(uint64_t entityID, float impulse, bool wake);
		void RigidBody2DComponent_AddForce(uint64_t entityID, glm::vec3* inForce, glm::vec3* inOffset, bool wake);
		void RigidBody2DComponent_AddTorque(uint64_t entityID, float torque, bool wake);

#pragma endregion

#pragma region RigidBodyComponent

		void RigidBodyComponent_AddForce(uint64_t entityID, glm::vec3* inForce, EForceMode forceMode);
		void RigidBodyComponent_AddForceAtLocation(uint64_t entityID, glm::vec3* inForce, glm::vec3* inLocation, EForceMode forceMode);
		void RigidBodyComponent_AddTorque(uint64_t entityID, glm::vec3* inTorque, EForceMode forceMode);
		void RigidBodyComponent_GetLinearVelocity(uint64_t entityID, glm::vec3* outVelocity);
		void RigidBodyComponent_SetLinearVelocity(uint64_t entityID, glm::vec3* inVelocity);
		void RigidBodyComponent_GetAngularVelocity(uint64_t entityID, glm::vec3* outVelocity);
		void RigidBodyComponent_SetAngularVelocity(uint64_t entityID, glm::vec3* inVelocity);
		float RigidBodyComponent_GetMaxLinearVelocity(uint64_t entityID);
		void RigidBodyComponent_SetMaxLinearVelocity(uint64_t entityID, float maxVelocity);
		float RigidBodyComponent_GetMaxAngularVelocity(uint64_t entityID);
		void RigidBodyComponent_SetMaxAngularVelocity(uint64_t entityID, float maxVelocity);
		float RigidBodyComponent_GetLinearDrag(uint64_t entityID);
		void RigidBodyComponent_SetLinearDrag(uint64_t entityID, float linearDrag);
		float RigidBodyComponent_GetAngularDrag(uint64_t entityID);
		void RigidBodyComponent_SetAngularDrag(uint64_t entityID, float angularDrag);
		void RigidBodyComponent_Rotate(uint64_t entityID, glm::vec3* inRotation);
		uint32_t RigidBodyComponent_GetLayer(uint64_t entityID);
		void RigidBodyComponent_SetLayer(uint64_t entityID, uint32_t layerID);
		MonoString* RigidBodyComponent_GetLayerName(uint64_t entityID);
		void RigidBodyComponent_SetLayerByName(uint64_t entityID, MonoString* inName);
		float RigidBodyComponent_GetMass(uint64_t entityID);
		void RigidBodyComponent_SetMass(uint64_t entityID, float mass);
		EBodyType RigidBodyComponent_GetBodyType(uint64_t entityID);
		void RigidBodyComponent_SetBodyType(uint64_t entityID, EBodyType type);
		bool RigidBodyComponent_IsTrigger(uint64_t entityID);
		void RigidBodyComponent_SetTrigger(uint64_t entityID, bool isTrigger);
		void RigidBodyComponent_MoveKinematic(uint64_t entityID, glm::vec3* inTargetPosition, glm::vec3* inTargetRotation, float inDeltaSeconds);
		void RigidBodyComponent_SetAxisLock(uint64_t entityID, EActorAxis axis, bool value, bool forceWake);
		bool RigidBodyComponent_IsAxisLocked(uint64_t entityID, EActorAxis axis);
		uint32_t RigidBodyComponent_GetLockedAxes(uint64_t entityID);
		bool RigidBodyComponent_IsSleeping(uint64_t entityID);
		void RigidBodyComponent_SetIsSleeping(uint64_t entityID, bool isSleeping);
		void RigidBodyComponent_AddRadialImpulse(uint64_t entityID, glm::vec3* inOrigin, float radius, float strength, EFalloffMode falloff, bool velocityChange);
		void RigidBodyComponent_Teleport(uint64_t entityID, glm::vec3* inTargetPosition, glm::vec3* inTargetRotation, bool inForce);

#pragma endregion

#pragma region CharacterControllerComponent

		bool CharacterControllerComponent_GetIsGravityEnabled(uint64_t entityID);
		void CharacterControllerComponent_SetIsGravityEnabled(uint64_t entityID, bool isGravityEnabled);
		float CharacterControllerComponent_GetSlopeLimit(uint64_t entityID);
		void CharacterControllerComponent_SetSlopeLimit(uint64_t entityID, float slopeLimit);
		float CharacterControllerComponent_GetStepOffset(uint64_t entityID);
		void CharacterControllerComponent_SetStepOffset(uint64_t entityID, float stepOffset);
		void CharacterControllerComponent_SetTranslation(uint64_t entityID, glm::vec3* inPosition);
		void CharacterControllerComponent_SetRotation(uint64_t entityID, glm::quat* inRotation);
		void CharacterControllerComponent_Move(uint64_t entityID, glm::vec3* inDisplacement);
		void CharacterControllerComponent_Jump(uint64_t entityID, float jumpPower);
		void CharacterControllerComponent_GetLinearVelocity(uint64_t entityID, glm::vec3* outVelocity);
		void CharacterControllerComponent_SetLinearVelocity(uint64_t entityID, const glm::vec3& inVelocity);
		bool CharacterControllerComponent_IsGrounded(uint64_t entityID);
		ECollisionFlags CharacterControllerComponent_GetCollisionFlags(uint64_t entityID);

#pragma endregion

#pragma region FixedJointComponent

		uint64_t FixedJointComponent_GetConnectedEntity(uint64_t entityID);
		void FixedJointComponent_SetConnectedEntity(uint64_t entityID, uint64_t connectedEntityID);
		bool FixedJointComponent_IsBreakable(uint64_t entityID);
		void FixedJointComponent_SetIsBreakable(uint64_t entityID, bool isBreakable);
		bool FixedJointComponent_IsBroken(uint64_t entityID);
		void FixedJointComponent_Break(uint64_t entityID);
		float FixedJointComponent_GetBreakForce(uint64_t entityID);
		void FixedJointComponent_SetBreakForce(uint64_t entityID, float breakForce);
		float FixedJointComponent_GetBreakTorque(uint64_t entityID);
		void FixedJointComponent_SetBreakTorque(uint64_t entityID, float breakForce);
		bool FixedJointComponent_IsCollisionEnabled(uint64_t entityID);
		void FixedJointComponent_SetCollisionEnabled(uint64_t entityID, bool isCollisionEnabled);
		bool FixedJointComponent_IsPreProcessingEnabled(uint64_t entityID);
		void FixedJointComponent_SetPreProcessingEnabled(uint64_t entityID, bool isPreProcessingEnabled);

#pragma endregion

#pragma region BoxColliderComponent

		void BoxColliderComponent_GetHalfSize(uint64_t entityID, glm::vec3* outSize);
		void BoxColliderComponent_GetOffset(uint64_t entityID, glm::vec3* outOffset);
		bool BoxColliderComponent_GetMaterial(uint64_t entityID, ColliderMaterial* outMaterial);
		void BoxColliderComponent_SetMaterial(uint64_t entityID, ColliderMaterial* inMaterial);

#pragma endregion

#pragma region SphereColliderComponent

		float SphereColliderComponent_GetRadius(uint64_t entityID);
		void SphereColliderComponent_GetOffset(uint64_t entityID, glm::vec3* outOffset);
		bool SphereColliderComponent_GetMaterial(uint64_t entityID, ColliderMaterial* outMaterial);
		void SphereColliderComponent_SetMaterial(uint64_t entityID, ColliderMaterial* inMaterial);

#pragma endregion

#pragma region CapsuleColliderComponent

		float CapsuleColliderComponent_GetRadius(uint64_t entityID);
		float CapsuleColliderComponent_GetHeight(uint64_t entityID);
		void CapsuleColliderComponent_GetOffset(uint64_t entityID, glm::vec3* outOffset);
		bool CapsuleColliderComponent_GetMaterial(uint64_t entityID, ColliderMaterial* outMaterial);
		void CapsuleColliderComponent_SetMaterial(uint64_t entityID, ColliderMaterial* inMaterial);

#pragma endregion

#pragma region MeshColliderComponent

		bool MeshColliderComponent_IsMeshStatic(uint64_t entityID);
		bool MeshColliderComponent_IsColliderMeshValid(uint64_t entityID, AssetHandle* meshHandle);
		bool MeshColliderComponent_GetColliderMesh(uint64_t entityID, AssetHandle* outHandle);
		bool MeshColliderComponent_GetMaterial(uint64_t entityID, ColliderMaterial* outMaterial);
		void MeshColliderComponent_SetMaterial(uint64_t entityID, ColliderMaterial* inMaterial);

#pragma endregion

#pragma region MeshCollider

		bool MeshCollider_IsStaticMesh(AssetHandle* meshHandle);

#pragma endregion

#pragma region AudioComponent

		bool AudioComponent_IsPlaying(uint64_t entityID);
		bool AudioComponent_Play(uint64_t entityID, float startTime);
		bool AudioComponent_Stop(uint64_t entityID);
		bool AudioComponent_Pause(uint64_t entityID);
		bool AudioComponent_Resume(uint64_t entityID);
		float AudioComponent_GetVolumeMult(uint64_t entityID);
		void AudioComponent_SetVolumeMult(uint64_t entityID, float volumeMult);
		float AudioComponent_GetPitchMult(uint64_t entityID);
		void AudioComponent_SetPitchMult(uint64_t entityID, float pitchMult);
		void AudioComponent_SetEvent(uint64_t entityID, Audio::CommandID eventID);

#pragma endregion

#pragma region TextComponent

		size_t TextComponent_GetHash(uint64_t entityID);
		MonoString* TextComponent_GetText(uint64_t entityID);
		void TextComponent_SetText(uint64_t entityID, MonoString* text);
		void TextComponent_GetColor(uint64_t entityID, glm::vec4* outColor);
		void TextComponent_SetColor(uint64_t entityID, glm::vec4* inColor);

#pragma endregion

#pragma region Audio
		uint32_t Audio_PostEvent(Audio::CommandID eventID, uint64_t entityID);
		uint32_t Audio_PostEventFromAC(Audio::CommandID eventID, uint64_t entityID);
		uint32_t Audio_PostEventAtLocation(Audio::CommandID eventID, Transform* inLocation);
		bool Audio_StopEventID(uint32_t playingEventID);
		bool Audio_PauseEventID(uint32_t playingEventID);
		bool Audio_ResumeEventID(uint32_t playingEventID);
		uint64_t Audio_CreateAudioEntity(Audio::CommandID eventID, Transform* inLocation, float volume, float pitch);

		void Audio_PreloadEventSources(Audio::CommandID eventID);
		void Audio_UnloadEventSources(Audio::CommandID eventID);

		void Audio_SetLowPassFilterValue(uint64_t entityID, float value);
		void Audio_SetHighPassFilterValue(uint64_t entityID, float value);
		void Audio_SetLowPassFilterValue_Event(Audio::CommandID eventID, float value);
		void Audio_SetHighPassFilterValue_Event(Audio::CommandID eventID, float value);
		void Audio_SetLowPassFilterValue_AC(uint64_t entityID, float value);
		void Audio_SetHighPassFilterValue_AC(uint64_t entityID, float value);

#pragma endregion

#pragma region AudioCommandID
		
		uint32_t AudioCommandID_Constructor(MonoString* inCommandName);

#pragma endregion

#pragma region AudioParameters
		//============================================================================================
		/// Audio Parameters Interface
		void Audio_SetParameterFloat(Audio::CommandID parameterID, uint64_t entityID, float value);
		void Audio_SetParameterInt(Audio::CommandID parameterID, uint64_t entityID, int value);
		void Audio_SetParameterBool(Audio::CommandID parameterID, uint64_t entityID, bool value);
		void Audio_SetParameterFloatForAC(Audio::CommandID parameterID, uint64_t entityID, float value);
		void Audio_SetParameterIntForAC(Audio::CommandID parameterID, uint64_t entityID, int value);
		void Audio_SetParameterBoolForAC(Audio::CommandID parameterID, uint64_t entityID, bool value);

		void Audio_SetParameterFloatForEvent(Audio::CommandID parameterID, uint32_t eventID, float value);
		void Audio_SetParameterIntForEvent(Audio::CommandID parameterID, uint32_t eventID, int value);
		void Audio_SetParameterBoolForEvent(Audio::CommandID parameterID, uint32_t eventID, bool value);


#pragma endregion

#pragma region Texture2D

		bool Texture2D_Create(uint32_t width, uint32_t height, TextureWrap wrapMode, TextureFilter filterMode, AssetHandle* outHandle);
		void Texture2D_GetSize(AssetHandle* inHandle, uint32_t* outWidth, uint32_t* outHeight);
		void Texture2D_SetData(AssetHandle* inHandle, MonoArray* inData);
		//MonoArray* Texture2D_GetData(AssetHandle* inHandle);

#pragma endregion

#pragma region Mesh

		bool Mesh_GetMaterialByIndex(AssetHandle* meshHandle, int index, AssetHandle* outHandle);
		int Mesh_GetMaterialCount(AssetHandle* meshHandle);

#pragma endregion

#pragma region StaticMesh

		bool StaticMesh_GetMaterialByIndex(AssetHandle* meshHandle, int index, AssetHandle* outHandle);
		int StaticMesh_GetMaterialCount(AssetHandle* meshHandle);

#pragma endregion

#pragma region Material

		void Material_GetAlbedoColor(uint64_t entityID, AssetHandle* meshHandle, AssetHandle* materialHandle, glm::vec3* outAlbedoColor);
		void Material_SetAlbedoColor(uint64_t entityID, AssetHandle* meshHandle, AssetHandle* materialHandle, glm::vec3* inAlbedoColor);
		float Material_GetMetalness(uint64_t entityID, AssetHandle* meshHandle, AssetHandle* materialHandle);
		void Material_SetMetalness(uint64_t entityID, AssetHandle* meshHandle, AssetHandle* materialHandle, float inMetalness);
		float Material_GetRoughness(uint64_t entityID, AssetHandle* meshHandle, AssetHandle* materialHandle);
		void Material_SetRoughness(uint64_t entityID, AssetHandle* meshHandle, AssetHandle* materialHandle, float inRoughness);
		float Material_GetEmission(uint64_t entityID, AssetHandle* meshHandle, AssetHandle* materialHandle);
		void Material_SetEmission(uint64_t entityID, AssetHandle* meshHandle, AssetHandle* materialHandle, float inEmission);

		void Material_SetFloat(uint64_t entityID, AssetHandle* meshHandle, AssetHandle* materialHandle, MonoString* inUniform, float value);
		void Material_SetVector3(uint64_t entityID, AssetHandle* meshHandle, AssetHandle* materialHandle, MonoString* inUniform, glm::vec3* inValue);
		void Material_SetVector4(uint64_t entityID, AssetHandle* meshHandle, AssetHandle* materialHandle, MonoString* inUniform, glm::vec3* inValue);
		void Material_SetTexture(uint64_t entityID, AssetHandle* meshHandle, AssetHandle* materialHandle, MonoString* inUniform, AssetHandle* inTexture);

#pragma endregion

#pragma region MeshFactory

		void* MeshFactory_CreatePlane(float width, float height);

#pragma endregion

#pragma region Physics

		struct RaycastData
		{
			glm::vec3 Origin;
			glm::vec3 Direction;
			float MaxDistance;
			MonoArray* RequiredComponentTypes;
			MonoArray* ExcludeEntities;
		};

		struct ShapeQueryData
		{
			glm::vec3 Origin;
			glm::vec3 Direction;
			float MaxDistance;
			MonoObject* ShapeDataInstance;
			MonoArray* RequiredComponentTypes;
			MonoArray* ExcludeEntities;
		};

		struct RaycastData2D
		{
			glm::vec2 Origin;
			glm::vec2 Direction;
			float MaxDistance;
			MonoArray* RequiredComponentTypes;
		};

		struct ScriptRaycastHit
		{
			uint64_t HitEntity = 0;
			glm::vec3 Position = glm::vec3(0.0f);
			glm::vec3 Normal = glm::vec3(0.0f);
			float Distance = 0.0f;
			MonoObject* HitCollider = nullptr;
		};

		bool Physics_CastRay(RaycastData* inRaycastData, ScriptRaycastHit* outHit);
		bool Physics_CastShape(ShapeQueryData* inShapeQueryData, ScriptRaycastHit* outHit);
		int32_t Physics_OverlapShape(ShapeQueryData* inOverlapData, MonoArray** outHits);

		MonoArray* Physics_Raycast2D(RaycastData2D* inRaycastData);

		void Physics_GetGravity(glm::vec3* outGravity);
		void Physics_SetGravity(glm::vec3* inGravity);

		void Physics_AddRadialImpulse(glm::vec3* inOrigin, float radius, float strength, EFalloffMode falloff, bool velocityChange);

#pragma endregion

#pragma region Noise

		Noise* Noise_Constructor(int seed);
		void Noise_Destructor(Noise* _this);

		float Noise_GetFrequency(Noise* _this);
		void Noise_SetFrequency(Noise* _this, float frequency);

		int Noise_GetFractalOctaves(Noise* _this);
		void Noise_SetFractalOctaves(Noise* _this, int octaves);

		float Noise_GetFractalLacunarity(Noise* _this);
		void Noise_SetFractalLacunarity(Noise* _this, float lacunarity);

		float Noise_GetFractalGain(Noise* _this);
		void Noise_SetFractalGain(Noise* _this, float gain);

		float Noise_Get(Noise* _this, float x, float y);

		void Noise_SetSeed(int seed);
		float Noise_Perlin(float x, float y);

#pragma endregion

#pragma region Matrix4
		void Matrix4_LookAt(glm::vec3* eye, glm::vec3* center, glm::vec3* up, glm::mat4* outMatrix);
#pragma endregion

#pragma region Log

		enum class LogLevel : int32_t
		{
			Trace = BIT(0),
			Debug = BIT(1),
			Info = BIT(2),
			Warn = BIT(3),
			Error = BIT(4),
			Critical = BIT(5)
		};


		void Log_LogMessage(LogLevel level, MonoString* inFormattedMessage);

#pragma endregion

#pragma region Input

		bool Input_IsKeyPressed(KeyCode keycode);
		bool Input_IsKeyHeld(KeyCode keycode);
		bool Input_IsKeyDown(KeyCode keycode);
		bool Input_IsKeyReleased(KeyCode keycode);
		bool Input_IsMouseButtonPressed(MouseButton button);
		bool Input_IsMouseButtonHeld(MouseButton button);
		bool Input_IsMouseButtonDown(MouseButton button);
		bool Input_IsMouseButtonReleased(MouseButton button);
		void Input_GetMousePosition(glm::vec2* outPosition);
		void Input_SetCursorMode(CursorMode mode);
		CursorMode Input_GetCursorMode();
		bool Input_IsControllerPresent(int id);
		MonoArray* Input_GetConnectedControllerIDs();
		MonoString* Input_GetControllerName(int id);
		bool Input_IsControllerButtonPressed(int id, int button);
		bool Input_IsControllerButtonHeld(int id, int button);
		bool Input_IsControllerButtonDown(int id, int button);
		bool Input_IsControllerButtonReleased(int id, int button);
		float Input_GetControllerAxis(int id, int axis);
		uint8_t Input_GetControllerHat(int id, int hat);
		float Input_GetControllerDeadzone(int id, int axis);
		void Input_SetControllerDeadzone(int id, int axis, float deadzone);


#pragma endregion

#pragma region EditorUI

		void EditorUI_Text(MonoString* inText);
		bool EditorUI_Button(MonoString* inLabel, glm::vec2* inSize);
		bool EditorUI_BeginPropertyHeader(MonoString* label, bool openByDefault);
		void EditorUI_EndPropertyHeader();
		void EditorUI_PropertyGrid(bool inBegin);
		bool EditorUI_PropertyFloat(MonoString* inLabel, float* outValue);
		bool EditorUI_PropertyVec2(MonoString* inLabel, glm::vec2* outValue);
		bool EditorUI_PropertyVec3(MonoString* inLabel, glm::vec3* outValue);
		bool EditorUI_PropertyVec4(MonoString* inLabel, glm::vec4* outValue);

#pragma endregion

#pragma region SceneRenderer

		bool SceneRenderer_IsFogEnabled();
		void SceneRenderer_SetFogEnabled(bool enable);
		float SceneRenderer_GetOpacity();
		void SceneRenderer_SetOpacity(float opacity);

		bool SceneRenderer_DepthOfField_IsEnabled();
		void SceneRenderer_DepthOfField_SetEnabled(bool enabled);
		float SceneRenderer_DepthOfField_GetFocusDistance();
		void SceneRenderer_DepthOfField_SetFocusDistance(float focusDistance);
		float SceneRenderer_DepthOfField_GetBlurSize();
		void SceneRenderer_DepthOfField_SetBlurSize(float blurSize);

#pragma endregion

#pragma region DebugRenderer

		void DebugRenderer_DrawLine(glm::vec3* p0, glm::vec3* p1, glm::vec4* color);
		void DebugRenderer_DrawQuadBillboard(glm::vec3* translation, glm::vec2* size, glm::vec4* color);
		void DebugRenderer_SetLineWidth(float width);

#pragma endregion

#pragma region PerformanceTimers

		float PerformanceTimers_GetFrameTime();
		float PerformanceTimers_GetGPUTime();
		float PerformanceTimers_GetMainThreadWorkTime();
		float PerformanceTimers_GetMainThreadWaitTime();
		float PerformanceTimers_GetRenderThreadWorkTime();
		float PerformanceTimers_GetRenderThreadWaitTime();
		uint32_t PerformanceTimers_GetFramesPerSecond();
		uint32_t PerformanceTimers_GetEntityCount();
		uint32_t PerformanceTimers_GetScriptEntityCount();
		 
#pragma endregion

	}

}

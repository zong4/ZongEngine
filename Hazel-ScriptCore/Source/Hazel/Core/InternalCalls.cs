using Coral.Managed.Interop;
using System;

namespace Hazel
{
	internal static unsafe class InternalCalls
	{
#pragma warning disable CS0649 // Variable is never assigned to

		#region AssetHandle

		internal static delegate* unmanaged<AssetHandle, bool> AssetHandle_IsValid;

		#endregion

		#region Application

		internal static delegate* unmanaged<void> Application_Quit;
		internal static delegate* unmanaged<uint> Application_GetWidth;
		internal static delegate* unmanaged<uint> Application_GetHeight;
		internal static delegate* unmanaged<NativeString> Application_GetDataDirectoryPath;
		internal static delegate* unmanaged<NativeString, NativeString, NativeString> Application_GetSetting;
		internal static delegate* unmanaged<NativeString, int, int> Application_GetSettingInt;
		internal static delegate* unmanaged<NativeString, float, float> Application_GetSettingFloat;


		#endregion

		#region SceneManager

		internal static delegate* unmanaged<NativeString, bool> SceneManager_IsSceneValid;
		internal static delegate* unmanaged<ulong, bool> SceneManager_IsSceneIDValid;
		internal static delegate* unmanaged<AssetHandle, void> SceneManager_LoadScene;
		internal static delegate* unmanaged<ulong, void> SceneManager_LoadSceneByID;
		internal static delegate* unmanaged<ulong> SceneManager_GetCurrentSceneID;
		internal static delegate* unmanaged<NativeString> SceneManager_GetCurrentSceneName;

		#endregion

		#region Scene

		internal static delegate* unmanaged<NativeString, ulong> Scene_FindEntityByTag;
		internal static delegate* unmanaged<ulong, bool> Scene_IsEntityValid;
		internal static delegate* unmanaged<NativeString, ulong> Scene_CreateEntity;
		internal static delegate* unmanaged<AssetHandle, ulong> Scene_InstantiatePrefab;
		internal static delegate* unmanaged<AssetHandle, Vector3*, ulong> Scene_InstantiatePrefabWithTranslation;
		internal static delegate* unmanaged<ulong, AssetHandle, Vector3*, ulong> Scene_InstantiateChildPrefabWithTranslation;
		internal static delegate* unmanaged<ulong, AssetHandle, Vector3*, Vector3*, Vector3*, ulong> Scene_InstantiateChildPrefabWithTransform;
		internal static delegate* unmanaged<AssetHandle, Vector3*, Vector3*, Vector3*, ulong> Scene_InstantiatePrefabWithTransform;
		internal static delegate* unmanaged<ulong, void> Scene_DestroyEntity;
		internal static delegate* unmanaged<ulong, void> Scene_DestroyAllChildren;

		internal static delegate* unmanaged<NativeArray<ulong>> Scene_GetEntities;
		internal static delegate* unmanaged<ulong, NativeArray<ulong>> Scene_GetChildrenIDs;
		internal static delegate* unmanaged<float, void> Scene_SetTimeScale;

		#endregion

		#region Entity

		internal static delegate* unmanaged<ulong, ulong> Entity_GetParent;
		internal static delegate* unmanaged<ulong, ulong, void> Entity_SetParent;
		internal static delegate* unmanaged<ulong, NativeArray<ulong>> Entity_GetChildren;
		internal static delegate* unmanaged<ulong, ReflectionType, void> Entity_CreateComponent;
		internal static delegate* unmanaged<ulong, ReflectionType, bool> Entity_HasComponent;
		internal static delegate* unmanaged<ulong, ReflectionType, bool> Entity_RemoveComponent;

		#endregion

		#region TagComponent

		internal static delegate* unmanaged<ulong, NativeString> TagComponent_GetTag;
		internal static delegate* unmanaged<ulong, NativeString, void> TagComponent_SetTag;

		#endregion

		#region TransformComponent

		internal static delegate* unmanaged<ulong, Transform*, void> TransformComponent_GetTransform;
		internal static delegate* unmanaged<ulong, Transform*, void> TransformComponent_SetTransform;
		internal static delegate* unmanaged<ulong, Vector3*, void> TransformComponent_GetTranslation;
		internal static delegate* unmanaged<ulong, Vector3*, void> TransformComponent_SetTranslation;
		internal static delegate* unmanaged<ulong, Vector3*, void> TransformComponent_GetRotation;
		internal static delegate* unmanaged<ulong, Vector3*, void> TransformComponent_SetRotation;
		internal static delegate* unmanaged<ulong, Vector3*, void> TransformComponent_GetScale;
		internal static delegate* unmanaged<ulong, Vector3*, void> TransformComponent_SetScale;
		internal static delegate* unmanaged<ulong, Transform*, void> TransformComponent_GetWorldSpaceTransform;
		internal static delegate* unmanaged<ulong, Matrix4*, void> TransformComponent_GetTransformMatrix;
		internal static delegate* unmanaged<ulong, Matrix4*, void> TransformComponent_SetTransformMatrix;
		internal static delegate* unmanaged<ulong, Quaternion*, void> TransformComponent_GetRotationQuat;
		internal static delegate* unmanaged<ulong, Quaternion*, void> TransformComponent_SetRotationQuat;

		#endregion

		#region Transform

		internal static delegate* unmanaged<Transform*, Transform*, Transform*, void> TransformMultiply_Native;
		
		#endregion

		#region MeshComponent

		internal static delegate* unmanaged<ulong, AssetHandle*, bool> MeshComponent_GetMesh;
		internal static delegate* unmanaged<ulong, AssetHandle, void> MeshComponent_SetMesh;
		internal static delegate* unmanaged<ulong, bool> MeshComponent_GetVisible;
		internal static delegate* unmanaged<ulong, bool, void> MeshComponent_SetVisible;
		internal static delegate* unmanaged<ulong, int, bool> MeshComponent_HasMaterial;
		internal static delegate* unmanaged<ulong, int, AssetHandle*, bool> MeshComponent_GetMaterial;
		internal static delegate* unmanaged<ulong, bool> MeshComponent_GetIsRigged;

		#endregion

		#region StaticMeshComponent

		internal static delegate* unmanaged<ulong, AssetHandle*, bool> StaticMeshComponent_GetMesh;
		internal static delegate* unmanaged<ulong, AssetHandle, void> StaticMeshComponent_SetMesh;
		internal static delegate* unmanaged<ulong, int, bool> StaticMeshComponent_HasMaterial;
		internal static delegate* unmanaged<ulong, int, AssetHandle*, bool> StaticMeshComponent_GetMaterial;
		internal static delegate* unmanaged<ulong, int, AssetHandle, void> StaticMeshComponent_SetMaterial;
		internal static delegate* unmanaged<ulong, bool> StaticMeshComponent_GetVisible;
		internal static delegate* unmanaged<ulong, bool, void> StaticMeshComponent_SetVisible;

		#endregion

		#region AnimationComponent

		internal static delegate* unmanaged<NativeString, uint> Identifier_Get;
		internal static delegate* unmanaged<ulong, uint, bool> AnimationComponent_GetInputBool;
		internal static delegate* unmanaged<ulong, uint, bool, void> AnimationComponent_SetInputBool;
		internal static delegate* unmanaged<ulong, uint, int> AnimationComponent_GetInputInt;
		internal static delegate* unmanaged<ulong, uint, int, void> AnimationComponent_SetInputInt;
		internal static delegate* unmanaged<ulong, uint, float> AnimationComponent_GetInputFloat;
		internal static delegate* unmanaged<ulong, uint, float, void> AnimationComponent_SetInputFloat;
		internal static delegate* unmanaged<ulong, uint, Vector3*, void> AnimationComponent_GetInputVector3;
		internal static delegate* unmanaged<ulong, uint, Vector3*, void> AnimationComponent_SetInputVector3;
		internal static delegate* unmanaged<ulong, uint, void> AnimationComponent_SetInputTrigger;
		internal static delegate* unmanaged<ulong, Transform*, void> AnimationComponent_GetRootMotion;

		#endregion

		#region ScriptComponent

		internal static delegate* unmanaged<ulong, NativeInstance<object>> ScriptComponent_GetInstance;

		#endregion

		#region CameraComponent

		internal static delegate* unmanaged<ulong, float, float, float, void> CameraComponent_SetPerspective;
		internal static delegate* unmanaged<ulong, float, float, float, void> CameraComponent_SetOrthographic;
		internal static delegate* unmanaged<ulong, float> CameraComponent_GetVerticalFOV;
		internal static delegate* unmanaged<ulong, float, float> CameraComponent_SetVerticalFOV;
		internal static delegate* unmanaged<ulong, float> CameraComponent_GetPerspectiveNearClip;
		internal static delegate* unmanaged<ulong, float, void> CameraComponent_SetPerspectiveNearClip;
		internal static delegate* unmanaged<ulong, float> CameraComponent_GetPerspectiveFarClip;
		internal static delegate* unmanaged<ulong, float, void> CameraComponent_SetPerspectiveFarClip;
		internal static delegate* unmanaged<ulong, float> CameraComponent_GetOrthographicSize;
		internal static delegate* unmanaged<ulong, float, void> CameraComponent_SetOrthographicSize;
		internal static delegate* unmanaged<ulong, float> CameraComponent_GetOrthographicNearClip;
		internal static delegate* unmanaged<ulong, float, void> CameraComponent_SetOrthographicNearClip;
		internal static delegate* unmanaged<ulong, float> CameraComponent_GetOrthographicFarClip;
		internal static delegate* unmanaged<ulong, float, void> CameraComponent_SetOrthographicFarClip;
		internal static delegate* unmanaged<ulong, CameraComponentType> CameraComponent_GetProjectionType;
		internal static delegate* unmanaged<ulong, CameraComponentType, void> CameraComponent_SetProjectionType;
		internal static delegate* unmanaged<ulong, bool> CameraComponent_GetPrimary;
		internal static delegate* unmanaged<ulong, bool, void> CameraComponent_SetPrimary;
		internal static delegate* unmanaged<ulong, Vector3*, Vector2*, void> CameraComponent_ToScreenSpace;

		#endregion

		#region DirectionalLightComponent

		internal static delegate* unmanaged<ulong, Vector3*, void> DirectionalLightComponent_GetRadiance;
		internal static delegate* unmanaged<ulong, Vector3*, void> DirectionalLightComponent_SetRadiance;
		internal static delegate* unmanaged<ulong, float> DirectionalLightComponent_GetIntensity;
		internal static delegate* unmanaged<ulong, float, void> DirectionalLightComponent_SetIntensity;
		internal static delegate* unmanaged<ulong, bool> DirectionalLightComponent_GetCastShadows;
		internal static delegate* unmanaged<ulong, bool, void> DirectionalLightComponent_SetCastShadows;
		internal static delegate* unmanaged<ulong, bool> DirectionalLightComponent_GetSoftShadows;
		internal static delegate* unmanaged<ulong, bool, void> DirectionalLightComponent_SetSoftShadows;
		internal static delegate* unmanaged<ulong, float> DirectionalLightComponent_GetLightSize;
		internal static delegate* unmanaged<ulong, float, void> DirectionalLightComponent_SetLightSize;

		#endregion

		#region PointLightComponent

		internal static delegate* unmanaged<ulong, Vector3*, void> PointLightComponent_GetRadiance;
		internal static delegate* unmanaged<ulong, Vector3*, void> PointLightComponent_SetRadiance;
		internal static delegate* unmanaged<ulong, float> PointLightComponent_GetIntensity;
		internal static delegate* unmanaged<ulong, float, void> PointLightComponent_SetIntensity;
		internal static delegate* unmanaged<ulong, float> PointLightComponent_GetRadius;
		internal static delegate* unmanaged<ulong, float, void> PointLightComponent_SetRadius;
		internal static delegate* unmanaged<ulong, float> PointLightComponent_GetFalloff;
		internal static delegate* unmanaged<ulong, float, void> PointLightComponent_SetFalloff;

		#endregion

		#region SpotLightComponent

		internal static delegate* unmanaged<ulong, Vector3*, void> SpotLightComponent_GetRadiance;
		internal static delegate* unmanaged<ulong, Vector3*, void> SpotLightComponent_SetRadiance;
		internal static delegate* unmanaged<ulong, float> SpotLightComponent_GetIntensity;
		internal static delegate* unmanaged<ulong, float, void> SpotLightComponent_SetIntensity;
		internal static delegate* unmanaged<ulong, float> SpotLightComponent_GetRange;
		internal static delegate* unmanaged<ulong, float, void> SpotLightComponent_SetRange;
		internal static delegate* unmanaged<ulong, float> SpotLightComponent_GetAngle;
		internal static delegate* unmanaged<ulong, float, void> SpotLightComponent_SetAngle;
		internal static delegate* unmanaged<ulong, float> SpotLightComponent_GetAngleAttenuation;
		internal static delegate* unmanaged<ulong, float, void> SpotLightComponent_SetAngleAttenuation;
		internal static delegate* unmanaged<ulong, float> SpotLightComponent_GetFalloff;
		internal static delegate* unmanaged<ulong, float, void> SpotLightComponent_SetFalloff;
		internal static delegate* unmanaged<ulong, bool> SpotLightComponent_GetCastsShadows;
		internal static delegate* unmanaged<ulong, bool, void> SpotLightComponent_SetCastsShadows;
		internal static delegate* unmanaged<ulong, bool> SpotLightComponent_GetSoftShadows;
		internal static delegate* unmanaged<ulong, bool, void> SpotLightComponent_SetSoftShadows;

		#endregion
		
		#region SkyLightComponent

		internal static delegate* unmanaged<ulong, float> SkyLightComponent_GetIntensity;
		internal static delegate* unmanaged<ulong, float, void> SkyLightComponent_SetIntensity;
		internal static delegate* unmanaged<ulong, float> SkyLightComponent_GetTurbidity;
		internal static delegate* unmanaged<ulong, float, void> SkyLightComponent_SetTurbidity;
		internal static delegate* unmanaged<ulong, float> SkyLightComponent_GetAzimuth;
		internal static delegate* unmanaged<ulong, float, void> SkyLightComponent_SetAzimuth;
		internal static delegate* unmanaged<ulong, float> SkyLightComponent_GetInclination;
		internal static delegate* unmanaged<ulong, float, void> SkyLightComponent_SetInclination;

		#endregion

		#region SpriteRendererComponent

		internal static delegate* unmanaged<ulong, Vector4*, void> SpriteRendererComponent_GetColor;
		internal static delegate* unmanaged<ulong, Vector4*, void> SpriteRendererComponent_SetColor;
		internal static delegate* unmanaged<ulong, float> SpriteRendererComponent_GetTilingFactor;
		internal static delegate* unmanaged<ulong, float, void> SpriteRendererComponent_SetTilingFactor;
		internal static delegate* unmanaged<ulong, Vector2*, void> SpriteRendererComponent_GetUVStart;
		internal static delegate* unmanaged<ulong, Vector2*, void> SpriteRendererComponent_SetUVStart;
		internal static delegate* unmanaged<ulong, Vector2*, void> SpriteRendererComponent_GetUVEnd;
		internal static delegate* unmanaged<ulong, Vector2*, void> SpriteRendererComponent_SetUVEnd;

		#endregion

		#region RigidBody2DComponent

		internal static delegate* unmanaged<ulong, RigidBody2DBodyType> RigidBody2DComponent_GetBodyType;
		internal static delegate* unmanaged<ulong, RigidBody2DBodyType, void> RigidBody2DComponent_SetBodyType;
		internal static delegate* unmanaged<ulong, Vector2*, void> RigidBody2DComponent_GetTranslation;
		internal static delegate* unmanaged<ulong, Vector2*, void> RigidBody2DComponent_SetTranslation;
		internal static delegate* unmanaged<ulong, float> RigidBody2DComponent_GetRotation;
		internal static delegate* unmanaged<ulong, float, void> RigidBody2DComponent_SetRotation;
		internal static delegate* unmanaged<ulong, float> RigidBody2DComponent_GetMass;
		internal static delegate* unmanaged<ulong, float, void> RigidBody2DComponent_SetMass;
		internal static delegate* unmanaged<ulong, Vector2*, void> RigidBody2DComponent_GetLinearVelocity;
		internal static delegate* unmanaged<ulong, Vector2*, void> RigidBody2DComponent_SetLinearVelocity;
		internal static delegate* unmanaged<ulong, float> RigidBody2DComponent_GetGravityScale;
		internal static delegate* unmanaged<ulong, float, void> RigidBody2DComponent_SetGravityScale;
		internal static delegate* unmanaged<ulong, Vector2*, Vector2*, bool, void> RigidBody2DComponent_ApplyLinearImpulse;
		internal static delegate* unmanaged<ulong, float, bool, void> RigidBody2DComponent_ApplyAngularImpulse;
		internal static delegate* unmanaged<ulong, Vector2*, Vector2*, bool, void> RigidBody2DComponent_AddForce;
		internal static delegate* unmanaged<ulong, float, bool, void> RigidBody2DComponent_AddTorque;

		#endregion

		#region RigidBodyComponent

		internal static delegate* unmanaged<ulong, Vector3*, EForceMode, void> RigidBodyComponent_AddForce;
		internal static delegate* unmanaged<ulong, Vector3*, Vector3*, EForceMode, void> RigidBodyComponent_AddForceAtLocation;
		internal static delegate* unmanaged<ulong, Vector3*, EForceMode, void> RigidBodyComponent_AddTorque;
		internal static delegate* unmanaged<ulong, Vector3*, void> RigidBodyComponent_GetLinearVelocity;
		internal static delegate* unmanaged<ulong, Vector3*, void> RigidBodyComponent_SetLinearVelocity;
		internal static delegate* unmanaged<ulong, Vector3*, void> RigidBodyComponent_GetAngularVelocity;
		internal static delegate* unmanaged<ulong, Vector3*, void> RigidBodyComponent_SetAngularVelocity;
		internal static delegate* unmanaged<ulong, float> RigidBodyComponent_GetMaxLinearVelocity;
		internal static delegate* unmanaged<ulong, float, void> RigidBodyComponent_SetMaxLinearVelocity;
		internal static delegate* unmanaged<ulong, float> RigidBodyComponent_GetMaxAngularVelocity;
		internal static delegate* unmanaged<ulong, float, void> RigidBodyComponent_SetMaxAngularVelocity;
		internal static delegate* unmanaged<ulong, float> RigidBodyComponent_GetLinearDrag;
		internal static delegate* unmanaged<ulong, float, void> RigidBodyComponent_SetLinearDrag;
		internal static delegate* unmanaged<ulong, float> RigidBodyComponent_GetAngularDrag;
		internal static delegate* unmanaged<ulong, float, void> RigidBodyComponent_SetAngularDrag;
		internal static delegate* unmanaged<ulong, Vector3*, void> RigidBodyComponent_Rotate;
		internal static delegate* unmanaged<ulong, uint> RigidBodyComponent_GetLayer;
		internal static delegate* unmanaged<ulong, uint, void> RigidBodyComponent_SetLayer;
		internal static delegate* unmanaged<ulong, NativeString> RigidBodyComponent_GetLayerName;
		internal static delegate* unmanaged<ulong, NativeString, void> RigidBodyComponent_SetLayerByName;
		internal static delegate* unmanaged<ulong, float> RigidBodyComponent_GetMass;
		internal static delegate* unmanaged<ulong, float, void> RigidBodyComponent_SetMass;
		internal static delegate* unmanaged<ulong, EBodyType> RigidBodyComponent_GetBodyType;
		internal static delegate* unmanaged<ulong, EBodyType, void> RigidBodyComponent_SetBodyType;
		internal static delegate* unmanaged<ulong, bool> RigidBodyComponent_IsTrigger;
		internal static delegate* unmanaged<ulong, bool, void> RigidBodyComponent_SetTrigger;
		internal static delegate* unmanaged<ulong, Vector3*, Vector3*, float, void> RigidBodyComponent_MoveKinematic;
		internal static delegate* unmanaged<ulong, EActorAxis, bool, bool, void> RigidBodyComponent_SetAxisLock;
		internal static delegate* unmanaged<ulong, EActorAxis, bool> RigidBodyComponent_IsAxisLocked;
		internal static delegate* unmanaged<ulong, uint> RigidBodyComponent_GetLockedAxes;
		internal static delegate* unmanaged<ulong, bool> RigidBodyComponent_IsSleeping;
		internal static delegate* unmanaged<ulong, bool, void> RigidBodyComponent_SetIsSleeping;

		#endregion

		#region CharacterControllerComponent

		internal static delegate* unmanaged<ulong, bool> CharacterControllerComponent_GetIsGravityEnabled;
		internal static delegate* unmanaged<ulong, bool, bool> CharacterControllerComponent_SetIsGravityEnabled;
		internal static delegate* unmanaged<ulong, float> CharacterControllerComponent_GetSlopeLimit;
		internal static delegate* unmanaged<ulong, float, void> CharacterControllerComponent_SetSlopeLimit;
		internal static delegate* unmanaged<ulong, float> CharacterControllerComponent_GetStepOffset;
		internal static delegate* unmanaged<ulong, float, void> CharacterControllerComponent_SetStepOffset;
		internal static delegate* unmanaged<ulong, Vector3*, void> CharacterControllerComponent_SetTranslation;
		internal static delegate* unmanaged<ulong, Quaternion*, void> CharacterControllerComponent_SetRotation;
		internal static delegate* unmanaged<ulong, Vector3*, void> CharacterControllerComponent_Move;
		internal static delegate* unmanaged<ulong, Quaternion*, void> CharacterControllerComponent_Rotate;
		internal static delegate* unmanaged<ulong, float, void> CharacterControllerComponent_Jump;
		internal static delegate* unmanaged<ulong, Vector3*, void> CharacterControllerComponent_GetLinearVelocity;
		internal static delegate* unmanaged<ulong, Vector3*, void> CharacterControllerComponent_SetLinearVelocity;
		internal static delegate* unmanaged<ulong, bool> CharacterControllerComponent_IsGrounded;
		internal static delegate* unmanaged<ulong, ECollisionFlags> CharacterControllerComponent_GetCollisionFlags;

		#endregion

		#region BoxColliderComponent

		internal static delegate* unmanaged<ulong, Vector3*, void> BoxColliderComponent_GetHalfSize;
		internal static delegate* unmanaged<ulong, Vector3*, void> BoxColliderComponent_GetOffset;
		internal static delegate* unmanaged<ulong, PhysicsMaterial*, bool> BoxColliderComponent_GetMaterial;
		internal static delegate* unmanaged<ulong, PhysicsMaterial*, void> BoxColliderComponent_SetMaterial;

		#endregion

		#region SphereColliderComponent

		internal static delegate* unmanaged<ulong, float> SphereColliderComponent_GetRadius;
		internal static delegate* unmanaged<ulong, Vector3*, void> SphereColliderComponent_GetOffset;
		internal static delegate* unmanaged<ulong, PhysicsMaterial*, bool> SphereColliderComponent_GetMaterial;
		internal static delegate* unmanaged<ulong, PhysicsMaterial*, void> SphereColliderComponent_SetMaterial;

		#endregion

		#region CapsuleColliderComponent

		internal static delegate* unmanaged<ulong, float> CapsuleColliderComponent_GetRadius;
		internal static delegate* unmanaged<ulong, float> CapsuleColliderComponent_GetHeight;
		internal static delegate* unmanaged<ulong, Vector3*, void> CapsuleColliderComponent_GetOffset;
		internal static delegate* unmanaged<ulong, PhysicsMaterial*, bool> CapsuleColliderComponent_GetMaterial;
		internal static delegate* unmanaged<ulong, PhysicsMaterial*, void> CapsuleColliderComponent_SetMaterial;

		#endregion

		#region MeshColliderComponent

		internal static delegate* unmanaged<ulong, bool> MeshColliderComponent_IsMeshStatic;
		internal static delegate* unmanaged<ulong, AssetHandle*, bool> MeshColliderComponent_IsColliderMeshValid;
		internal static delegate* unmanaged<ulong, AssetHandle*, bool> MeshColliderComponent_GetColliderMesh;
		internal static delegate* unmanaged<ulong, PhysicsMaterial*, bool> MeshColliderComponent_GetMaterial;
		internal static delegate* unmanaged<ulong, PhysicsMaterial*, void> MeshColliderComponent_SetMaterial;

		#endregion

		#region MeshCollider

		internal static delegate* unmanaged<AssetHandle, bool> MeshCollider_IsStaticMesh;

		#endregion

		#region AudioComponent

		internal static delegate* unmanaged<ulong, bool> AudioComponent_IsPlaying;
		internal static delegate* unmanaged<ulong, float, bool> AudioComponent_Play;
		internal static delegate* unmanaged<ulong, bool> AudioComponent_Stop;
		internal static delegate* unmanaged<ulong, bool> AudioComponent_Pause;
		internal static delegate* unmanaged<ulong, bool> AudioComponent_Resume;
		internal static delegate* unmanaged<ulong, float> AudioComponent_GetVolumeMult;
		internal static delegate* unmanaged<ulong, float, void> AudioComponent_SetVolumeMult;
		internal static delegate* unmanaged<ulong, float> AudioComponent_GetPitchMult;
		internal static delegate* unmanaged<ulong, float, void> AudioComponent_SetPitchMult;
		internal static delegate* unmanaged<ulong, uint, void> AudioComponent_SetEvent;

		#endregion

		#region TextComponent

		internal static delegate* unmanaged<ulong, ulong> TextComponent_GetHash;
		internal static delegate* unmanaged<ulong, NativeString> TextComponent_GetText;
		internal static delegate* unmanaged<ulong, NativeString, void> TextComponent_SetText;
		internal static delegate* unmanaged<ulong, Vector4*, void> TextComponent_GetColor;
		internal static delegate* unmanaged<ulong, Vector4*, void> TextComponent_SetColor;

		#endregion

		#region Audio

		internal static delegate* unmanaged<ulong, bool> Audio_GetObjectInfo;
		internal static delegate* unmanaged<ulong, void> Audio_ReleaseAudioObject;
		internal static delegate* unmanaged<uint, ulong, uint> Audio_PostEvent;
		internal static delegate* unmanaged<uint, ulong, uint> Audio_PostEventFromAC;
		internal static delegate* unmanaged<uint, Transform*, uint> Audio_PostEventAtLocation;
		internal static delegate* unmanaged<uint, bool> Audio_StopEventID;
		internal static delegate* unmanaged<uint, bool> Audio_PauseEventID;
		internal static delegate* unmanaged<uint, bool> Audio_ResumeEventID;
		internal static delegate* unmanaged<uint, Transform*, float, float, ulong> Audio_CreateAudioEntity;

		#endregion

		#region AudioCommandID

		internal static delegate* unmanaged<NativeString, uint> AudioCommandID_Constructor;

		#endregion

		#region AudioObject

		internal static delegate* unmanaged<NativeString, Transform*, ulong> AudioObject_Constructor;
		internal static delegate* unmanaged<ulong, Transform*, void> AudioObject_SetTransform;
		internal static delegate* unmanaged<ulong, Transform*, void> AudioObject_GetTransform;

		//============================================================================================
		/// Audio Parameters Interface
		internal static delegate* unmanaged<uint, ulong, float, void> Audio_SetParameterFloat;
		internal static delegate* unmanaged<uint, ulong, float, void> Audio_SetParameterFloatForAC;
		internal static delegate* unmanaged<uint, ulong, int, void> Audio_SetParameterInt;
		internal static delegate* unmanaged<uint, ulong, int, void> Audio_SetParameterIntForAC;
		internal static delegate* unmanaged<uint, ulong, bool, void> Audio_SetParameterBool;
		internal static delegate* unmanaged<uint, ulong, bool, void> Audio_SetParameterBoolForAC;
		internal static delegate* unmanaged<uint, uint, float, void> Audio_SetParameterFloatForEvent;
		internal static delegate* unmanaged<uint, uint, int, void> Audio_SetParameterIntForEvent;
		internal static delegate* unmanaged<uint, uint, bool, void> Audio_SetParameterBoolForEvent;

		//============================================================================================
		internal static delegate* unmanaged<uint, void> Audio_PreloadEventSources;
		internal static delegate* unmanaged<uint, void> Audio_UnloadEventSources;
		internal static delegate* unmanaged<ulong, float, void> Audio_SetLowPassFilterValue;
		internal static delegate* unmanaged<ulong, float, void> Audio_SetHighPassFilterValue;
		internal static delegate* unmanaged<uint, float, void> Audio_SetLowPassFilterValue_Event;
		internal static delegate* unmanaged<uint, float, void> Audio_SetHighPassFilterValue_Event;
		internal static delegate* unmanaged<ulong, float, void> Audio_SetLowPassFilterValue_AC;
		internal static delegate* unmanaged<ulong, float, void> Audio_SetHighPassFilterValue_AC;

		#endregion

		#region Texture2D

		internal static delegate* unmanaged<uint, uint, TextureWrapMode, TextureFilterMode, AssetHandle*, bool> Texture2D_Create;
		internal static delegate* unmanaged<uint*, uint*, void> Texture2D_GetSize;
		internal static delegate* unmanaged<AssetHandle, NativeArray<Vector4>, void> Texture2D_SetData;

		#endregion

		#region Mesh

		internal static delegate* unmanaged<AssetHandle, int, AssetHandle*, bool> Mesh_GetMaterialByIndex;
		internal static delegate* unmanaged<AssetHandle, int> Mesh_GetMaterialCount;

		#endregion

		#region StaticMesh

		internal static delegate* unmanaged<AssetHandle, int, AssetHandle*, bool> StaticMesh_GetMaterialByIndex;
		internal static delegate* unmanaged<AssetHandle, int> StaticMesh_GetMaterialCount;

		#endregion

		#region Material

		internal static delegate* unmanaged<ulong, AssetHandle, AssetHandle, Vector3*, void> Material_GetAlbedoColor;
		internal static delegate* unmanaged<ulong, AssetHandle, AssetHandle, Vector3*, void> Material_SetAlbedoColor;
		internal static delegate* unmanaged<ulong, AssetHandle, AssetHandle, float> Material_GetMetalness;
		internal static delegate* unmanaged<ulong, AssetHandle, AssetHandle, float, void> Material_SetMetalness;
		internal static delegate* unmanaged<ulong, AssetHandle, AssetHandle, float> Material_GetRoughness;
		internal static delegate* unmanaged<ulong, AssetHandle, AssetHandle, float, void> Material_SetRoughness;
		internal static delegate* unmanaged<ulong, AssetHandle, AssetHandle, float> Material_GetEmission;
		internal static delegate* unmanaged<ulong, AssetHandle, AssetHandle, float, void> Material_SetEmission;

		internal static delegate* unmanaged<ulong, AssetHandle, AssetHandle, NativeString, float, void> Material_SetFloat;
		internal static delegate* unmanaged<ulong, AssetHandle, AssetHandle, NativeString, Vector3*, void> Material_SetVector3;
		internal static delegate* unmanaged<ulong, AssetHandle, AssetHandle, NativeString, Vector4*, void> Material_SetVector4;
		internal static delegate* unmanaged<ulong, AssetHandle, AssetHandle, NativeString, AssetHandle, void> Material_SetTexture;

		#endregion

		#region MeshFactory

		internal static delegate* unmanaged<float, float, IntPtr> MeshFactory_CreatePlane;

		#endregion

		#region Physics

		internal static delegate* unmanaged<RaycastData*, SceneQueryHit*, bool> Physics_CastRay;
		internal static delegate* unmanaged<ShapeQueryData*, SceneQueryHit*, bool> Physics_CastShape;
		internal static delegate* unmanaged<ShapeQueryData*, NativeArray<SceneQueryHit>*, int> Physics_OverlapShape;
		internal static delegate* unmanaged<RaycastData2D*, NativeArray<RaycastHit2D>> Physics_Raycast2D;
		internal static delegate* unmanaged<Vector3*, void> Physics_GetGravity;
		internal static delegate* unmanaged<Vector3*, void> Physics_SetGravity;
		internal static delegate* unmanaged<Vector3*, float, float, EFalloffMode, bool, void> Physics_AddRadialImpulse;

		#endregion

		#region Noise

		internal static delegate* unmanaged<int, IntPtr> Noise_Constructor;
		internal static delegate* unmanaged<IntPtr, void> Noise_Destructor;
		internal static delegate* unmanaged<IntPtr, float> Noise_GetFrequency;
		internal static delegate* unmanaged<IntPtr, float, void> Noise_SetFrequency;
		internal static delegate* unmanaged<IntPtr, int> Noise_GetFractalOctaves;
		internal static delegate* unmanaged<IntPtr, int, void> Noise_SetFractalOctaves;
		internal static delegate* unmanaged<IntPtr, float> Noise_GetFractalLacunarity;
		internal static delegate* unmanaged<IntPtr, float, void> Noise_SetFractalLacunarity;
		internal static delegate* unmanaged<IntPtr, float> Noise_GetFractalGain;
		internal static delegate* unmanaged<IntPtr, float, void> Noise_SetFractalGain;
		internal static delegate* unmanaged<IntPtr, float, float, float> Noise_Get;

		internal static delegate* unmanaged<int, void> Noise_SetSeed;
		internal static delegate* unmanaged<float, float, float> Noise_Perlin;

		#endregion

		#region Matrix4

		internal static delegate* unmanaged<Vector3*, Vector3*, Vector3*, Matrix4*, void> Matrix4_LookAt;

		#endregion

		#region Log

		internal static delegate* unmanaged<Log.LogLevel, NativeString, void> Log_LogMessage;

		#endregion

		#region Input

		internal static delegate* unmanaged<KeyCode, Bool32> Input_IsKeyPressed;
		internal static delegate* unmanaged<KeyCode, Bool32> Input_IsKeyHeld;
		internal static delegate* unmanaged<KeyCode, Bool32> Input_IsKeyDown;
		internal static delegate* unmanaged<KeyCode, Bool32> Input_IsKeyReleased;
		internal static delegate* unmanaged<MouseButton, bool> Input_IsMouseButtonPressed;
		internal static delegate* unmanaged<MouseButton, bool> Input_IsMouseButtonHeld;
		internal static delegate* unmanaged<MouseButton, bool> Input_IsMouseButtonDown;
		internal static delegate* unmanaged<MouseButton, bool> Input_IsMouseButtonReleased;
		internal static delegate* unmanaged<Vector2*, void> Input_GetMousePosition;
		internal static delegate* unmanaged<CursorMode, void> Input_SetCursorMode;
		internal static delegate* unmanaged<CursorMode> Input_GetCursorMode;

		internal static delegate* unmanaged<int, bool> Input_IsControllerPresent;
		internal static delegate* unmanaged<NativeArray<int>> Input_GetConnectedControllerIDs;
		internal static delegate* unmanaged<int, NativeString> Input_GetControllerName;
		internal static delegate* unmanaged<int, int, bool> Input_IsControllerButtonPressed;
		internal static delegate* unmanaged<int, int, bool> Input_IsControllerButtonHeld;
		internal static delegate* unmanaged<int, int, bool> Input_IsControllerButtonDown;
		internal static delegate* unmanaged<int, int, bool> Input_IsControllerButtonReleased;
		internal static delegate* unmanaged<int, int, float> Input_GetControllerAxis;
		internal static delegate* unmanaged<int, int, byte> Input_GetControllerHat;
		internal static delegate* unmanaged<int, int, float> Input_GetControllerDeadzone;
		internal static delegate* unmanaged<int, int, float, void> Input_SetControllerDeadzone;

		#endregion
		
		#region SceneRenderer

		internal static delegate* unmanaged<bool> SceneRenderer_IsFogEnabled;
		internal static delegate* unmanaged<bool, void> SceneRenderer_SetFogEnabled;

		internal static delegate* unmanaged<float> SceneRenderer_GetOpacity;
		internal static delegate* unmanaged<float, void> SceneRenderer_SetOpacity;

		internal static delegate* unmanaged<bool> SceneRenderer_DepthOfField_IsEnabled;
		internal static delegate* unmanaged<bool, void> SceneRenderer_DepthOfField_SetEnabled;
		internal static delegate* unmanaged<float> SceneRenderer_DepthOfField_GetFocusDistance;
		internal static delegate* unmanaged<float, void> SceneRenderer_DepthOfField_SetFocusDistance;
		internal static delegate* unmanaged<float> SceneRenderer_DepthOfField_GetBlurSize;
		internal static delegate* unmanaged<float, void> SceneRenderer_DepthOfField_SetBlurSize;

		#endregion

		#region DebugRenderer

		internal static delegate* unmanaged<Vector3*, Vector3*, Vector4*, void> DebugRenderer_DrawLine;
		internal static delegate* unmanaged<Vector3*, Vector2*, Vector4*, void> DebugRenderer_DrawQuadBillboard;
		internal static delegate* unmanaged<float, void> DebugRenderer_SetLineWidth;

		#endregion

		#region PerformanceTimers

		internal static delegate* unmanaged<float> PerformanceTimers_GetFrameTime;
		internal static delegate* unmanaged<float> PerformanceTimers_GetGPUTime;
		internal static delegate* unmanaged<float> PerformanceTimers_GetMainThreadWorkTime;
		internal static delegate* unmanaged<float> PerformanceTimers_GetMainThreadWaitTime;
		internal static delegate* unmanaged<float> PerformanceTimers_GetRenderThreadWorkTime;
		internal static delegate* unmanaged<float> PerformanceTimers_GetRenderThreadWaitTime;
		internal static delegate* unmanaged<uint> PerformanceTimers_GetFramesPerSecond;
		internal static delegate* unmanaged<uint> PerformanceTimers_GetEntityCount;
		internal static delegate* unmanaged<uint> PerformanceTimers_GetScriptEntityCount;

		#endregion

#pragma warning restore CS0649
	}
}

using System;
using System.Runtime.CompilerServices;

namespace Hazel
{
	internal static class InternalCalls
	{
		#region AssetHandle

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool AssetHandle_IsValid(ref AssetHandle assetHandle);

		#endregion

		#region Application

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Application_Quit();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern uint Application_GetWidth();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern uint Application_GetHeight();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern string Application_GetDataDirectoryPath();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern string Application_GetSetting(string name, string defaultValue);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern int Application_GetSettingInt(string name, int defaultValue);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float Application_GetSettingFloat(string name, float defaultValue);

		#endregion

		#region SceneManager

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool SceneManager_IsSceneValid(string scene);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool SceneManager_IsSceneIDValid(ulong sceneID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SceneManager_LoadScene(ref AssetHandle sceneHandle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SceneManager_LoadSceneByID(ulong sceneID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong SceneManager_GetCurrentSceneID();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern string SceneManager_GetCurrentSceneName();

		#endregion

		#region Scene

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Scene_FindEntityByTag(string tag);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Scene_IsEntityValid(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Scene_CreateEntity(string tag);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Scene_InstantiatePrefab(ref AssetHandle prefabHandle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Scene_InstantiatePrefabWithTranslation(ref AssetHandle prefabHandle, ref Vector3 translation);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Scene_InstantiateChildPrefabWithTranslation(ulong parentID, ref AssetHandle prefabHandle, ref Vector3 translation);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Scene_InstantiateChildPrefabWithTransform(ulong parentID, ref AssetHandle prefabHandle, ref Vector3 translation, ref Vector3 rotation, ref Vector3 scale);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Scene_InstantiatePrefabWithTransform(ref AssetHandle prefabHandle, ref Vector3 translation, ref Vector3 rotation, ref Vector3 scale);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Scene_DestroyEntity(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Scene_DestroyAllChildren(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Entity[] Scene_GetEntities();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong[] Scene_GetChildrenIDs(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Scene_SetTimeScale(float timeScale);

		#endregion

		#region Entity

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Entity_GetParent(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Entity_SetParent(ulong entityID, ulong parentID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Entity[] Entity_GetChildren(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Entity_CreateComponent(ulong entityID, Type type);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Entity_HasComponent(ulong entityID, Type type);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Entity_RemoveComponent(ulong entityID, Type type);

		#endregion

		#region TagComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern string TagComponent_GetTag(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TagComponent_SetTag(ulong entityID, string tag);

		#endregion

		#region TransformComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_GetTransform(ulong entityID, out Transform outTransform);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetTransform(ulong entityID, ref Transform inTransform);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_GetTranslation(ulong entityID, out Vector3 outTranslation);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetTranslation(ulong entityID, ref Vector3 inTranslation);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_GetRotation(ulong entityID, out Vector3 outRotation);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetRotation(ulong entityID, ref Vector3 inRotation);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_GetScale(ulong entityID, out Vector3 outScale);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetScale(ulong entityID, ref Vector3 inScale);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_GetWorldSpaceTransform(ulong entityID, out Transform outTransform);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_GetTransformMatrix(ulong entityID, out Matrix4 outTransformMatrix);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetTransformMatrix(ulong entityID, ref Matrix4 outTransformMatrix);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetRotationQuat(ulong entityID, ref Quaternion inRotation);

		#endregion

		#region Transform

		[MethodImpl(MethodImplOptions.InternalCall)]
		public static extern void TransformMultiply_Native(Transform a, Transform b, out Transform result);
		
		#endregion

		#region MeshComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool MeshComponent_GetMesh(ulong entityID, out AssetHandle outHandle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void MeshComponent_SetMesh(ulong entityID, ref AssetHandle meshHandle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool MeshComponent_HasMaterial(ulong entityID, int index);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool MeshComponent_GetMaterial(ulong entityID, int index, out AssetHandle outHandle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool MeshComponent_GetIsRigged(ulong entityID);

		#endregion

		#region StaticMeshComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool StaticMeshComponent_GetMesh(ulong entityID, out AssetHandle outHandle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void StaticMeshComponent_SetMesh(ulong entityID, ref AssetHandle meshHandle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool StaticMeshComponent_HasMaterial(ulong entityID, int index);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool StaticMeshComponent_GetMaterial(ulong entityID, int index, out AssetHandle outHandle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void StaticMeshComponent_SetMaterial(ulong entityID, int index, ulong handle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool StaticMeshComponent_IsVisible(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void StaticMeshComponent_SetVisible(ulong entityID, bool visible);

		#endregion

		#region AnimationComponent
		[MethodImpl(MethodImplOptions.InternalCall)]
		public static extern uint Identifier_Get(string name);
		[MethodImpl(MethodImplOptions.InternalCall)]
		public static extern bool AnimationComponent_GetInputBool(ulong entityID, uint inputID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		public static extern void AnimationComponent_SetInputBool(ulong entityID, uint inputID, bool value);
		[MethodImpl(MethodImplOptions.InternalCall)]
		public static extern int AnimationComponent_GetInputInt(ulong entityID, uint inputID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		public static extern void AnimationComponent_SetInputInt(ulong entityID, uint inputID, int value);
		[MethodImpl(MethodImplOptions.InternalCall)]
		public static extern float AnimationComponent_GetInputFloat(ulong entityID, uint inputID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		public static extern void AnimationComponent_SetInputFloat(ulong entityID, uint inputID, float value);
		[MethodImpl(MethodImplOptions.InternalCall)]
		public static extern void AnimationComponent_GetRootMotion(ulong entityID, out Transform result);

		#endregion

		#region ScriptComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern object ScriptComponent_GetInstance(ulong entityID);

		#endregion

		#region CameraComponent
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPerspective(ulong entityID, float verticalFOV, float nearClip, float farClip);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetOrthographic(ulong entityID, float size, float nearClip, float farClip);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetVerticalFOV(ulong entityID); 
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_SetVerticalFOV(ulong entityID, float verticalFOV);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetPerspectiveNearClip(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPerspectiveNearClip(ulong entityID, float nearClip);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetPerspectiveFarClip(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPerspectiveFarClip(ulong entityID, float farClip);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetOrthographicSize(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetOrthographicSize(ulong entityID, float size);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetOrthographicNearClip(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetOrthographicNearClip(ulong entityID, float nearClip);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetOrthographicFarClip(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetOrthographicFarClip(ulong entityID, float farClip);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern CameraComponentType CameraComponent_GetProjectionType(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetProjectionType(ulong entityID, CameraComponentType type);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_GetPrimary(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPrimary(ulong entityID, bool value);


		#endregion

		#region DirectionalLightComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void DirectionalLightComponent_GetRadiance(ulong entityID, out Vector3 outRadiance);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void DirectionalLightComponent_SetRadiance(ulong entityID, ref Vector3 inRadiance);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float DirectionalLightComponent_GetIntensity(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void DirectionalLightComponent_SetIntensity(ulong entityID, float intensity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool DirectionalLightComponent_GetCastShadows(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void DirectionalLightComponent_SetCastShadows(ulong entityID, bool castShadows);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool DirectionalLightComponent_GetSoftShadows(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void DirectionalLightComponent_SetSoftShadows(ulong entityID, bool softShadows);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float DirectionalLightComponent_GetLightSize(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void DirectionalLightComponent_SetLightSize(ulong entityID, float lightSize);

		#endregion

		#region PointLightComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void PointLightComponent_GetRadiance(ulong entityID, out Vector3 outRadiance);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void PointLightComponent_SetRadiance(ulong entityID, ref Vector3 inRadiance);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float PointLightComponent_GetIntensity(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void PointLightComponent_SetIntensity(ulong entityID, float intensity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float PointLightComponent_GetRadius(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void PointLightComponent_SetRadius(ulong entityID, float radius);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float PointLightComponent_GetFalloff(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void PointLightComponent_SetFalloff(ulong entityID, float falloff);

		#endregion

		#region SpotLightComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpotLightComponent_GetRadiance(ulong entityID, out Vector3 outRadiance);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpotLightComponent_SetRadiance(ulong entityID, ref Vector3 inRadiance);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SpotLightComponent_GetIntensity(ulong entityID);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpotLightComponent_SetIntensity(ulong entityID, float intensity);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SpotLightComponent_GetRange(ulong entityID);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpotLightComponent_SetRange(ulong entityID, float range);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SpotLightComponent_GetAngle(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpotLightComponent_SetAngle(ulong entityID, float angle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SpotLightComponent_GetAngleAttenuation(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpotLightComponent_SetAngleAttenuation(ulong entityID, float angleAttenuation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SpotLightComponent_GetFalloff(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpotLightComponent_SetFalloff(ulong entityID, float falloff);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool SpotLightComponent_GetCastsShadows(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpotLightComponent_SetCastsShadows(ulong entityID, bool falloff);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool SpotLightComponent_GetSoftShadows(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpotLightComponent_SetSoftShadows(ulong entityID, bool falloff);
		#endregion
		
		#region SkyLightComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SkyLightComponent_GetIntensity(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SkyLightComponent_SetIntensity(ulong entityID, float intensity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SkyLightComponent_GetTurbidity(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SkyLightComponent_SetTurbidity(ulong entityID, float turbidity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SkyLightComponent_GetAzimuth(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SkyLightComponent_SetAzimuth(ulong entityID, float azimuth);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SkyLightComponent_GetInclination(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SkyLightComponent_SetInclination(ulong entityID, float inclination);

		#endregion

		#region SpriteRendererComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_GetColor(ulong entityID, out Vector4 color);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_SetColor(ulong entityID, ref Vector4 color);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SpriteRendererComponent_GetTilingFactor(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_SetTilingFactor(ulong entityID, float tilingFactor);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_GetUVStart(ulong entityID, out Vector2 uvStart);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_SetUVStart(ulong entityID, ref Vector2 uvStart);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_GetUVEnd(ulong entityID, out Vector2 uvEnd);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_SetUVEnd(ulong entityID, ref Vector2 uvEnd);

		#endregion

		#region RigidBody2DComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern RigidBody2DBodyType RigidBody2DComponent_GetBodyType(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetBodyType(ulong entityID, RigidBody2DBodyType type);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_GetTranslation(ulong entityID, out Vector2 translation);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetTranslation(ulong entityID, ref Vector2 translation);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float RigidBody2DComponent_GetRotation(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetRotation(ulong entityID, float rotation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float RigidBody2DComponent_GetMass(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetMass(ulong entityID, float mass);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_GetLinearVelocity(ulong entityID, out Vector2 velocity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetLinearVelocity(ulong entityID, ref Vector2 velocity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float RigidBody2DComponent_GetGravityScale(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetGravityScale(ulong entityID, float gravityScale);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_ApplyLinearImpulse(ulong entityID, ref Vector2 impulse, ref Vector2 offset, bool wake);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_ApplyAngularImpulse(ulong entityID, float impulse, bool wake);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_AddForce(ulong entityID, ref Vector2 force, ref Vector2 offset, bool wake);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_AddTorque(ulong entityID, float torque, bool wake);

		#endregion

		#region RigidBodyComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_AddForce(ulong entityID, ref Vector3 force, EForceMode forceMode);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_AddForceAtLocation(ulong entityID, ref Vector3 force, ref Vector3 location, EForceMode forceMode);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_AddTorque(ulong entityID, ref Vector3 torque, EForceMode forceMode);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_GetLinearVelocity(ulong entityID, out Vector3 velocity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_SetLinearVelocity(ulong entityID, ref Vector3 velocity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_GetAngularVelocity(ulong entityID, out Vector3 velocity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_SetAngularVelocity(ulong entityID, ref Vector3 velocity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float RigidBodyComponent_GetMaxLinearVelocity(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_SetMaxLinearVelocity(ulong entityID, float maxVelocity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float RigidBodyComponent_GetMaxAngularVelocity(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_SetMaxAngularVelocity(ulong entityID, float maxVelocity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float RigidBodyComponent_GetLinearDrag(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_SetLinearDrag(ulong entityID, float linearDrag);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float RigidBodyComponent_GetAngularDrag(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_SetAngularDrag(ulong entityID, float angularDrag);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_Rotate(ulong entityID, ref Vector3 rotation);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern uint RigidBodyComponent_GetLayer(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_SetLayer(ulong entityID, uint layerID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern string RigidBodyComponent_GetLayerName(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_SetLayerByName(ulong entityID, string layerName);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float RigidBodyComponent_GetMass(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_SetMass(ulong entityID, float mass);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern EBodyType RigidBodyComponent_GetBodyType(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_SetBodyType(ulong entityID, EBodyType type);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBodyComponent_IsTrigger(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_SetTrigger(ulong entityID, bool isTrigger);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_MoveKinematic(ulong entityID, ref Vector3 targetPosition, ref Vector3 targetRotation, float deltaSeconds);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_SetAxisLock(ulong entityID, EActorAxis axis, bool value, bool forceWake);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBodyComponent_IsAxisLocked(ulong entityID, EActorAxis axis);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern uint RigidBodyComponent_GetLockedAxes(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBodyComponent_IsSleeping(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_SetIsSleeping(ulong entityID, bool isSleeping);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBodyComponent_Teleport(ulong entityID, ref Vector3 targetPosition, ref Vector3 targetRotation, bool force);

		#endregion

		#region CharacterControllerComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CharacterControllerComponent_GetIsGravityEnabled(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CharacterControllerComponent_SetIsGravityEnabled(ulong entityID, bool isGravityEnabled);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CharacterControllerComponent_GetSlopeLimit(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CharacterControllerComponent_SetSlopeLimit(ulong entityID, float mass);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CharacterControllerComponent_GetStepOffset(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CharacterControllerComponent_SetStepOffset(ulong entityID, float mass);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CharacterControllerComponent_SetTranslation(ulong entityID, ref Vector3 translation);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CharacterControllerComponent_SetRotation(ulong entityID, ref Quaternion rotation);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CharacterControllerComponent_Move(ulong entityID, ref Vector3 displacement);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CharacterControllerComponent_Jump(ulong entityID, float jumpPower);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CharacterControllerComponent_GetLinearVelocity(ulong entityID, out Vector3 velocity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CharacterControllerComponent_SetLinearVelocity(ulong entityID, ref Vector3 velocity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CharacterControllerComponent_IsGrounded(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ECollisionFlags CharacterControllerComponent_GetCollisionFlags(ulong entityID);

		#endregion

		#region FixedJointComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong FixedJointComponent_GetConnectedEntity(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void FixedJointComponent_SetConnectedEntity(ulong entityID, ulong connectedEntityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool FixedJointComponent_IsBreakable(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void FixedJointComponent_SetIsBreakable(ulong entityID, bool isBreakable);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool FixedJointComponent_IsBroken(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void FixedJointComponent_Break(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float FixedJointComponent_GetBreakForce(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void FixedJointComponent_SetBreakForce(ulong entityID, float breakForce);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float FixedJointComponent_GetBreakTorque(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void FixedJointComponent_SetBreakTorque(ulong entityID, float breakForce);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool FixedJointComponent_IsCollisionEnabled(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void FixedJointComponent_SetCollisionEnabled(ulong entityID, bool isCollisionEnabled);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool FixedJointComponent_IsPreProcessingEnabled(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void FixedJointComponent_SetPreProcessingEnabled(ulong entityID, bool isPreProcessingEnabled);

		#endregion

		#region BoxColliderComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void BoxColliderComponent_GetHalfSize(ulong entityID, out Vector3 halfSize);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void BoxColliderComponent_GetOffset(ulong entityID, out Vector3 offset);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxColliderComponent_GetMaterial(ulong entityID, out PhysicsMaterial outMaterial);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void BoxColliderComponent_SetMaterial(ulong entityID, ref PhysicsMaterial outMaterial);

		#endregion

		#region SphereColliderComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SphereColliderComponent_GetRadius(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SphereColliderComponent_GetOffset(ulong entityID, out Vector3 offset);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool SphereColliderComponent_GetMaterial(ulong entityID, out PhysicsMaterial outMaterial);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SphereColliderComponent_SetMaterial(ulong entityID, ref PhysicsMaterial outMaterial);

		#endregion

		#region CapsuleColliderComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CapsuleColliderComponent_GetRadius(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CapsuleColliderComponent_GetHeight(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CapsuleColliderComponent_GetOffset(ulong entityID, out Vector3 offset);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CapsuleColliderComponent_GetMaterial(ulong entityID, out PhysicsMaterial outMaterial);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CapsuleColliderComponent_SetMaterial(ulong entityID, ref PhysicsMaterial outMaterial);

		#endregion

		#region MeshColliderComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool MeshColliderComponent_IsMeshStatic(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool MeshColliderComponent_IsColliderMeshValid(ulong entityID, ref AssetHandle meshHandle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool MeshColliderComponent_GetColliderMesh(ulong entityID, out AssetHandle outHandle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool MeshColliderComponent_GetMaterial(ulong entityID, out PhysicsMaterial outMaterial);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void MeshColliderComponent_SetMaterial(ulong entityID, ref PhysicsMaterial outMaterial);

		#endregion

		#region MeshCollider

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool MeshCollider_IsStaticMesh(ref AssetHandle meshHandle);

		#endregion

		#region AudioComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool AudioComponent_IsPlaying(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool AudioComponent_Play(ulong entityID, float startTime);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool AudioComponent_Stop(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool AudioComponent_Pause(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool AudioComponent_Resume(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float AudioComponent_GetVolumeMult(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void AudioComponent_SetVolumeMult(ulong entityID, float volumeMult);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float AudioComponent_GetPitchMult(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void AudioComponent_SetPitchMult(ulong entityID, float pitchMult);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void AudioComponent_SetEvent(ulong entityID, uint eventID);

		#endregion

		#region TextComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong TextComponent_GetHash(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern string TextComponent_GetText(ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TextComponent_SetText(ulong entityID, string text);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TextComponent_GetColor(ulong entityID, out Vector4 outColor);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TextComponent_SetColor(ulong entityID, ref Vector4 inColor);

		#endregion

		#region Audio

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Audio_GetObjectInfo(ulong objectID, out string debugName);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_ReleaseAudioObject(ulong objectID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern uint Audio_PostEvent(uint id, ulong objectID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern uint Audio_PostEventFromAC(uint id, ulong entityID);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern uint Audio_PostEventAtLocation(uint id, ref Transform transform);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Audio_StopEventID(uint id);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Audio_PauseEventID(uint id);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Audio_ResumeEventID(uint id);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Audio_CreateAudioEntity(uint eventID, ref Transform transform, float volume, float pitch);

		#endregion

		#region AudioCommandID

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern uint AudioCommandID_Constructor(string commandName);

		#endregion

		#region AudioObject

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong AudioObject_Constructor(string debugName, ref Transform objectTransform);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void AudioObject_SetTransform(ulong objectID, ref Transform objectTransform);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void AudioObject_GetTransform(ulong objectID, out Transform transformOut);

		//============================================================================================
		/// Audio Parameters Interface
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_SetParameterFloat(uint id, ulong objectID, float value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_SetParameterFloatForAC(uint id, ulong entityID, float value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_SetParameterInt(uint id, ulong objectID, int value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_SetParameterIntForAC(uint id, ulong entityID, int value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_SetParameterBool(uint id, ulong objectID, bool value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_SetParameterBoolForAC(uint id, ulong entityID, bool value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_SetParameterFloatForEvent(uint id, uint eventID, float value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_SetParameterIntForEvent(uint id, uint eventID, int value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_SetParameterBoolForEvent(uint id, uint eventID, bool value);

		//============================================================================================
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_PreloadEventSources(uint eventID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_UnloadEventSources(uint eventID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_SetLowPassFilterValue(ulong objectID, float value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_SetHighPassFilterValue(ulong objectID, float value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_SetLowPassFilterValue_Event(uint eventID, float value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_SetHighPassFilterValue_Event(uint eventID, float value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_SetLowPassFilterValue_AC(ulong entityID, float value);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Audio_SetHighPassFilterValue_AC(ulong entityID, float value);
		#endregion

		#region Texture2D

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Texture2D_Create(uint width, uint height, TextureWrapMode wrapMode, TextureFilterMode filterMode, out AssetHandle outHandle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Texture2D_GetSize(out uint outWidth, out uint outHeight);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Texture2D_SetData(ref AssetHandle handle, Vector4[] data);
		//[MethodImpl(MethodImplOptions.InternalCall)]
		//internal static extern Vector4[] Texture2D_GetData(ref AssetHandle handle);

		#endregion

		#region Mesh

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Mesh_GetMaterialByIndex(ref AssetHandle meshHandle, int index, out AssetHandle outHandle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern int Mesh_GetMaterialCount(ref AssetHandle meshHandle);

		#endregion

		#region StaticMesh

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool StaticMesh_GetMaterialByIndex(ref AssetHandle meshHandle, int index, out AssetHandle outHandle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern int StaticMesh_GetMaterialCount(ref AssetHandle meshHandle);

		#endregion

		#region Material

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Material_GetAlbedoColor(ulong entityID, ref AssetHandle meshHandle, ref AssetHandle materialHandle, out Vector3 outAlbedoColor);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Material_SetAlbedoColor(ulong entityID, ref AssetHandle meshHandle, ref AssetHandle materialHandle, ref Vector3 inAlbedoColor);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float Material_GetMetalness(ulong entityID, ref AssetHandle meshHandle, ref AssetHandle materialHandle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Material_SetMetalness(ulong entityID, ref AssetHandle meshHandle, ref AssetHandle materialHandle, float inMetalness);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float Material_GetRoughness(ulong entityID, ref AssetHandle meshHandle, ref AssetHandle materialHandle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Material_SetRoughness(ulong entityID, ref AssetHandle meshHandle, ref AssetHandle materialHandle, float inRoughness);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float Material_GetEmission(ulong entityID, ref AssetHandle meshHandle, ref AssetHandle materialHandle);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Material_SetEmission(ulong entityID, ref AssetHandle meshHandle, ref AssetHandle materialHandle, float inEmission);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Material_SetFloat(ulong entityID, ref AssetHandle meshHandle, ref AssetHandle materialHandle, string uniform, float value);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Material_SetVector3(ulong entityID, ref AssetHandle meshHandle, ref AssetHandle materialHandle, string uniform, ref Vector3 value);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Material_SetVector4(ulong entityID, ref AssetHandle meshHandle, ref AssetHandle materialHandle, string uniform, ref Vector4 value);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Material_SetTexture(ulong entityID, ref AssetHandle meshHandle, ref AssetHandle materialHandle, string uniform, ref AssetHandle textureHandle);

		#endregion

		#region MeshFactory

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern IntPtr MeshFactory_CreatePlane(float width, float height);

		#endregion

		#region Physics

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Physics_CastRay(ref RaycastData raycastData, out SceneQueryHit outHit);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Physics_CastShape(ref ShapeQueryData shapeCastData, out SceneQueryHit outHit);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern int Physics_OverlapShape(ref ShapeQueryData shapeQueryData, out SceneQueryHit[] outHits);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern RaycastHit2D[] Physics_Raycast2D(ref RaycastData2D raycastData);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Physics_GetGravity(out Vector3 gravity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Physics_SetGravity(ref Vector3 gravity);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Physics_AddRadialImpulse(ref Vector3 origin, float radius, float strength, EFalloffMode falloff, bool velocityChange);

		#endregion

		#region Noise

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern IntPtr Noise_Constructor(int seed);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Noise_Destructor(IntPtr instance);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float Noise_GetFrequency(IntPtr instance);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Noise_SetFrequency(IntPtr instance, float frequency);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern int Noise_GetFractalOctaves(IntPtr instance);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Noise_SetFractalOctaves(IntPtr instance, int octaves);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float Noise_GetFractalLacunarity(IntPtr instance);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Noise_SetFractalLacunarity(IntPtr instance, float lacunarity);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float Noise_GetFractalGain(IntPtr instance);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Noise_SetFractalGain(IntPtr instance, float gain);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float Noise_Get(IntPtr instance, float x, float y);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Noise_SetSeed(int seed);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float Noise_Perlin(float x, float y);


		#endregion

		#region Matrix4

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Matrix4_LookAt(ref Vector3 eye, ref Vector3 center, ref Vector3 up, ref Matrix4 outMatrix);

		#endregion

		#region Log

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Log_LogMessage(Log.LogLevel level, string formattedMessage);

		#endregion

		#region Input

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_IsKeyPressed(KeyCode keycode);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_IsKeyHeld(KeyCode keycode);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_IsKeyDown(KeyCode keycode);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_IsKeyReleased(KeyCode keycode);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_IsMouseButtonPressed(MouseButton button);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_IsMouseButtonHeld(MouseButton button);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_IsMouseButtonDown(MouseButton button);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_IsMouseButtonReleased(MouseButton button);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Input_GetMousePosition(out Vector2 position);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Input_SetCursorMode(CursorMode mode);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern CursorMode Input_GetCursorMode();

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_IsControllerPresent(int id);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern int[] Input_GetConnectedControllerIDs();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern string Input_GetControllerName(int id);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_IsControllerButtonPressed(int id, int button);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_IsControllerButtonHeld(int id, int button);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_IsControllerButtonDown(int id, int button);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_IsControllerButtonReleased(int id, int button);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float Input_GetControllerAxis(int id, int axis);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern byte Input_GetControllerHat(int id, int hat);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float Input_GetControllerDeadzone(int id, int axis);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Input_SetControllerDeadzone(int id, int axis, float deadzone);


		#endregion

		#region EditorUI

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void EditorUI_Text(string inLabel);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool EditorUI_Button(string inLabel, ref Vector2 inSize);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool EditorUI_BeginPropertyHeader(string label, bool openByDefault);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void EditorUI_EndPropertyHeader();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void EditorUI_PropertyGrid(bool begin);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool EditorUI_PropertyFloat(string inLabel, ref float outValue);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool EditorUI_PropertyVec2(string inLabel, ref Vector2 outValue);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool EditorUI_PropertyVec3(string inLabel, ref Vector3 outValue);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool EditorUI_PropertyVec4(string inLabel, ref Vector4 outValue);
		
		#endregion
		
		#region SceneRenderer

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool SceneRenderer_IsFogEnabled();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SceneRenderer_SetFogEnabled(bool enable);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SceneRenderer_GetOpacity();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SceneRenderer_SetOpacity(float opacity);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool SceneRenderer_DepthOfField_IsEnabled();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SceneRenderer_DepthOfField_SetEnabled(bool enabled);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SceneRenderer_DepthOfField_GetFocusDistance();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SceneRenderer_DepthOfField_SetFocusDistance(float focusDistance);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SceneRenderer_DepthOfField_GetBlurSize();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SceneRenderer_DepthOfField_SetBlurSize(float blurSize);

		#endregion

		#region DebugRenderer

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void DebugRenderer_DrawLine(ref Vector3 p0, ref Vector3 p1, ref Vector4 color);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void DebugRenderer_DrawQuadBillboard(ref Vector3 translation, ref Vector2 size, ref Vector4 color);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void DebugRenderer_SetLineWidth(float width);


		#endregion

		#region PerformanceTimers

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float PerformanceTimers_GetFrameTime();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float PerformanceTimers_GetGPUTime();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float PerformanceTimers_GetMainThreadWorkTime();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float PerformanceTimers_GetMainThreadWaitTime();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float PerformanceTimers_GetRenderThreadWorkTime();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float PerformanceTimers_GetRenderThreadWaitTime();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern uint PerformanceTimers_GetFramesPerSecond();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern uint PerformanceTimers_GetEntityCount();
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern uint PerformanceTimers_GetScriptEntityCount();

		#endregion
	}
}

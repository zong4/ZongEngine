#include "hzpch.h"
#include "ScriptGlue.h"

#include "Hazel/Animation/AnimationGraph.h"
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Audio/AudioComponent.h"
#include "Hazel/Audio/AudioEngine.h"
#include "Hazel/Audio/AudioEvents/AudioCommandRegistry.h"
#include "Hazel/Audio/AudioPlayback.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Core/Events/EditorEvents.h"
#include "Hazel/Core/Hash.h"
#include "Hazel/Core/Math/Noise.h"
#include "Hazel/ImGui/ImGui.h"
#include "Hazel/Physics/PhysicsLayer.h"
#include "Hazel/Physics2D/Physics2D.h"

#include "Hazel/Script/ScriptEngine.h"

#include "Hazel/Renderer/MeshFactory.h"
#include "Hazel/Renderer/SceneRenderer.h"
#include "Hazel/Scene/Prefab.h"
#include "Hazel/Utilities/TypeInfo.h"
#include "Hazel/Reflection/TypeName.h"

#include <box2d/box2d.h>
#include <glm/gtc/quaternion.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <glm/glm.hpp>

#include <functional>

#include <Coral/HostInstance.hpp>

namespace Hazel {

#ifdef HZ_PLATFORM_WINDOWS
	#define HZ_FUNCTION_NAME __func__
#else
	#define HZ_FUNCTION_NAME __FUNCTION__
#endif

#define HZ_ADD_INTERNAL_CALL(icall) coreAssembly.AddInternalCall("Hazel.InternalCalls", #icall, (void*)InternalCalls::icall)

#ifdef HZ_DIST
	#define HZ_ICALL_VALIDATE_PARAM(param)
	#define HZ_ICALL_VALIDATE_PARAM_V(param, value)
#else
	#define HZ_ICALL_VALIDATE_PARAM(param) { if (!(param)) { HZ_CORE_ERROR("{} called with an invalid value ({}) for parameter '{}'", HZ_FUNCTION_NAME, param, #param); } }
	#define HZ_ICALL_VALIDATE_PARAM_V(param, value) { if (!(param)) { HZ_CORE_ERROR("{} called with an invalid value ({}) for parameter '{}'.", HZ_FUNCTION_NAME, value, #param); } }
#endif

	std::unordered_map<Coral::TypeId, std::function<void(Entity&)>> s_CreateComponentFuncs;
	std::unordered_map<Coral::TypeId, std::function<bool(Entity&)>> s_HasComponentFuncs;
	std::unordered_map<Coral::TypeId, std::function<void(Entity&)>> s_RemoveComponentFuncs;

	template<typename TComponent>
	static void RegisterManagedComponent(Coral::ManagedAssembly& coreAssembly)
	{
		// NOTE(Peter): Get the demangled type name of TComponent
		const TypeNameString& componentTypeName = TypeInfo<TComponent, true>().Name();
		std::string componentName = std::format("Hazel.{}", componentTypeName);

		// Backwards compatibility: Submesh component is "Hazel.Mesh" for scripting.
		if constexpr (std::is_same_v<TComponent, SubmeshComponent>)
		{
			componentName = "Hazel.MeshComponent";
		}

		auto& type = coreAssembly.GetType(componentName);

		if (type)
		{
			s_CreateComponentFuncs[type.GetTypeId()] = [](Entity& entity) { entity.AddComponent<TComponent>(); };
			s_HasComponentFuncs[type.GetTypeId()] = [](Entity& entity) { return entity.HasComponent<TComponent>(); };
			s_RemoveComponentFuncs[type.GetTypeId()] = [](Entity& entity) { entity.RemoveComponent<TComponent>(); };
		}
		else
		{
			HZ_CORE_VERIFY(false, "No C# component class found for {}!", componentName);
		}
	}

	template<typename TComponent>
	static void RegisterManagedComponent(std::function<void(Entity&)>&& addFunction, Coral::ManagedAssembly& coreAssembly)
	{
		// NOTE(Peter): Get the demangled type name of TComponent
		const TypeNameString& componentTypeName = TypeInfo<TComponent, true>().Name();
		std::string componentName = std::format("Hazel.{}", componentTypeName);

		auto& type = coreAssembly.GetType(componentName);

		if (type)
		{
			s_CreateComponentFuncs[type.GetTypeId()] = std::move(addFunction);
			s_HasComponentFuncs[type.GetTypeId()] = [](Entity& entity) { return entity.HasComponent<TComponent>(); };
			s_RemoveComponentFuncs[type.GetTypeId()] = [](Entity& entity) { entity.RemoveComponent<TComponent>(); };
		}
		else
		{
			HZ_CORE_VERIFY(false, "No C# component class found for {}!", componentName);
		}
	}

	template<typename... Args>
	static void WarnWithTrace(std::format_string<Args...> format, Args&&... args)
	{
		/*auto stackTrace = ScriptUtils::GetCurrentStackTrace();
		std::string formattedMessage = std::format(inFormat, std::forward<TArgs>(inArgs)...);
		Log::GetEditorConsoleLogger()->warn("{}\nStack Trace: {}", formattedMessage, stackTrace);*/
	}

	template<typename... Args>
	static void ErrorWithTrace(const std::format_string<Args...> format, Args&&... args)
	{
		/*auto stackTrace = ScriptUtils::GetCurrentStackTrace();
		std::string formattedMessage = std::format(inFormat, std::forward<TArgs>(inArgs)...);
		Log::GetEditorConsoleLogger()->error("{}\nStack Trace: {}", formattedMessage, stackTrace);*/
	}

	void ScriptGlue::RegisterGlue(Coral::ManagedAssembly& coreAssembly)
	{
		if (!s_CreateComponentFuncs.empty())
		{
			s_CreateComponentFuncs.clear();
			s_HasComponentFuncs.clear();
			s_RemoveComponentFuncs.clear();
		}

		RegisterComponentTypes(coreAssembly);
		RegisterInternalCalls(coreAssembly);

		coreAssembly.UploadInternalCalls();
	}

	void ScriptGlue::RegisterComponentTypes(Coral::ManagedAssembly& coreAssembly)
	{
		RegisterManagedComponent<TransformComponent>(coreAssembly);
		RegisterManagedComponent<TagComponent>(coreAssembly);
		RegisterManagedComponent<SubmeshComponent>(coreAssembly); // Note: C++ SubmeshComponent is mapped to C# MeshComponent (so as to not break old scripts)
		RegisterManagedComponent<StaticMeshComponent>(coreAssembly);
		RegisterManagedComponent<AnimationComponent>(coreAssembly);
		RegisterManagedComponent<ScriptComponent>(coreAssembly);
		RegisterManagedComponent<CameraComponent>(coreAssembly);
		RegisterManagedComponent<DirectionalLightComponent>(coreAssembly);
		RegisterManagedComponent<PointLightComponent>(coreAssembly);
		RegisterManagedComponent<SpotLightComponent>(coreAssembly);
		RegisterManagedComponent<SkyLightComponent>(coreAssembly);
		RegisterManagedComponent<SpriteRendererComponent>(coreAssembly);
		RegisterManagedComponent<RigidBody2DComponent>(coreAssembly);
		RegisterManagedComponent<BoxCollider2DComponent>(coreAssembly);
		RegisterManagedComponent<RigidBodyComponent>([](Entity& entity)
		{
			RigidBodyComponent component;
			component.EnableDynamicTypeChange = true;
			entity.AddComponent<RigidBodyComponent>(component);
		}, coreAssembly);
		RegisterManagedComponent<BoxColliderComponent>(coreAssembly);
		RegisterManagedComponent<SphereColliderComponent>(coreAssembly);
		RegisterManagedComponent<CapsuleColliderComponent>(coreAssembly);
		RegisterManagedComponent<MeshColliderComponent>(coreAssembly);
		RegisterManagedComponent<CharacterControllerComponent>(coreAssembly);
		RegisterManagedComponent<TextComponent>(coreAssembly);
		RegisterManagedComponent<AudioListenerComponent>(coreAssembly);
		RegisterManagedComponent<AudioComponent>(coreAssembly);
	}

	void ScriptGlue::RegisterInternalCalls(Coral::ManagedAssembly& coreAssembly)
	{
		HZ_ADD_INTERNAL_CALL(AssetHandle_IsValid);

		HZ_ADD_INTERNAL_CALL(Application_Quit);
		HZ_ADD_INTERNAL_CALL(Application_GetWidth);
		HZ_ADD_INTERNAL_CALL(Application_GetHeight);
		HZ_ADD_INTERNAL_CALL(Application_GetDataDirectoryPath);
		HZ_ADD_INTERNAL_CALL(Application_GetSetting);
		HZ_ADD_INTERNAL_CALL(Application_GetSettingInt);
		HZ_ADD_INTERNAL_CALL(Application_GetSettingFloat);

		HZ_ADD_INTERNAL_CALL(SceneManager_IsSceneValid);
		HZ_ADD_INTERNAL_CALL(SceneManager_IsSceneIDValid);
		HZ_ADD_INTERNAL_CALL(SceneManager_LoadScene);
		HZ_ADD_INTERNAL_CALL(SceneManager_LoadSceneByID);
		HZ_ADD_INTERNAL_CALL(SceneManager_GetCurrentSceneID);
		HZ_ADD_INTERNAL_CALL(SceneManager_GetCurrentSceneName);

		HZ_ADD_INTERNAL_CALL(Scene_FindEntityByTag);
		HZ_ADD_INTERNAL_CALL(Scene_IsEntityValid);
		HZ_ADD_INTERNAL_CALL(Scene_CreateEntity);
		HZ_ADD_INTERNAL_CALL(Scene_InstantiatePrefab);
		HZ_ADD_INTERNAL_CALL(Scene_InstantiatePrefabWithTranslation);
		HZ_ADD_INTERNAL_CALL(Scene_InstantiatePrefabWithTransform);
		HZ_ADD_INTERNAL_CALL(Scene_InstantiateChildPrefabWithTranslation);
		HZ_ADD_INTERNAL_CALL(Scene_InstantiateChildPrefabWithTransform);
		HZ_ADD_INTERNAL_CALL(Scene_DestroyEntity);
		HZ_ADD_INTERNAL_CALL(Scene_DestroyAllChildren);
		HZ_ADD_INTERNAL_CALL(Scene_GetEntities);
		HZ_ADD_INTERNAL_CALL(Scene_GetChildrenIDs);
		HZ_ADD_INTERNAL_CALL(Scene_SetTimeScale);

		HZ_ADD_INTERNAL_CALL(Entity_GetParent);
		HZ_ADD_INTERNAL_CALL(Entity_SetParent);
		HZ_ADD_INTERNAL_CALL(Entity_GetChildren);
		HZ_ADD_INTERNAL_CALL(Entity_CreateComponent);
		HZ_ADD_INTERNAL_CALL(Entity_HasComponent);
		HZ_ADD_INTERNAL_CALL(Entity_RemoveComponent);

		HZ_ADD_INTERNAL_CALL(TagComponent_GetTag);
		HZ_ADD_INTERNAL_CALL(TagComponent_SetTag);

		HZ_ADD_INTERNAL_CALL(TransformComponent_GetTransform);
		HZ_ADD_INTERNAL_CALL(TransformComponent_SetTransform);
		HZ_ADD_INTERNAL_CALL(TransformComponent_GetTranslation);
		HZ_ADD_INTERNAL_CALL(TransformComponent_SetTranslation);
		HZ_ADD_INTERNAL_CALL(TransformComponent_GetRotation);
		HZ_ADD_INTERNAL_CALL(TransformComponent_SetRotation);
		HZ_ADD_INTERNAL_CALL(TransformComponent_GetScale);
		HZ_ADD_INTERNAL_CALL(TransformComponent_SetScale);
		HZ_ADD_INTERNAL_CALL(TransformComponent_GetWorldSpaceTransform);
		HZ_ADD_INTERNAL_CALL(TransformComponent_GetTransformMatrix);
		HZ_ADD_INTERNAL_CALL(TransformComponent_SetTransformMatrix);
		HZ_ADD_INTERNAL_CALL(TransformComponent_GetRotationQuat);
		HZ_ADD_INTERNAL_CALL(TransformComponent_SetRotationQuat);
		HZ_ADD_INTERNAL_CALL(TransformMultiply_Native);

		HZ_ADD_INTERNAL_CALL(MeshComponent_GetMesh);
		HZ_ADD_INTERNAL_CALL(MeshComponent_SetMesh);
		HZ_ADD_INTERNAL_CALL(MeshComponent_GetVisible);
		HZ_ADD_INTERNAL_CALL(MeshComponent_SetVisible);
		HZ_ADD_INTERNAL_CALL(MeshComponent_HasMaterial);
		HZ_ADD_INTERNAL_CALL(MeshComponent_GetMaterial);
		HZ_ADD_INTERNAL_CALL(MeshComponent_GetIsRigged);

		HZ_ADD_INTERNAL_CALL(StaticMeshComponent_GetMesh);
		HZ_ADD_INTERNAL_CALL(StaticMeshComponent_SetMesh);
		HZ_ADD_INTERNAL_CALL(StaticMeshComponent_HasMaterial);
		HZ_ADD_INTERNAL_CALL(StaticMeshComponent_GetMaterial);
		HZ_ADD_INTERNAL_CALL(StaticMeshComponent_SetMaterial);
		HZ_ADD_INTERNAL_CALL(StaticMeshComponent_GetVisible);
		HZ_ADD_INTERNAL_CALL(StaticMeshComponent_SetVisible);

		HZ_ADD_INTERNAL_CALL(Identifier_Get);
		HZ_ADD_INTERNAL_CALL(AnimationComponent_GetInputBool);
		HZ_ADD_INTERNAL_CALL(AnimationComponent_SetInputBool);
		HZ_ADD_INTERNAL_CALL(AnimationComponent_GetInputInt);
		HZ_ADD_INTERNAL_CALL(AnimationComponent_SetInputInt);
		HZ_ADD_INTERNAL_CALL(AnimationComponent_GetInputFloat);
		HZ_ADD_INTERNAL_CALL(AnimationComponent_SetInputFloat);
		HZ_ADD_INTERNAL_CALL(AnimationComponent_GetInputVector3);
		HZ_ADD_INTERNAL_CALL(AnimationComponent_SetInputVector3);
		HZ_ADD_INTERNAL_CALL(AnimationComponent_SetInputTrigger);
		HZ_ADD_INTERNAL_CALL(AnimationComponent_GetRootMotion);

		HZ_ADD_INTERNAL_CALL(ScriptComponent_GetInstance);

		HZ_ADD_INTERNAL_CALL(CameraComponent_SetPerspective);
		HZ_ADD_INTERNAL_CALL(CameraComponent_SetOrthographic);
		HZ_ADD_INTERNAL_CALL(CameraComponent_GetVerticalFOV);
		HZ_ADD_INTERNAL_CALL(CameraComponent_SetVerticalFOV);
		HZ_ADD_INTERNAL_CALL(CameraComponent_GetPerspectiveNearClip);
		HZ_ADD_INTERNAL_CALL(CameraComponent_SetPerspectiveNearClip);
		HZ_ADD_INTERNAL_CALL(CameraComponent_GetPerspectiveFarClip);
		HZ_ADD_INTERNAL_CALL(CameraComponent_SetPerspectiveFarClip);
		HZ_ADD_INTERNAL_CALL(CameraComponent_GetOrthographicSize);
		HZ_ADD_INTERNAL_CALL(CameraComponent_SetOrthographicSize);
		HZ_ADD_INTERNAL_CALL(CameraComponent_GetOrthographicNearClip);
		HZ_ADD_INTERNAL_CALL(CameraComponent_SetOrthographicNearClip);
		HZ_ADD_INTERNAL_CALL(CameraComponent_GetOrthographicFarClip);
		HZ_ADD_INTERNAL_CALL(CameraComponent_SetOrthographicFarClip);
		HZ_ADD_INTERNAL_CALL(CameraComponent_GetProjectionType);
		HZ_ADD_INTERNAL_CALL(CameraComponent_SetProjectionType);
		HZ_ADD_INTERNAL_CALL(CameraComponent_GetPrimary);
		HZ_ADD_INTERNAL_CALL(CameraComponent_SetPrimary);
		HZ_ADD_INTERNAL_CALL(CameraComponent_ToScreenSpace);

		HZ_ADD_INTERNAL_CALL(DirectionalLightComponent_GetRadiance);
		HZ_ADD_INTERNAL_CALL(DirectionalLightComponent_SetRadiance);
		HZ_ADD_INTERNAL_CALL(DirectionalLightComponent_GetIntensity);
		HZ_ADD_INTERNAL_CALL(DirectionalLightComponent_SetIntensity);
		HZ_ADD_INTERNAL_CALL(DirectionalLightComponent_GetCastShadows);
		HZ_ADD_INTERNAL_CALL(DirectionalLightComponent_SetCastShadows);
		HZ_ADD_INTERNAL_CALL(DirectionalLightComponent_GetSoftShadows);
		HZ_ADD_INTERNAL_CALL(DirectionalLightComponent_SetSoftShadows);
		HZ_ADD_INTERNAL_CALL(DirectionalLightComponent_GetLightSize);
		HZ_ADD_INTERNAL_CALL(DirectionalLightComponent_SetLightSize);

		HZ_ADD_INTERNAL_CALL(PointLightComponent_GetRadiance);
		HZ_ADD_INTERNAL_CALL(PointLightComponent_SetRadiance);
		HZ_ADD_INTERNAL_CALL(PointLightComponent_GetIntensity);
		HZ_ADD_INTERNAL_CALL(PointLightComponent_SetIntensity);
		HZ_ADD_INTERNAL_CALL(PointLightComponent_GetRadius);
		HZ_ADD_INTERNAL_CALL(PointLightComponent_SetRadius);
		HZ_ADD_INTERNAL_CALL(PointLightComponent_GetFalloff);
		HZ_ADD_INTERNAL_CALL(PointLightComponent_SetFalloff);

		HZ_ADD_INTERNAL_CALL(SkyLightComponent_GetIntensity);
		HZ_ADD_INTERNAL_CALL(SkyLightComponent_SetIntensity);
		HZ_ADD_INTERNAL_CALL(SkyLightComponent_GetTurbidity);
		HZ_ADD_INTERNAL_CALL(SkyLightComponent_SetTurbidity);
		HZ_ADD_INTERNAL_CALL(SkyLightComponent_GetAzimuth);
		HZ_ADD_INTERNAL_CALL(SkyLightComponent_SetAzimuth);
		HZ_ADD_INTERNAL_CALL(SkyLightComponent_GetInclination);
		HZ_ADD_INTERNAL_CALL(SkyLightComponent_SetInclination);
		
		HZ_ADD_INTERNAL_CALL(SpotLightComponent_GetRadiance);
		HZ_ADD_INTERNAL_CALL(SpotLightComponent_SetRadiance);
		HZ_ADD_INTERNAL_CALL(SpotLightComponent_GetIntensity);
		HZ_ADD_INTERNAL_CALL(SpotLightComponent_SetIntensity);
		HZ_ADD_INTERNAL_CALL(SpotLightComponent_GetRange);
		HZ_ADD_INTERNAL_CALL(SpotLightComponent_SetRange);
		HZ_ADD_INTERNAL_CALL(SpotLightComponent_GetAngle);
		HZ_ADD_INTERNAL_CALL(SpotLightComponent_SetAngle);
		HZ_ADD_INTERNAL_CALL(SpotLightComponent_GetAngleAttenuation);
		HZ_ADD_INTERNAL_CALL(SpotLightComponent_SetAngleAttenuation);
		HZ_ADD_INTERNAL_CALL(SpotLightComponent_GetFalloff);
		HZ_ADD_INTERNAL_CALL(SpotLightComponent_SetFalloff);
		HZ_ADD_INTERNAL_CALL(SpotLightComponent_SetCastsShadows);
		HZ_ADD_INTERNAL_CALL(SpotLightComponent_GetCastsShadows);
		HZ_ADD_INTERNAL_CALL(SpotLightComponent_GetSoftShadows);
		HZ_ADD_INTERNAL_CALL(SpotLightComponent_SetSoftShadows);

		HZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetColor);
		HZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetColor);
		HZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetTilingFactor);
		HZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetTilingFactor);
		HZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetUVStart);
		HZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetUVStart);
		HZ_ADD_INTERNAL_CALL(SpriteRendererComponent_GetUVEnd);
		HZ_ADD_INTERNAL_CALL(SpriteRendererComponent_SetUVEnd);

		HZ_ADD_INTERNAL_CALL(RigidBody2DComponent_GetBodyType);
		HZ_ADD_INTERNAL_CALL(RigidBody2DComponent_SetBodyType);
		HZ_ADD_INTERNAL_CALL(RigidBody2DComponent_GetTranslation);
		HZ_ADD_INTERNAL_CALL(RigidBody2DComponent_SetTranslation);
		HZ_ADD_INTERNAL_CALL(RigidBody2DComponent_GetRotation);
		HZ_ADD_INTERNAL_CALL(RigidBody2DComponent_SetRotation);
		HZ_ADD_INTERNAL_CALL(RigidBody2DComponent_GetMass);
		HZ_ADD_INTERNAL_CALL(RigidBody2DComponent_SetMass);
		HZ_ADD_INTERNAL_CALL(RigidBody2DComponent_GetLinearVelocity);
		HZ_ADD_INTERNAL_CALL(RigidBody2DComponent_SetLinearVelocity);
		HZ_ADD_INTERNAL_CALL(RigidBody2DComponent_GetGravityScale);
		HZ_ADD_INTERNAL_CALL(RigidBody2DComponent_SetGravityScale);
		HZ_ADD_INTERNAL_CALL(RigidBody2DComponent_ApplyLinearImpulse);
		HZ_ADD_INTERNAL_CALL(RigidBody2DComponent_ApplyAngularImpulse);
		HZ_ADD_INTERNAL_CALL(RigidBody2DComponent_AddForce);
		HZ_ADD_INTERNAL_CALL(RigidBody2DComponent_AddTorque);

		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_AddForce);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_AddForceAtLocation);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_AddTorque);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_GetLinearVelocity);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_SetLinearVelocity);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_GetAngularVelocity);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_SetAngularVelocity);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_GetMaxLinearVelocity);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_SetMaxLinearVelocity);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_GetMaxAngularVelocity);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_SetMaxAngularVelocity);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_GetLinearDrag);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_SetLinearDrag);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_GetAngularDrag);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_SetAngularDrag);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_Rotate);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_GetLayer);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_SetLayer);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_GetLayerName);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_SetLayerByName);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_GetMass);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_SetMass);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_GetBodyType);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_SetBodyType);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_MoveKinematic);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_SetAxisLock);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_IsAxisLocked);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_GetLockedAxes);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_IsSleeping);
		HZ_ADD_INTERNAL_CALL(RigidBodyComponent_SetIsSleeping);

		HZ_ADD_INTERNAL_CALL(CharacterControllerComponent_GetIsGravityEnabled);
		HZ_ADD_INTERNAL_CALL(CharacterControllerComponent_SetIsGravityEnabled);
		HZ_ADD_INTERNAL_CALL(CharacterControllerComponent_GetSlopeLimit);
		HZ_ADD_INTERNAL_CALL(CharacterControllerComponent_SetSlopeLimit);
		HZ_ADD_INTERNAL_CALL(CharacterControllerComponent_GetStepOffset);
		HZ_ADD_INTERNAL_CALL(CharacterControllerComponent_SetStepOffset);
		HZ_ADD_INTERNAL_CALL(CharacterControllerComponent_SetTranslation);
		HZ_ADD_INTERNAL_CALL(CharacterControllerComponent_SetRotation);
		HZ_ADD_INTERNAL_CALL(CharacterControllerComponent_Move);
		HZ_ADD_INTERNAL_CALL(CharacterControllerComponent_Rotate);
		HZ_ADD_INTERNAL_CALL(CharacterControllerComponent_Jump);
		HZ_ADD_INTERNAL_CALL(CharacterControllerComponent_GetLinearVelocity);
		HZ_ADD_INTERNAL_CALL(CharacterControllerComponent_SetLinearVelocity);
		HZ_ADD_INTERNAL_CALL(CharacterControllerComponent_IsGrounded);
		HZ_ADD_INTERNAL_CALL(CharacterControllerComponent_GetCollisionFlags);

		HZ_ADD_INTERNAL_CALL(BoxColliderComponent_GetHalfSize);
		HZ_ADD_INTERNAL_CALL(BoxColliderComponent_GetOffset);
		HZ_ADD_INTERNAL_CALL(BoxColliderComponent_GetMaterial);
		HZ_ADD_INTERNAL_CALL(BoxColliderComponent_SetMaterial);

		HZ_ADD_INTERNAL_CALL(SphereColliderComponent_GetRadius);
		HZ_ADD_INTERNAL_CALL(SphereColliderComponent_GetOffset);
		HZ_ADD_INTERNAL_CALL(SphereColliderComponent_GetMaterial);
		HZ_ADD_INTERNAL_CALL(SphereColliderComponent_SetMaterial);

		HZ_ADD_INTERNAL_CALL(CapsuleColliderComponent_GetRadius);
		HZ_ADD_INTERNAL_CALL(CapsuleColliderComponent_GetHeight);
		HZ_ADD_INTERNAL_CALL(CapsuleColliderComponent_GetOffset);
		HZ_ADD_INTERNAL_CALL(CapsuleColliderComponent_GetMaterial);
		HZ_ADD_INTERNAL_CALL(CapsuleColliderComponent_SetMaterial);

		HZ_ADD_INTERNAL_CALL(MeshColliderComponent_IsMeshStatic);
		HZ_ADD_INTERNAL_CALL(MeshColliderComponent_IsColliderMeshValid);
		HZ_ADD_INTERNAL_CALL(MeshColliderComponent_GetColliderMesh);
		HZ_ADD_INTERNAL_CALL(MeshColliderComponent_GetMaterial);
		HZ_ADD_INTERNAL_CALL(MeshColliderComponent_SetMaterial);

		HZ_ADD_INTERNAL_CALL(MeshCollider_IsStaticMesh);

		HZ_ADD_INTERNAL_CALL(AudioComponent_IsPlaying);
		HZ_ADD_INTERNAL_CALL(AudioComponent_Play);
		HZ_ADD_INTERNAL_CALL(AudioComponent_Stop);
		HZ_ADD_INTERNAL_CALL(AudioComponent_Pause);
		HZ_ADD_INTERNAL_CALL(AudioComponent_Resume);
		HZ_ADD_INTERNAL_CALL(AudioComponent_GetVolumeMult);
		HZ_ADD_INTERNAL_CALL(AudioComponent_SetVolumeMult);
		HZ_ADD_INTERNAL_CALL(AudioComponent_GetPitchMult);
		HZ_ADD_INTERNAL_CALL(AudioComponent_SetPitchMult);
		HZ_ADD_INTERNAL_CALL(AudioComponent_SetEvent);

		HZ_ADD_INTERNAL_CALL(TextComponent_GetHash);
		HZ_ADD_INTERNAL_CALL(TextComponent_GetText);
		HZ_ADD_INTERNAL_CALL(TextComponent_SetText);
		HZ_ADD_INTERNAL_CALL(TextComponent_GetColor);
		HZ_ADD_INTERNAL_CALL(TextComponent_SetColor);

		//============================================================================================
		/// Audio
		HZ_ADD_INTERNAL_CALL(Audio_PostEvent);
		HZ_ADD_INTERNAL_CALL(Audio_PostEventFromAC);
		HZ_ADD_INTERNAL_CALL(Audio_PostEventAtLocation);
		HZ_ADD_INTERNAL_CALL(Audio_StopEventID);
		HZ_ADD_INTERNAL_CALL(Audio_PauseEventID);
		HZ_ADD_INTERNAL_CALL(Audio_ResumeEventID);
		HZ_ADD_INTERNAL_CALL(Audio_CreateAudioEntity);

		HZ_ADD_INTERNAL_CALL(AudioCommandID_Constructor);
		//============================================================================================
		/// Audio Parameters Interface
		HZ_ADD_INTERNAL_CALL(Audio_SetParameterFloat);
		HZ_ADD_INTERNAL_CALL(Audio_SetParameterInt);
		HZ_ADD_INTERNAL_CALL(Audio_SetParameterBool);
		HZ_ADD_INTERNAL_CALL(Audio_SetParameterFloatForAC);
		HZ_ADD_INTERNAL_CALL(Audio_SetParameterIntForAC);
		HZ_ADD_INTERNAL_CALL(Audio_SetParameterBoolForAC);

		HZ_ADD_INTERNAL_CALL(Audio_SetParameterFloatForEvent);
		HZ_ADD_INTERNAL_CALL(Audio_SetParameterIntForEvent);
		HZ_ADD_INTERNAL_CALL(Audio_SetParameterBoolForEvent);
		//============================================================================================
		HZ_ADD_INTERNAL_CALL(Audio_PreloadEventSources);
		HZ_ADD_INTERNAL_CALL(Audio_UnloadEventSources);

		HZ_ADD_INTERNAL_CALL(Audio_SetLowPassFilterValue);
		HZ_ADD_INTERNAL_CALL(Audio_SetHighPassFilterValue);
		HZ_ADD_INTERNAL_CALL(Audio_SetLowPassFilterValue_Event);
		HZ_ADD_INTERNAL_CALL(Audio_SetHighPassFilterValue_Event);

		HZ_ADD_INTERNAL_CALL(Audio_SetLowPassFilterValue_AC);
		HZ_ADD_INTERNAL_CALL(Audio_SetHighPassFilterValue_AC);

		//============================================================================================

		HZ_ADD_INTERNAL_CALL(Texture2D_Create);
		HZ_ADD_INTERNAL_CALL(Texture2D_GetSize);
		HZ_ADD_INTERNAL_CALL(Texture2D_SetData);
		//HZ_ADD_INTERNAL_CALL(Texture2D_GetData);

		HZ_ADD_INTERNAL_CALL(Mesh_GetMaterialByIndex);
		HZ_ADD_INTERNAL_CALL(Mesh_GetMaterialCount);

		HZ_ADD_INTERNAL_CALL(StaticMesh_GetMaterialByIndex);
		HZ_ADD_INTERNAL_CALL(StaticMesh_GetMaterialCount);

		HZ_ADD_INTERNAL_CALL(Material_GetAlbedoColor);
		HZ_ADD_INTERNAL_CALL(Material_SetAlbedoColor);
		HZ_ADD_INTERNAL_CALL(Material_GetMetalness);
		HZ_ADD_INTERNAL_CALL(Material_SetMetalness);
		HZ_ADD_INTERNAL_CALL(Material_GetRoughness);
		HZ_ADD_INTERNAL_CALL(Material_SetRoughness);
		HZ_ADD_INTERNAL_CALL(Material_GetEmission);
		HZ_ADD_INTERNAL_CALL(Material_SetEmission);
		HZ_ADD_INTERNAL_CALL(Material_SetFloat);
		HZ_ADD_INTERNAL_CALL(Material_SetVector3);
		HZ_ADD_INTERNAL_CALL(Material_SetVector4);
		HZ_ADD_INTERNAL_CALL(Material_SetTexture);

		HZ_ADD_INTERNAL_CALL(MeshFactory_CreatePlane);

		HZ_ADD_INTERNAL_CALL(Physics_CastRay);
		HZ_ADD_INTERNAL_CALL(Physics_CastShape);
		HZ_ADD_INTERNAL_CALL(Physics_Raycast2D);
		HZ_ADD_INTERNAL_CALL(Physics_OverlapShape);
		HZ_ADD_INTERNAL_CALL(Physics_GetGravity);
		HZ_ADD_INTERNAL_CALL(Physics_SetGravity);
		HZ_ADD_INTERNAL_CALL(Physics_AddRadialImpulse);

		HZ_ADD_INTERNAL_CALL(Matrix4_LookAt);

		HZ_ADD_INTERNAL_CALL(Noise_Constructor);
		HZ_ADD_INTERNAL_CALL(Noise_Destructor);
		HZ_ADD_INTERNAL_CALL(Noise_GetFrequency);
		HZ_ADD_INTERNAL_CALL(Noise_SetFrequency);
		HZ_ADD_INTERNAL_CALL(Noise_GetFractalOctaves);
		HZ_ADD_INTERNAL_CALL(Noise_SetFractalOctaves);
		HZ_ADD_INTERNAL_CALL(Noise_GetFractalLacunarity);
		HZ_ADD_INTERNAL_CALL(Noise_SetFractalLacunarity);
		HZ_ADD_INTERNAL_CALL(Noise_GetFractalGain);
		HZ_ADD_INTERNAL_CALL(Noise_SetFractalGain);
		HZ_ADD_INTERNAL_CALL(Noise_Get);

		HZ_ADD_INTERNAL_CALL(Noise_SetSeed);
		HZ_ADD_INTERNAL_CALL(Noise_Perlin);

		HZ_ADD_INTERNAL_CALL(Log_LogMessage);

		HZ_ADD_INTERNAL_CALL(Input_IsKeyPressed);
		HZ_ADD_INTERNAL_CALL(Input_IsKeyHeld);
		HZ_ADD_INTERNAL_CALL(Input_IsKeyDown);
		HZ_ADD_INTERNAL_CALL(Input_IsKeyReleased);
		HZ_ADD_INTERNAL_CALL(Input_IsMouseButtonPressed);
		HZ_ADD_INTERNAL_CALL(Input_IsMouseButtonHeld);
		HZ_ADD_INTERNAL_CALL(Input_IsMouseButtonDown);
		HZ_ADD_INTERNAL_CALL(Input_IsMouseButtonReleased);
		HZ_ADD_INTERNAL_CALL(Input_GetMousePosition);
		HZ_ADD_INTERNAL_CALL(Input_SetCursorMode);
		HZ_ADD_INTERNAL_CALL(Input_GetCursorMode);
		HZ_ADD_INTERNAL_CALL(Input_IsControllerPresent);
		HZ_ADD_INTERNAL_CALL(Input_GetConnectedControllerIDs);
		HZ_ADD_INTERNAL_CALL(Input_GetControllerName);
		HZ_ADD_INTERNAL_CALL(Input_IsControllerButtonPressed);
		HZ_ADD_INTERNAL_CALL(Input_IsControllerButtonHeld);
		HZ_ADD_INTERNAL_CALL(Input_IsControllerButtonDown);
		HZ_ADD_INTERNAL_CALL(Input_IsControllerButtonReleased);
		HZ_ADD_INTERNAL_CALL(Input_GetControllerAxis);
		HZ_ADD_INTERNAL_CALL(Input_GetControllerHat);
		HZ_ADD_INTERNAL_CALL(Input_GetControllerDeadzone);
		HZ_ADD_INTERNAL_CALL(Input_SetControllerDeadzone);

		HZ_ADD_INTERNAL_CALL(SceneRenderer_GetOpacity);
		HZ_ADD_INTERNAL_CALL(SceneRenderer_SetOpacity);

		HZ_ADD_INTERNAL_CALL(SceneRenderer_DepthOfField_IsEnabled);
		HZ_ADD_INTERNAL_CALL(SceneRenderer_DepthOfField_SetEnabled);
		HZ_ADD_INTERNAL_CALL(SceneRenderer_DepthOfField_GetFocusDistance);
		HZ_ADD_INTERNAL_CALL(SceneRenderer_DepthOfField_SetFocusDistance);
		HZ_ADD_INTERNAL_CALL(SceneRenderer_DepthOfField_GetBlurSize);
		HZ_ADD_INTERNAL_CALL(SceneRenderer_DepthOfField_SetBlurSize);

		HZ_ADD_INTERNAL_CALL(DebugRenderer_DrawLine);
		HZ_ADD_INTERNAL_CALL(DebugRenderer_DrawQuadBillboard);
		HZ_ADD_INTERNAL_CALL(DebugRenderer_SetLineWidth);

		HZ_ADD_INTERNAL_CALL(PerformanceTimers_GetFrameTime);
		HZ_ADD_INTERNAL_CALL(PerformanceTimers_GetGPUTime);
		HZ_ADD_INTERNAL_CALL(PerformanceTimers_GetMainThreadWorkTime);
		HZ_ADD_INTERNAL_CALL(PerformanceTimers_GetMainThreadWaitTime);
		HZ_ADD_INTERNAL_CALL(PerformanceTimers_GetRenderThreadWorkTime);
		HZ_ADD_INTERNAL_CALL(PerformanceTimers_GetRenderThreadWaitTime);
		HZ_ADD_INTERNAL_CALL(PerformanceTimers_GetFramesPerSecond);
		HZ_ADD_INTERNAL_CALL(PerformanceTimers_GetEntityCount);
		HZ_ADD_INTERNAL_CALL(PerformanceTimers_GetScriptEntityCount);
	}

	namespace InternalCalls {

		static inline Entity GetEntity(uint64_t entityID)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_VERIFY(scene, "No active scene!");
			return scene->TryGetEntityWithUUID(entityID);
		};

#pragma region AssetHandle

		bool AssetHandle_IsValid(Param<AssetHandle> assetHandle)
		{
			return AssetManager::IsAssetHandleValid(assetHandle);
		}

#pragma endregion

#pragma region Application

		void Application_Quit()
		{
#ifdef HZ_DIST
			Application::Get().DispatchEvent<WindowCloseEvent>();
#else
			Application::Get().DispatchEvent<EditorExitPlayModeEvent>();
#endif
		}

		uint32_t Application_GetWidth() { return ScriptEngine::GetInstance().GetCurrentScene()->GetViewportWidth(); }
		uint32_t Application_GetHeight() { return ScriptEngine::GetInstance().GetCurrentScene()->GetViewportHeight(); }

		Coral::String Application_GetSetting(Coral::String name, Coral::String defaultValue)
		{
			std::string key = name;
			return Coral::String::New(Application::Get().GetSettings().Get(key, defaultValue));
		}

		int Application_GetSettingInt(Coral::String name, int defaultValue)
		{
			std::string key = name;
			return Application::Get().GetSettings().GetInt(key, defaultValue);
		}
		
		float Application_GetSettingFloat(Coral::String name, float defaultValue)
		{
			std::string key = name;
			return Application::Get().GetSettings().GetFloat(key, defaultValue);
		}

		Coral::String Application_GetDataDirectoryPath()
		{
			auto filepath = Project::GetProjectDirectory() / "Data";
			if (!std::filesystem::exists(filepath))
				std::filesystem::create_directory(filepath);

			return Coral::String::New(filepath.string());
		}

#pragma endregion

#pragma region SceneManager

		bool SceneManager_IsSceneValid(Coral::String inScene)
		{
			return FileSystem::Exists(Project::GetActiveAssetDirectory() / std::string(inScene));
		}

		bool SceneManager_IsSceneIDValid(Param<AssetHandle> sceneHandle)
		{
			return AssetManager::GetAsset<Scene>(sceneHandle) != nullptr;
		}

		void SceneManager_LoadScene(Param<AssetHandle> sceneHandle)
		{
			Ref<Scene> activeScene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_ASSERT(activeScene, "No active scene!");
			HZ_ICALL_VALIDATE_PARAM(AssetManager::IsAssetHandleValid(sceneHandle));

			activeScene->OnSceneTransition(sceneHandle);
		}

		void SceneManager_LoadSceneByID(Param<AssetHandle> sceneHandle)
		{
			Ref<Scene> activeScene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_VERIFY(activeScene, "No active scene!");

			// TODO(Yan): OnSceneTransition should take scene by AssetHandle, NOT filepath (because this won't work in runtime)
			const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(sceneHandle);

			if (metadata.Type != AssetType::Scene || !metadata.IsValid())
			{
				ErrorWithTrace("Tried to load a scene with an invalid ID ('{}')", (AssetHandle)sceneHandle);
				return;
			}

			activeScene->OnSceneTransition(sceneHandle);
		}

		uint64_t SceneManager_GetCurrentSceneID() { return ScriptEngine::GetInstance().GetCurrentScene()->GetUUID(); }

		Coral::String SceneManager_GetCurrentSceneName()
		{ 
			//TODO: It would be good if this could take an AssetHandle and return the name of the specified scene
			//return activeScene = AssetManager::GetAsset<Scene>(assetHandle)->GetName();

			Ref<Scene> activeScene = ScriptEngine::GetInstance().GetCurrentScene();
			return Coral::String::New(activeScene->GetName());
		}

#pragma endregion

#pragma region Scene

		uint64_t Scene_FindEntityByTag(Coral::String tag)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_VERIFY(scene, "No active scene!");
			Entity entity = scene->TryGetEntityWithTag(tag);
			return entity ? entity.GetUUID() : UUID(0);
		}

		bool Scene_IsEntityValid(uint64_t entityID)
		{
			if (entityID == 0)
				return false;

			return (bool)(ScriptEngine::GetInstance().GetCurrentScene()->TryGetEntityWithUUID(entityID));
		}

		uint64_t Scene_CreateEntity(Coral::String tag)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_VERIFY(scene, "No active scene!");
			return scene->CreateEntity(tag).GetUUID();
		}

		uint64_t Scene_InstantiatePrefab(Param<AssetHandle> prefabHandle)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_VERIFY(scene, "No active scene!");

			Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(prefabHandle);
			if (prefab == nullptr)
			{
				WarnWithTrace("Cannot instantiate prefab. No prefab with handle {} found.", AssetHandle(prefabHandle));
				return 0;
			}

			return scene->Instantiate(prefab).GetUUID();
		}

		uint64_t Scene_InstantiatePrefabWithTranslation(Param<AssetHandle> prefabHandle, glm::vec3* inTranslation)
		{
			return Scene_InstantiatePrefabWithTransform(prefabHandle, inTranslation, nullptr, nullptr);
		}

		uint64_t Scene_InstantiatePrefabWithTransform(Param<AssetHandle> prefabHandle, glm::vec3* inTranslation, glm::vec3* inRotation, glm::vec3* inScale)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_VERIFY(scene, "No active scene!");

			Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(prefabHandle);
			if (prefab == nullptr)
			{
				WarnWithTrace<AssetHandle>("Cannot instantiate prefab. No prefab with handle {} found.", AssetHandle(prefabHandle));
				return 0;
			}

			return scene->Instantiate(prefab, inTranslation, inRotation, inScale).GetUUID();
		}

		uint64_t Scene_InstantiateChildPrefabWithTranslation(uint64_t parentID, Param<AssetHandle> prefabHandle, glm::vec3* inTranslation)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_VERIFY(scene, "No active scene!");
			Entity parent = scene->TryGetEntityWithUUID(parentID);
			HZ_ICALL_VALIDATE_PARAM_V(parent, parentID);

			Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(prefabHandle);
			if (prefab == nullptr)
			{
				ErrorWithTrace<AssetHandle>("Cannot instantiate prefab. No prefab with handle {} found.", AssetHandle(prefabHandle));
				return 0;
			}

			return scene->InstantiateChild(prefab, parent, inTranslation, nullptr, nullptr).GetUUID();
		}

		uint64_t Scene_InstantiateChildPrefabWithTransform(uint64_t parentID, Param<AssetHandle> prefabHandle, glm::vec3* inTranslation, glm::vec3* inRotation, glm::vec3* inScale)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_VERIFY(scene, "No active scene!");
			Entity parent = scene->TryGetEntityWithUUID(parentID);
			HZ_ICALL_VALIDATE_PARAM_V(parent, parentID);

			Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(prefabHandle);
			if (prefab == nullptr)
			{
				ErrorWithTrace<AssetHandle>("Cannot instantiate prefab. No prefab with handle {} found.", AssetHandle(prefabHandle));
				return 0;
			}

			return scene->InstantiateChild(prefab, parent, inTranslation, inRotation, inScale).GetUUID();
		}

		void Scene_DestroyEntity(uint64_t entityID)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_VERIFY(scene, "No active scene!");
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			scene->SubmitToDestroyEntity(entity);
		}

		void Scene_DestroyAllChildren(uint64_t entityID)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_VERIFY(scene, "No active scene!");
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			const std::vector<UUID> children = entity.Children();
			for (UUID id : children)
				scene->DestroyEntity(id);
		}

		Coral::Array<uint64_t> Scene_GetEntities()
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_VERIFY(scene, "No active scene!");

			auto entities = scene->GetAllEntitiesWith<IDComponent>();
			auto result = Coral::Array<uint64_t>::New(int32_t(entities.size()));
			uint32_t i = 0;
			for (auto entity : entities)
				result[i++]= entities.get<IDComponent>(entity).ID;

			return result;
		}

		Coral::Array<uint64_t> Scene_GetChildrenIDs(uint64_t entityID)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_VERIFY(scene, "No active scene!");
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			const auto& children = entity.Children();
			auto result = Coral::Array<uint64_t>::New(int32_t(children.size()));

			for (size_t i = 0; i < children.size(); i++)
				result[i] = children[i];

			return result;
		}

		void Scene_SetTimeScale(float timeScale)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_VERIFY(scene, "No active scene!");
			scene->SetTimeScale(timeScale);
		}

#pragma endregion

#pragma region Entity

		uint64_t Entity_GetParent(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			return entity.GetParentUUID();
		}

		void Entity_SetParent(uint64_t entityID, uint64_t parentID)
		{
			Entity child = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(child, entityID);

			if (parentID == 0)
			{
				ScriptEngine::GetInstance().GetCurrentScene()->UnparentEntity(child);
			}
			else
			{
				Entity parent = GetEntity(parentID);
				HZ_ICALL_VALIDATE_PARAM_V(parent, parentID);
				child.SetParent(parent);
			}
		}

		Coral::Array<uint64_t> Entity_GetChildren(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			const auto& children = entity.Children();
			auto result = Coral::Array<uint64_t>::New(int32_t(children.size()));
			for (uint32_t i = 0; i < children.size(); i++)
				result[i] = children[i];

			return result;
		}

		void Entity_CreateComponent(uint64_t entityID, Coral::ReflectionType componentType)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			if (!entity)
				return;

			Coral::Type& type = componentType;

			if (!type)
				return;

			//Coral::ScopedString typeName = type.GetFullName();

			if (auto it = s_HasComponentFuncs.find(type.GetTypeId()); it != s_HasComponentFuncs.end() && it->second(entity))
			{
				//WarnWithTrace("Attempting to add duplicate component '{}' to entity '{}', ignoring.", std::string(typeName), entity.Name());
				return;
			}

			if (auto it = s_CreateComponentFuncs.find(type.GetTypeId()); it != s_CreateComponentFuncs.end())
			{
				return it->second(entity);
			}

			//ErrorWithTrace("Cannot create component of type '{}' for entity '{}'. That component hasn't been registered with the engine.", std::string(typeName), entity.Name());
		}

		bool Entity_HasComponent(uint64_t entityID, Coral::ReflectionType componentType)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			if (!entity)
				return false;

			Coral::Type& type = componentType;

			if (!type)
				return false;

			//Coral::ScopedString typeName = type.GetFullName();

			if (auto it = s_HasComponentFuncs.find(type.GetTypeId()); it != s_HasComponentFuncs.end())
			{
				// Note (0x): compiler bug?  if you return it->second(entity) directly, it does not return the correct result
				// e.g. evaluating the function returns false, but then the caller receives true ??!!
				// Copying the function first, then calling it works as expected.
				auto func = it->second;
				return func(entity);
			}

			//ErrorWithTrace("Cannot check if entity '{}' has a component of type '{}'. That component hasn't been registered with the engine.", entity.Name(), std::string(typeName));
			return false;
		}

		bool Entity_RemoveComponent(uint64_t entityID, Coral::ReflectionType componentType)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			if (!entity)
				return false;

			Coral::Type& type = componentType;

			if (!type)
				return false;

			//Coral::ScopedString typeName = type.GetFullName();

			if (auto it = s_HasComponentFuncs.find(type.GetTypeId()); it == s_HasComponentFuncs.end() || !it->second(entity))
			{
				//WarnWithTrace("Tried to remove component '{}' from entity '{}' even though it doesn't have that component.", std::string(typeName), entity.Name());
				return false;
			}

			if (auto it = s_RemoveComponentFuncs.find(type.GetTypeId()); it != s_RemoveComponentFuncs.end())
			{
				it->second(entity);
				return true;
			}

			//ErrorWithTrace("Cannot remove component of type '{}' from entity '{}'. That component hasn't been registered with the engine.", std::string(typeName), entity.Name());
			return false;
		}

#pragma endregion

#pragma region TagComponent

		Coral::String TagComponent_GetTag(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			const auto& tagComponent = entity.GetComponent<TagComponent>();
			return Coral::String::New(tagComponent.Tag);
		}

		void TagComponent_SetTag(uint64_t entityID, Coral::String inTag)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			auto& tagComponent = entity.GetComponent<TagComponent>();
			tagComponent.Tag = inTag;
		}

#pragma endregion

		Ref<PhysicsScene> GetPhysicsScene()
		{
			Ref<Scene> entityScene = ScriptEngine::GetInstance().GetCurrentScene();

			if (!entityScene->IsPlaying())
				return nullptr;

			return entityScene->GetPhysicsScene();
		}

		Ref<PhysicsBody> GetRigidBody(uint64_t entityID)
		{
			Ref<Scene> entityScene = ScriptEngine::GetInstance().GetCurrentScene();

			if (!entityScene->IsPlaying())
				return nullptr;

			Entity entity = entityScene->TryGetEntityWithUUID(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			Ref<PhysicsScene> physicsScene = entityScene->GetPhysicsScene();

			if (!physicsScene)
				return nullptr;

			return physicsScene->GetEntityBody(entity);
		}

		Ref<CharacterController> GetCharacterController(uint64_t entityID)
		{
			Ref<Scene> entityScene = ScriptEngine::GetInstance().GetCurrentScene();

			if (!entityScene->IsPlaying())
				return nullptr;

			Entity entity = entityScene->TryGetEntityWithUUID(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			Ref<PhysicsScene> physicsScene = entityScene->GetPhysicsScene();

			if (!physicsScene)
				return nullptr;

			return physicsScene->GetCharacterController(entity);
		}

#pragma region TransformComponent

		void TransformComponent_GetTransform(uint64_t entityID, Transform* outTransform)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			const auto& tc = entity.GetComponent<TransformComponent>();
			outTransform->Translation = tc.Translation;
			outTransform->Rotation = tc.GetRotationEuler();
			outTransform->Scale = tc.Scale;
		}

		void TransformComponent_SetTransform(uint64_t entityID, Transform* inTransform)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);


			if (inTransform == nullptr)
			{
				ErrorWithTrace("Attempting to set a null transform for entity '{}'", entity.Name());
				return;
			}

			auto& tc = entity.GetComponent<TransformComponent>();
			// call TransformComponent_Set... methods so that SetTransform(transform) behaves the same way as
			// setting individual components.
			TransformComponent_SetTranslation(entityID, &inTransform->Translation);
			TransformComponent_SetRotation(entityID, &inTransform->Rotation);
			tc.Scale = inTransform->Scale;
		}

		void TransformComponent_GetTranslation(uint64_t entityID, glm::vec3* outTranslation)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			*outTranslation = entity.GetComponent<TransformComponent>().Translation;
		}


		static void TeleportPhysicsBody(Entity& entity)
		{
			if (entity.HasComponent<RigidBodyComponent>())
			{
				const auto& rigidBodyComponent = entity.GetComponent<RigidBodyComponent>();
				auto rigidBody = GetRigidBody(entity.GetUUID());

				if (rigidBody)
				{
					Ref<Scene> scene = Scene::GetScene(entity.GetSceneUUID());
					HZ_CORE_VERIFY(scene, "No scene active?");

					const TransformComponent worldTransform = scene->GetWorldSpaceTransform(entity);

					switch (rigidBodyComponent.BodyType)
					{
						case EBodyType::Dynamic:
							WarnWithTrace("Setting the transform on a dynamic RigidBody. This will 'teleport' the body!");
							// intentional drop through to next case
						case EBodyType::Kinematic:
							GetPhysicsScene()->Teleport(entity, worldTransform.Translation, worldTransform.GetRotation(), true);
							break;
						case EBodyType::Static:
							rigidBody->SetTranslation(worldTransform.Translation);
							break;
					}
				}
				else
				{
					ErrorWithTrace("No rigid body found for entity '{}'!", entity.Name());
				}
			}
			else if (entity.HasComponent<CharacterControllerComponent>())
			{
				auto characterController = GetCharacterController(entity.GetUUID());

				if (characterController)
				{
					WarnWithTrace("Setting the translation on a character controller. This will 'teleport' the character!");

					Ref<Scene> scene = Scene::GetScene(entity.GetSceneUUID());
					HZ_CORE_VERIFY(scene, "No scene active?");
					const TransformComponent worldTransform = scene->GetWorldSpaceTransform(entity);
					characterController->SetTranslation(worldTransform.Translation);
				}
				else
				{
					ErrorWithTrace("No character controller found for entity '{}'!", entity.Name());
				}
			}
		}


		void TransformComponent_SetTranslation(uint64_t entityID, glm::vec3* inTranslation)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			if (inTranslation == nullptr)
			{
				ErrorWithTrace("Attempting to set null translation for entity '{}'", entity.Name());
				return;
			}

			// Setting translation always "teleports" the entity to the new location.
			// If you want to move it via physics, then use the appropriate method on the physics component.
			// For example:  RigidBodyComponent_AddForce(), CharacterControllerComponent_Move() etc.
			entity.GetComponent<TransformComponent>().Translation = *inTranslation;
			TeleportPhysicsBody(entity);
		}


		void TransformComponent_GetRotation(uint64_t entityID, glm::vec3* outRotation)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			*outRotation = entity.GetComponent<TransformComponent>().GetRotationEuler();
		}


		void TransformComponent_SetRotation(uint64_t entityID, glm::vec3* inRotation)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			if (inRotation == nullptr)
			{
				ErrorWithTrace("Attempting to set null rotation for entity '{}'!", entity.Name());
				return;
			}

			// Setting translation always "teleports" the entity to the orientation.
			// If you want to move it via physics, then use the appropriate method on the physics component.
			// For example:  RigidBodyComponent_AddTorque(), CharacterControllerComponent_Move() etc.
			entity.GetComponent<TransformComponent>().SetRotationEuler(*inRotation);
			TeleportPhysicsBody(entity);
		}


		void TransformComponent_GetRotationQuat(uint64_t entityID, glm::quat* outRotation)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			*outRotation = entity.GetComponent<TransformComponent>().GetRotation();
		}


		void TransformComponent_SetRotationQuat(uint64_t entityID, glm::quat* inRotation)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			if (inRotation == nullptr)
			{
				ErrorWithTrace("Attempting to set null rotation for entity '{}'!", entity.Name());
				return;
			}

			// Setting translation always "teleports" the entity to the orientation.
			// If you want to move it via physics, then use the appropriate method on the physics component.
			// For example:  RigidBodyComponent_AddTorque(), CharacterControllerComponent_Move() etc.
			entity.GetComponent<TransformComponent>().SetRotation(*inRotation);
			TeleportPhysicsBody(entity);
		}


		void TransformComponent_GetScale(uint64_t entityID, glm::vec3* outScale)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			*outScale = entity.GetComponent<TransformComponent>().Scale;
		}


		void TransformComponent_SetScale(uint64_t entityID, glm::vec3* inScale)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			if (!entity)
				return;

			if (inScale == nullptr)
			{
				ErrorWithTrace("Attempting to set null scale for entity '{}'!", entity.Name());
				return;
			}

			entity.GetComponent<TransformComponent>().Scale = *inScale;
		}

		void TransformComponent_GetWorldSpaceTransform(uint64_t entityID, Transform* outTransform)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();

			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			const auto& wt = scene->GetWorldSpaceTransform(entity);
			outTransform->Translation = wt.Translation;
			outTransform->Rotation = wt.GetRotationEuler();
			outTransform->Scale = wt.Scale;
		}

		void TransformComponent_GetTransformMatrix(uint64_t entityID, glm::mat4* outTransform)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			*outTransform = entity.Transform().GetTransform();
		}

		void TransformComponent_SetTransformMatrix(uint64_t entityID, glm::mat4* inTransform)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			entity.Transform().SetTransform(*inTransform);
		}

		void TransformMultiply_Native(Transform* inA, Transform* inB, Transform* outResult)
		{
			TransformComponent a;
			a.Translation = inA->Translation;
			a.SetRotationEuler(inA->Rotation);
			a.Scale = inA->Scale;

			TransformComponent b;
			b.Translation = inB->Translation;
			b.SetRotationEuler(inB->Rotation);
			b.Scale = inB->Scale;

			glm::mat4 transform = a.GetTransform() * b.GetTransform();
			b.SetTransform(transform);
			outResult->Translation = b.Translation;
			outResult->Rotation = b.GetRotationEuler();
			outResult->Scale = b.Scale;
		}

#pragma endregion

#pragma region MeshComponent

		bool MeshComponent_GetMesh(uint64_t entityID, OutParam<AssetHandle> outHandle)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

			if (entity.HasComponent<SubmeshComponent>())
			{
				const auto& meshComponent = entity.GetComponent<SubmeshComponent>();
				auto mesh = AssetManager::GetAsset<Mesh>(meshComponent.Mesh);

				if (!mesh)
				{
					ErrorWithTrace("Component has an invalid mesh asset!");
					*outHandle = AssetHandle(0);
					return false;
				}

				*outHandle = meshComponent.Mesh;
				return true;

			}

			ErrorWithTrace("This message should never appear. If it does it means the engine is broken.");
			*outHandle = AssetHandle(0);
			return false;
		}

		void MeshComponent_SetMesh(uint64_t entityID, Param<AssetHandle> meshHandle)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SubmeshComponent>());
			auto& meshComponent = entity.GetComponent<SubmeshComponent>();
			meshComponent.Mesh = meshHandle;
		}

		bool MeshComponent_GetVisible(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SubmeshComponent>());
			const auto& meshComponent = entity.GetComponent<SubmeshComponent>();
			return meshComponent.Visible;
		}

		void MeshComponent_SetVisible(uint64_t entityID, bool visible)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SubmeshComponent>());
			auto& meshComponent = entity.GetComponent<SubmeshComponent>();
			meshComponent.Visible = visible;
		}

		bool MeshComponent_HasMaterial(uint64_t entityID, int index)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SubmeshComponent>());
			const auto& meshComponent = entity.GetComponent<SubmeshComponent>();
			auto mesh = AssetManager::GetAsset<Mesh>(meshComponent.Mesh);
			Ref<MaterialTable> materialTable = meshComponent.MaterialTable;
			return (materialTable && materialTable->HasMaterial(index)) || mesh->GetMaterials()->HasMaterial(index);
		}

		bool MeshComponent_GetMaterial(uint64_t entityID, int index, OutParam<AssetHandle> outHandle)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SubmeshComponent>());

			const auto& meshComponent = entity.GetComponent<SubmeshComponent>();

			Ref<MaterialTable> materialTable = meshComponent.MaterialTable;

			if (materialTable->HasMaterial(index))
			{
				*outHandle = materialTable->GetMaterial(index);
			}
			else
			{
				auto mesh = AssetManager::GetAsset<Mesh>(meshComponent.Mesh);

				if (!mesh->GetMaterials()->HasMaterial(index))
				{
					*outHandle = AssetHandle(0);
					return false;
				}

				*outHandle = mesh->GetMaterials()->GetMaterial(index);
			}

			return true;
		}

		bool MeshComponent_GetIsRigged(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SubmeshComponent>());

			auto& meshComponent = entity.GetComponent<SubmeshComponent>();
			if(auto mesh = AssetManager::GetAsset<Mesh>(meshComponent.Mesh); mesh)
			{
				if (auto meshSource = AssetManager::GetAsset<MeshSource>(mesh->GetMeshSource()); meshSource)
				{
					return meshSource->IsSubmeshRigged(meshComponent.SubmeshIndex);
				}
			}
			return false;
		}

#pragma endregion

#pragma region StaticMeshComponent

		bool StaticMeshComponent_GetMesh(uint64_t entityID, OutParam<AssetHandle> outHandle)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<StaticMeshComponent>());

			const auto& meshComponent = entity.GetComponent<StaticMeshComponent>();
			auto mesh = AssetManager::GetAsset<StaticMesh>(meshComponent.StaticMesh);

			if (!mesh)
			{
				ErrorWithTrace("Component has an invalid mesh asset!");
				*outHandle = AssetHandle(0);
				return false;
			}

			*outHandle = meshComponent.StaticMesh;
			return true;
		}

		void StaticMeshComponent_SetMesh(uint64_t entityID, Param<AssetHandle> meshHandle)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<StaticMeshComponent>());
			auto& meshComponent = entity.GetComponent<StaticMeshComponent>();
			meshComponent.StaticMesh = meshHandle;
		}

		bool StaticMeshComponent_HasMaterial(uint64_t entityID, int index)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<StaticMeshComponent>());
			const auto& meshComponent = entity.GetComponent<StaticMeshComponent>();
			auto mesh = AssetManager::GetAsset<StaticMesh>(meshComponent.StaticMesh);
			Ref<MaterialTable> materialTable = meshComponent.MaterialTable;
			return (materialTable && materialTable->HasMaterial(index)) || mesh->GetMaterials()->HasMaterial(index);
		}

		bool StaticMeshComponent_GetMaterial(uint64_t entityID, int index, OutParam<AssetHandle> outHandle)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<StaticMeshComponent>());

			const auto& meshComponent = entity.GetComponent<StaticMeshComponent>();

			Ref<MaterialTable> materialTable = meshComponent.MaterialTable;

			if (materialTable->HasMaterial(index))
			{
				*outHandle = materialTable->GetMaterial(index);
			}
			else
			{
				auto mesh = AssetManager::GetAsset<StaticMesh>(meshComponent.StaticMesh);

				if (!mesh->GetMaterials()->HasMaterial(index))
				{
					*outHandle = AssetHandle(0);
					return false;
				}

				*outHandle = mesh->GetMaterials()->GetMaterial(index);
			}

			return true;
		}

		void StaticMeshComponent_SetMaterial(uint64_t entityID, int index, Param<AssetHandle> materialHandle)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<StaticMeshComponent>());

			Ref<MaterialTable> materialTable = entity.GetComponent<StaticMeshComponent>().MaterialTable;

			if ((uint32_t)index >= materialTable->GetMaterialCount())
			{
				WarnWithTrace("Material index out of range: {0}. Expected index less than {1}", index, materialTable->GetMaterialCount());
				return;
			}

			materialTable->SetMaterial(index, materialHandle);
		}

		bool StaticMeshComponent_GetVisible(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<StaticMeshComponent>());
			const auto& meshComponent = entity.GetComponent<StaticMeshComponent>();
			return meshComponent.Visible;
		}

		void StaticMeshComponent_SetVisible(uint64_t entityID, bool visible)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<StaticMeshComponent>());
			auto& meshComponent = entity.GetComponent<StaticMeshComponent>();
			meshComponent.Visible = visible;
		}

#pragma endregion

#pragma region AnimationComponent

		uint32_t Identifier_Get(Coral::String inName)
		{
			return Identifier(std::string(inName));
		}


		bool AnimationComponent_GetInputBool(uint64_t entityID, uint32_t inputID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AnimationComponent>());

			auto& animationComponent = entity.GetComponent<AnimationComponent>();
			if (animationComponent.AnimationGraph)
			{
				try
				{
					return animationComponent.AnimationGraph->Ins.at(inputID).getBool();
				}
				catch (const std::out_of_range&)
				{
					HZ_CONSOLE_LOG_ERROR("AnimationComponent.GetInputBool() - input with id {0} does not exist!", inputID);
				}
				catch (const choc::value::Error&)
				{
					HZ_CONSOLE_LOG_ERROR("AnimationComponent.GetInputBool() - input with id {0} is not of boolean type!", inputID);
				}
			}
			return false;
		}

		void AnimationComponent_SetInputBool(uint64_t entityID, uint32_t inputID, bool value)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AnimationComponent>());

			auto& animationComponent = entity.GetComponent<AnimationComponent>();
			if (animationComponent.AnimationGraph)
			{
				try
				{
					animationComponent.AnimationGraph->Ins.at(inputID).set(value);
				}
				catch (const std::out_of_range&)
				{
					HZ_CONSOLE_LOG_ERROR("AnimationComponent.SetInputBool() - input with id {0} does not exist!", inputID);
				}
				catch (const choc::value::Error&)
				{
					HZ_CONSOLE_LOG_ERROR("AnimationComponent.SetInputBool() - input with id {0} is not of boolean type!", inputID);
				}
			}
		}


		int32_t AnimationComponent_GetInputInt(uint64_t entityID, uint32_t inputID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AnimationComponent>());

			auto& animationComponent = entity.GetComponent<AnimationComponent>();
			if (animationComponent.AnimationGraph)
			{
				try
				{
					return animationComponent.AnimationGraph->Ins.at(inputID).getInt32();
				}
				catch (const std::out_of_range&)
				{
					HZ_CONSOLE_LOG_ERROR("AnimationComponent.GetInputInt() - input with id {0} does not exist!", inputID);
				}
				catch (const choc::value::Error&)
				{
					HZ_CONSOLE_LOG_ERROR("AnimationComponent.GetInputInt() - input with id {0} is not of integer type!", inputID);
				}
			}
			return false;
		}

		void AnimationComponent_SetInputInt(uint64_t entityID, uint32_t inputID, int32_t value)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AnimationComponent>());

			auto& animationComponent = entity.GetComponent<AnimationComponent>();
			if (animationComponent.AnimationGraph)
			{
				try
				{
					animationComponent.AnimationGraph->Ins.at(inputID).set(value);
				}
				catch (const std::out_of_range&)
				{
					HZ_CONSOLE_LOG_ERROR("AnimationComponent.SetInputInt() - input with id {0} does not exist!", inputID);
				}
				catch (const choc::value::Error&)
				{
					HZ_CONSOLE_LOG_ERROR("AnimationComponent.SetInputInt() - input with id {0} is not of integer type!", inputID);
				}
			}
		}


		float AnimationComponent_GetInputFloat(uint64_t entityID, uint32_t inputID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AnimationComponent>());

			auto& animationComponent = entity.GetComponent<AnimationComponent>();
			if (animationComponent.AnimationGraph)
			{
				try
				{
					return animationComponent.AnimationGraph->Ins.at(inputID).getFloat32();
				}
				catch (const std::out_of_range&)
				{
					HZ_CONSOLE_LOG_ERROR("AnimationComponent.GetInputFloat() - input with id {0} does not exist!", inputID);
				}
				catch (const choc::value::Error&)
				{
					HZ_CONSOLE_LOG_ERROR("AnimationComponent.GetInputFloat() - input with id {0} is not of float type!", inputID);
				}
			}
			return false;
		}

		void AnimationComponent_SetInputFloat(uint64_t entityID, uint32_t inputID, float value)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AnimationComponent>());

			auto& animationComponent = entity.GetComponent<AnimationComponent>();
			if (animationComponent.AnimationGraph)
			{
				try
				{
					animationComponent.AnimationGraph->Ins.at(inputID).set(value);
				}
				catch (const std::out_of_range&)
				{
					HZ_CONSOLE_LOG_ERROR("AnimationComponent.SetInputFloat() - input with id {0} does not exist!", inputID);
				}
				catch (const choc::value::Error&)
				{
					HZ_CONSOLE_LOG_ERROR("AnimationComponent.SetInputFloat() - input with id {0} is not of float type!", inputID);
				}
			}
		}


		void AnimationComponent_GetInputVector3(uint64_t entityID, uint32_t inputID, glm::vec3* value)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AnimationComponent>());

			auto& animationComponent = entity.GetComponent<AnimationComponent>();
			if (animationComponent.AnimationGraph)
			{
				try
				{
					auto& input = animationComponent.AnimationGraph->Ins.at(inputID);
					if (input.isObjectWithClassName(type::type_name<glm::vec3>()))
					{
						memcpy(value, input.getRawData(), sizeof(glm::vec3));
					}
					else
					{
						HZ_CONSOLE_LOG_ERROR("AnimationComponent.GetInputVector3() - input with id {0} is not of Vector3 type!", inputID);
					}
				}
				catch (const std::out_of_range&)
				{
					HZ_CONSOLE_LOG_ERROR("AnimationComponent.GetInputVector3() - input with id {0} does not exist!", inputID);
				}
			}
		}

		void AnimationComponent_SetInputVector3(uint64_t entityID, uint32_t inputId, const glm::vec3* value)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AnimationComponent>());

			auto& animationComponent = entity.GetComponent<AnimationComponent>();
			if (animationComponent.AnimationGraph)
			{
				try
				{
					auto& input = animationComponent.AnimationGraph->Ins.at(inputId);
					if (input.isObjectWithClassName(type::type_name<glm::vec3>()))
					{
						memcpy(input.getRawData(), value, sizeof(glm::vec3));
					}
					else
					{
						HZ_CONSOLE_LOG_ERROR("AnimationComponent.SetInputVector3() - input with id {0} is not of Vector3 type!", inputId);
					}
				}
				catch (const std::out_of_range&)
				{
					HZ_CONSOLE_LOG_ERROR("AnimationComponent.SetInputVector3() - input with id {0} does not exist!", inputId);
				}
			}
		}


		void AnimationComponent_SetInputTrigger(uint64_t entityID, uint32_t inputID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AnimationComponent>());

			auto& animationComponent = entity.GetComponent<AnimationComponent>();
			if (animationComponent.AnimationGraph)
			{
				try
				{
					animationComponent.AnimationGraph->InEvs.at(inputID).Event(inputID);
				}
				catch (const std::out_of_range&)
				{
					HZ_CONSOLE_LOG_ERROR("AnimationComponent.SetInputTrigger() - input with id {0} does not exist!", inputID);
				}
			}
		}


		void AnimationComponent_GetRootMotion(uint64_t entityID, Transform* outTransform)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AnimationComponent>());
			auto& animationComponent = entity.GetComponent<AnimationComponent>();
			if (animationComponent.AnimationGraph)
			{
				// RootMotion is at [0]th index of the pose
				Pose* pose = reinterpret_cast<Pose*>(animationComponent.AnimationGraph->EndpointOutputStreams.InValue(AnimationGraph::IDs::Pose).getRawData());
				outTransform->Translation = pose->RootMotion.Translation;
				outTransform->Rotation = glm::eulerAngles(pose->RootMotion.Rotation);
				outTransform->Scale = pose->RootMotion.Scale;
			}
		}

#pragma endregion

#pragma region SpotLightComponent

		void SpotLightComponent_GetRadiance(uint64_t entityID, glm::vec3* outRadiance)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpotLightComponent>());
			*outRadiance = entity.GetComponent<SpotLightComponent>().Radiance;
		}

		void SpotLightComponent_SetRadiance(uint64_t entityID, glm::vec3* inRadiance)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpotLightComponent>());
			HZ_ICALL_VALIDATE_PARAM_V(inRadiance, "nullptr");
			entity.GetComponent<SpotLightComponent>().Radiance = *inRadiance;
		}

		float SpotLightComponent_GetIntensity(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpotLightComponent>());
			return entity.GetComponent<SpotLightComponent>().Intensity;
		}

		void SpotLightComponent_SetIntensity(uint64_t entityID, float intensity)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpotLightComponent>());
			entity.GetComponent<SpotLightComponent>().Intensity = intensity;
		}

		float SpotLightComponent_GetRange(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpotLightComponent>());
			return entity.GetComponent<SpotLightComponent>().Range;
		}

		void SpotLightComponent_SetRange(uint64_t entityID, float range)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpotLightComponent>());
			entity.GetComponent<SpotLightComponent>().Range = range;
		}

		float SpotLightComponent_GetAngle(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpotLightComponent>());
			return entity.GetComponent<SpotLightComponent>().Angle;
		}

		void SpotLightComponent_SetAngle(uint64_t entityID, float angle)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpotLightComponent>());
			entity.GetComponent<SpotLightComponent>().Angle = angle;
		}

		float SpotLightComponent_GetFalloff(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpotLightComponent>());
			return entity.GetComponent<SpotLightComponent>().Falloff;
		}

		void SpotLightComponent_SetFalloff(uint64_t entityID, float falloff)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpotLightComponent>());
			entity.GetComponent<SpotLightComponent>().Falloff = falloff;
		}

		float SpotLightComponent_GetAngleAttenuation(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpotLightComponent>());
			return entity.GetComponent<SpotLightComponent>().AngleAttenuation;
		}

		void SpotLightComponent_SetAngleAttenuation(uint64_t entityID, float angleAttenuation)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpotLightComponent>());
			entity.GetComponent<SpotLightComponent>().AngleAttenuation = angleAttenuation;
		}

		bool SpotLightComponent_GetCastsShadows(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpotLightComponent>());
			return entity.GetComponent<SpotLightComponent>().CastsShadows;
		}

		void SpotLightComponent_SetCastsShadows(uint64_t entityID, bool castsShadows)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpotLightComponent>());
			entity.GetComponent<SpotLightComponent>().CastsShadows = castsShadows;
		}

		bool SpotLightComponent_GetSoftShadows(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpotLightComponent>());
			return entity.GetComponent<SpotLightComponent>().SoftShadows;
		}

		void SpotLightComponent_SetSoftShadows(uint64_t entityID, bool softShadows)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpotLightComponent>());
			entity.GetComponent<SpotLightComponent>().SoftShadows = softShadows;
		}

#pragma endregion

#pragma region ScriptComponent

		Coral::ManagedObject ScriptComponent_GetInstance(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			//HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<ScriptComponent>());
			if (!entity.HasComponent<ScriptComponent>())
			{
				HZ_CORE_ERROR("ScriptComponent_GetInstance no ScriptComponent apparently, returning null");
				return Coral::ManagedObject();
			}

			const auto& component = entity.GetComponent<ScriptComponent>();
#if 0
			if (!ScriptEngine::IsModuleValid(component.ScriptClassHandle))
			{
				ErrorWithTrace("Entity is referencing an invalid C# class!");
				return nullptr;
			}

			if (!ScriptEngine::IsEntityInstantiated(entity))
			{
				// Check if the entity is instantiated WITHOUT checking if the OnCreate method has run
				if (ScriptEngine::IsEntityInstantiated(entity, false))
				{
					// If so, call OnCreate here...
					ScriptEngine::CallMethod(component.ManagedInstance, "OnCreate");

					// NOTE(Peter): Don't use scriptComponent as a reference and modify it here
					//				If OnCreate spawns a lot of entities we would loose our reference
					//				to the script component...
					entity.GetComponent<ScriptComponent>().IsRuntimeInitialized = true;

					return GCManager::GetReferencedObject(component.ManagedInstance);
				}
				else if (component.ManagedInstance == nullptr)
				{
					ScriptEngine::RuntimeInitializeScriptEntity(entity);
					return GCManager::GetReferencedObject(component.ManagedInstance);
				}

				ErrorWithTrace("Entity '{0}' isn't instantiated?", entity.Name());
				return nullptr;
			}
#endif

			if (!component.Instance.IsValid())
			{
				HZ_CORE_ERROR("ScriptComponent_GetInstance returning null managed object");
				return Coral::ManagedObject();
			}

			HZ_CORE_VERIFY(component.Instance.IsValid());
			return *component.Instance.GetHandle();
		}

#pragma endregion

#pragma region CameraComponent

		void CameraComponent_SetPerspective(uint64_t entityID, float inVerticalFOV, float inNearClip, float inFarClip)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			SceneCamera& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetPerspective(inVerticalFOV, inNearClip, inFarClip);
		}

		void CameraComponent_SetOrthographic(uint64_t entityID, float inSize, float inNearClip, float inFarClip)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			SceneCamera& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetOrthographic(inSize, inNearClip, inFarClip);
		}

		float CameraComponent_GetVerticalFOV(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			const auto& component = entity.GetComponent<CameraComponent>();
			return component.Camera.GetDegPerspectiveVerticalFOV();
		}

		void CameraComponent_SetVerticalFOV(uint64_t entityID, float inVerticalFOV)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			auto& component = entity.GetComponent<CameraComponent>();
			return component.Camera.SetDegPerspectiveVerticalFOV(inVerticalFOV);
		}

		float CameraComponent_GetPerspectiveNearClip(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			SceneCamera& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetPerspectiveNearClip();
		}

		void CameraComponent_SetPerspectiveNearClip(uint64_t entityID, float inNearClip)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			SceneCamera& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetPerspectiveNearClip(inNearClip);
		}

		float CameraComponent_GetPerspectiveFarClip(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			SceneCamera& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetPerspectiveFarClip();
		}

		void CameraComponent_SetPerspectiveFarClip(uint64_t entityID, float inFarClip)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			SceneCamera& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetPerspectiveFarClip(inFarClip);
		}

		float CameraComponent_GetOrthographicSize(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			SceneCamera& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetOrthographicSize();
		}

		void CameraComponent_SetOrthographicSize(uint64_t entityID, float inSize)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			SceneCamera& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetOrthographicSize(inSize);
		}

		float CameraComponent_GetOrthographicNearClip(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			SceneCamera& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetOrthographicNearClip();
		}

		void CameraComponent_SetOrthographicNearClip(uint64_t entityID, float inNearClip)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			SceneCamera& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetOrthographicNearClip(inNearClip);
		}

		float CameraComponent_GetOrthographicFarClip(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			SceneCamera& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetOrthographicFarClip();
		}

		void CameraComponent_SetOrthographicFarClip(uint64_t entityID, float inFarClip)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			SceneCamera& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetOrthographicFarClip(inFarClip);
		}

		CameraComponent::Type CameraComponent_GetProjectionType(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			const auto& component = entity.GetComponent<CameraComponent>();
			return component.ProjectionType;
		}

		void CameraComponent_SetProjectionType(uint64_t entityID, CameraComponent::Type inType)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			auto& component = entity.GetComponent<CameraComponent>();
			component.ProjectionType = inType;
			component.Camera.SetProjectionType((SceneCamera::ProjectionType)inType);
		}

		bool CameraComponent_GetPrimary(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			const auto& component = entity.GetComponent<CameraComponent>();
			return component.Primary;
		}

		void CameraComponent_SetPrimary(uint64_t entityID, bool inValue)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			auto& component = entity.GetComponent<CameraComponent>();
			component.Primary = inValue;
		}

		void CameraComponent_ToScreenSpace(uint64_t entityID, glm::vec3* inWorldTranslation, glm::vec2* outResult)
		{
			Entity entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CameraComponent>());
			auto& component = entity.GetComponent<CameraComponent>();
			
			uint32_t viewportWidth = Application_GetWidth();
			uint32_t viewportHeight = Application_GetHeight();

			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			glm::mat4 cameraViewMatrix = glm::inverse(scene->GetWorldSpaceTransformMatrix(entity));

			glm::vec4 clipSpace = component.Camera.GetProjectionMatrix() * cameraViewMatrix * glm::vec4(*inWorldTranslation, 1.0f);
			clipSpace /= clipSpace.w;
			glm::vec2 screenSpace(clipSpace);
			screenSpace = screenSpace * 0.5f + 0.5f;
			screenSpace *= glm::vec2((float)viewportWidth, (float)viewportHeight);
			*outResult = screenSpace;
		}

#pragma endregion

#pragma region DirectionalLightComponent

		void DirectionalLightComponent_GetRadiance(uint64_t entityID, glm::vec3* outRadiance)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<DirectionalLightComponent>());
			*outRadiance = entity.GetComponent<DirectionalLightComponent>().Radiance;
		}

		void DirectionalLightComponent_SetRadiance(uint64_t entityID, glm::vec3* inRadiance)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<DirectionalLightComponent>());
			entity.GetComponent<DirectionalLightComponent>().Radiance = *inRadiance;
		}

		float DirectionalLightComponent_GetIntensity(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<DirectionalLightComponent>());
			return entity.GetComponent<DirectionalLightComponent>().Intensity;
		}

		void DirectionalLightComponent_SetIntensity(uint64_t entityID, float intensity)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<DirectionalLightComponent>());
			entity.GetComponent<DirectionalLightComponent>().Intensity = intensity;
		}

		bool DirectionalLightComponent_GetCastShadows(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<DirectionalLightComponent>());
			return entity.GetComponent<DirectionalLightComponent>().CastShadows;
		}

		void DirectionalLightComponent_SetCastShadows(uint64_t entityID, bool castShadows)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<DirectionalLightComponent>());
			entity.GetComponent<DirectionalLightComponent>().CastShadows = castShadows;
		}

		bool DirectionalLightComponent_GetSoftShadows(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<DirectionalLightComponent>());
			return entity.GetComponent<DirectionalLightComponent>().SoftShadows;
		}

		void DirectionalLightComponent_SetSoftShadows(uint64_t entityID, bool softShadows)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<DirectionalLightComponent>());
			entity.GetComponent<DirectionalLightComponent>().SoftShadows = softShadows;
		}

		float DirectionalLightComponent_GetLightSize(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<DirectionalLightComponent>());
			return entity.GetComponent<DirectionalLightComponent>().LightSize;
		}

		void DirectionalLightComponent_SetLightSize(uint64_t entityID, float lightSize)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<DirectionalLightComponent>());
			entity.GetComponent<DirectionalLightComponent>().LightSize = lightSize;
		}

#pragma endregion

#pragma region PointLightComponent

		void PointLightComponent_GetRadiance(uint64_t entityID, glm::vec3* outRadiance)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<PointLightComponent>());
			*outRadiance = entity.GetComponent<PointLightComponent>().Radiance;
		}

		void PointLightComponent_SetRadiance(uint64_t entityID, glm::vec3* inRadiance)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<PointLightComponent>());
			HZ_ICALL_VALIDATE_PARAM_V(inRadiance, "nullptr");
			entity.GetComponent<PointLightComponent>().Radiance = *inRadiance;
		}

		float PointLightComponent_GetIntensity(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<PointLightComponent>());
			return entity.GetComponent<PointLightComponent>().Intensity;
		}

		void PointLightComponent_SetIntensity(uint64_t entityID, float intensity)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<PointLightComponent>());
			entity.GetComponent<PointLightComponent>().Intensity = intensity;
		}

		float PointLightComponent_GetRadius(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<PointLightComponent>());
			return entity.GetComponent<PointLightComponent>().Radius;
		}

		void PointLightComponent_SetRadius(uint64_t entityID, float radius)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<PointLightComponent>());
			entity.GetComponent<PointLightComponent>().Radius = radius;
		}

		float PointLightComponent_GetFalloff(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<PointLightComponent>());
			return entity.GetComponent<PointLightComponent>().Falloff;
		}

		void PointLightComponent_SetFalloff(uint64_t entityID, float falloff)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<PointLightComponent>());
			entity.GetComponent<PointLightComponent>().Falloff = falloff;
		}

#pragma endregion

#pragma region SkyLightComponent

		float SkyLightComponent_GetIntensity(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SkyLightComponent>());
			return entity.GetComponent<SkyLightComponent>().Intensity;
		}

		void SkyLightComponent_SetIntensity(uint64_t entityID, float intensity)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SkyLightComponent>());
			entity.GetComponent<SkyLightComponent>().Intensity = intensity;
		}

		float SkyLightComponent_GetTurbidity(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SkyLightComponent>());
			return entity.GetComponent<SkyLightComponent>().TurbidityAzimuthInclination.x;
		}

		void SkyLightComponent_SetTurbidity(uint64_t entityID, float turbidity)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SkyLightComponent>());
			entity.GetComponent<SkyLightComponent>().TurbidityAzimuthInclination.x = turbidity;
		}

		float SkyLightComponent_GetAzimuth(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SkyLightComponent>());
			return entity.GetComponent<SkyLightComponent>().TurbidityAzimuthInclination.y;
		}

		void SkyLightComponent_SetAzimuth(uint64_t entityID, float azimuth)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SkyLightComponent>());
			entity.GetComponent<SkyLightComponent>().TurbidityAzimuthInclination.y = azimuth;
		}

		float SkyLightComponent_GetInclination(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SkyLightComponent>());
			return entity.GetComponent<SkyLightComponent>().TurbidityAzimuthInclination.z;
		}

		void SkyLightComponent_SetInclination(uint64_t entityID, float inclination)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SkyLightComponent>());
			entity.GetComponent<SkyLightComponent>().TurbidityAzimuthInclination.z = inclination;
		}

#pragma endregion

#pragma region SpriteRendererComponent

		void SpriteRendererComponent_GetColor(uint64_t entityID, glm::vec4* outColor)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpriteRendererComponent>());
			*outColor = entity.GetComponent<SpriteRendererComponent>().Color;
		}

		void SpriteRendererComponent_SetColor(uint64_t entityID, glm::vec4* inColor)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpriteRendererComponent>());
			entity.GetComponent<SpriteRendererComponent>().Color = *inColor;
		}

		float SpriteRendererComponent_GetTilingFactor(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpriteRendererComponent>());
			return entity.GetComponent<SpriteRendererComponent>().TilingFactor;
		}

		void SpriteRendererComponent_SetTilingFactor(uint64_t entityID, float tilingFactor)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpriteRendererComponent>());
			entity.GetComponent<SpriteRendererComponent>().TilingFactor = tilingFactor;
		}

		void SpriteRendererComponent_GetUVStart(uint64_t entityID, glm::vec2* outUVStart)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpriteRendererComponent>());
			*outUVStart = entity.GetComponent<SpriteRendererComponent>().UVStart;
		}

		void SpriteRendererComponent_SetUVStart(uint64_t entityID, glm::vec2* inUVStart)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpriteRendererComponent>());
			entity.GetComponent<SpriteRendererComponent>().UVStart = *inUVStart;
		}

		void SpriteRendererComponent_GetUVEnd(uint64_t entityID, glm::vec2* outUVEnd)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpriteRendererComponent>());
			*outUVEnd = entity.GetComponent<SpriteRendererComponent>().UVEnd;
		}

		void SpriteRendererComponent_SetUVEnd(uint64_t entityID, glm::vec2* inUVEnd)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SpriteRendererComponent>());
			entity.GetComponent<SpriteRendererComponent>().UVEnd = *inUVEnd;
		}

#pragma endregion

#pragma region RigidBody2DComponent

		RigidBody2DComponent::Type RigidBody2DComponent_GetBodyType(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBody2DComponent>());
			return entity.GetComponent<RigidBody2DComponent>().BodyType;
		}

		void RigidBody2DComponent_SetBodyType(uint64_t entityID, RigidBody2DComponent::Type inType)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBody2DComponent>());
			entity.GetComponent<RigidBody2DComponent>().BodyType = inType;
		}

		void RigidBody2DComponent_GetTranslation(uint64_t entityID, glm::vec2* outTranslation)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBody2DComponent>());

			const auto& component = entity.GetComponent<RigidBody2DComponent>();
			if (component.RuntimeBody == nullptr)
			{
				ErrorWithTrace("No b2Body exists for this entity!");
				*outTranslation = glm::vec2(0.0f);
				return;
			}

			b2Body* body = (b2Body*)component.RuntimeBody;
			const b2Vec2& translation = body->GetPosition();
			outTranslation->x = translation.x;
			outTranslation->y = translation.y;
		}

		void RigidBody2DComponent_SetTranslation(uint64_t entityID, glm::vec2* inTranslation)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBody2DComponent>());

			const auto& component = entity.GetComponent<RigidBody2DComponent>();
			if (component.RuntimeBody == nullptr)
			{
				ErrorWithTrace("No b2Body exists for this entity!");
				return;
			}

			b2Body* body = (b2Body*)component.RuntimeBody;
			body->SetTransform(b2Vec2(inTranslation->x, inTranslation->y), body->GetAngle());
		}

		float RigidBody2DComponent_GetRotation(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBody2DComponent>());

			const auto& component = entity.GetComponent<RigidBody2DComponent>();
			if (component.RuntimeBody == nullptr)
			{
				ErrorWithTrace("No b2Body exists for this entity!");
				return 0.0f;
			}

			return ((b2Body*)component.RuntimeBody)->GetAngle();
		}

		void RigidBody2DComponent_SetRotation(uint64_t entityID, float rotation)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBody2DComponent>());

			const auto& component = entity.GetComponent<RigidBody2DComponent>();
			if (component.RuntimeBody == nullptr)
			{
				ErrorWithTrace("No b2Body exists for this entity!");
				return;
			}

			b2Body* body = (b2Body*)component.RuntimeBody;
			body->SetTransform(body->GetPosition(), rotation);
		}

		float RigidBody2DComponent_GetMass(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBody2DComponent>());

			const auto& component = entity.GetComponent<RigidBody2DComponent>();
			if (component.RuntimeBody == nullptr)
			{
				ErrorWithTrace("No b2Body exists for this entity!");
				return 0.0f;
			}

			return ((b2Body*)component.RuntimeBody)->GetMass();
		}

		void RigidBody2DComponent_SetMass(uint64_t entityID, float mass)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBody2DComponent>());

			auto& component = entity.GetComponent<RigidBody2DComponent>();
			if (component.RuntimeBody == nullptr)
			{
				ErrorWithTrace("No b2Body exists for this entity!");
				return;
			}

			b2Body* body = (b2Body*)component.RuntimeBody;
			b2MassData massData;
			body->GetMassData(&massData);
			massData.mass = mass;
			body->SetMassData(&massData);

			component.Mass = mass;
		}

		void RigidBody2DComponent_GetLinearVelocity(uint64_t entityID, glm::vec2* outVelocity)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBody2DComponent>());

			const auto& component = entity.GetComponent<RigidBody2DComponent>();
			if (component.RuntimeBody == nullptr)
			{
				ErrorWithTrace("No b2Body exists for this entity!");
				return;
			}

			b2Body* body = (b2Body*)component.RuntimeBody;
			const b2Vec2& linearVelocity = body->GetLinearVelocity();
			outVelocity->x = linearVelocity.x;
			outVelocity->y = linearVelocity.y;
		}

		void RigidBody2DComponent_SetLinearVelocity(uint64_t entityID, glm::vec2* inVelocity)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBody2DComponent>());

			auto& component = entity.GetComponent<RigidBody2DComponent>();
			if (component.RuntimeBody == nullptr)
			{
				ErrorWithTrace("No b2Body exists for this entity!");
				return;
			}

			b2Body* body = (b2Body*)component.RuntimeBody;
			body->SetLinearVelocity(b2Vec2(inVelocity->x, inVelocity->y));
		}

		float RigidBody2DComponent_GetGravityScale(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBody2DComponent>());

			const auto& component = entity.GetComponent<RigidBody2DComponent>();
			if (component.RuntimeBody == nullptr)
			{
				ErrorWithTrace("No b2Body exists for this entity!");
				return 0.0f;
			}

			return ((b2Body*)component.RuntimeBody)->GetGravityScale();
		}

		void RigidBody2DComponent_SetGravityScale(uint64_t entityID, float gravityScale)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBody2DComponent>());

			auto& component = entity.GetComponent<RigidBody2DComponent>();
			if (component.RuntimeBody == nullptr)
			{
				ErrorWithTrace("No b2Body exists for this entity!");
				return;
			}

			b2Body* body = (b2Body*)component.RuntimeBody;
			body->SetGravityScale(gravityScale);
			component.GravityScale = gravityScale;
		}

		void RigidBody2DComponent_ApplyLinearImpulse(uint64_t entityID, glm::vec2* inImpulse, glm::vec2* inOffset, bool wake)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBody2DComponent>());

			auto& component = entity.GetComponent<RigidBody2DComponent>();
			if (component.RuntimeBody == nullptr)
			{
				ErrorWithTrace("No b2Body exists for this entity!");
				return;
			}

			if (component.BodyType != RigidBody2DComponent::Type::Dynamic)
			{
				ErrorWithTrace("Cannot add linear impulse to non-dynamic body!");
				return;
			}

			b2Body* body = (b2Body*)component.RuntimeBody;
			body->ApplyLinearImpulse(*(const b2Vec2*)inImpulse, body->GetWorldCenter() + *(const b2Vec2*)inOffset, wake);
		}

		void RigidBody2DComponent_ApplyAngularImpulse(uint64_t entityID, float impulse, bool wake)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBody2DComponent>());

			auto& component = entity.GetComponent<RigidBody2DComponent>();
			if (component.RuntimeBody == nullptr)
			{
				ErrorWithTrace("No b2Body exists for this entity!");
				return;
			}

			if (component.BodyType != RigidBody2DComponent::Type::Dynamic)
			{
				ErrorWithTrace("Cannot add angular impulse to non-dynamic body!");
				return;
			}

			b2Body* body = (b2Body*)component.RuntimeBody;
			body->ApplyAngularImpulse(impulse, wake);
		}

		void RigidBody2DComponent_AddForce(uint64_t entityID, glm::vec3* inForce, glm::vec3* inOffset, bool wake)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBody2DComponent>());

			auto& component = entity.GetComponent<RigidBody2DComponent>();
			if (component.RuntimeBody == nullptr)
			{
				ErrorWithTrace("No b2Body exists for this entity!");
				return;
			}

			if (component.BodyType != RigidBody2DComponent::Type::Dynamic)
			{
				ErrorWithTrace("Cannot apply force to non-dynamic body!");
				return;
			}

			b2Body* body = (b2Body*)component.RuntimeBody;
			body->ApplyForce(*(const b2Vec2*)inForce, body->GetWorldCenter() + *(const b2Vec2*)inOffset, wake);
		}

		void RigidBody2DComponent_AddTorque(uint64_t entityID, float torque, bool wake)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBody2DComponent>());

			auto& component = entity.GetComponent<RigidBody2DComponent>();
			if (component.RuntimeBody == nullptr)
			{
				ErrorWithTrace("No b2Body exists for this entity!");
				return;
			}

			if (component.BodyType != RigidBody2DComponent::Type::Dynamic)
			{
				ErrorWithTrace("Cannot apply torque to non-dynamic body!");
				return;
			}

			b2Body* body = (b2Body*)component.RuntimeBody;
			body->ApplyTorque(torque, wake);
		}

#pragma endregion

#pragma region RigidBodyComponent

		void RigidBodyComponent_AddForce(uint64_t entityID, glm::vec3* inForce, EForceMode forceMode)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);

			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			rigidBody->AddForce(*inForce, forceMode);
		}

		void RigidBodyComponent_AddForceAtLocation(uint64_t entityID, glm::vec3* inForce, glm::vec3* inLocation, EForceMode forceMode)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			rigidBody->AddForce(*inForce, *inLocation, forceMode);
		}

		void RigidBodyComponent_AddTorque(uint64_t entityID, glm::vec3* inTorque, EForceMode forceMode)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			rigidBody->AddTorque(*inTorque);
		}

		void RigidBodyComponent_GetLinearVelocity(uint64_t entityID, glm::vec3* outVelocity)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			*outVelocity = rigidBody->GetLinearVelocity();
		}

		void RigidBodyComponent_SetLinearVelocity(uint64_t entityID, glm::vec3* inVelocity)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			if (inVelocity == nullptr)
			{
				ErrorWithTrace("Cannot set linear velocity of RigidBody to null. Entity: '{}'", entity.Name());
				return;
			}

			rigidBody->SetLinearVelocity(*inVelocity);
		}

		void RigidBodyComponent_GetAngularVelocity(uint64_t entityID, glm::vec3* outVelocity)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			*outVelocity = rigidBody->GetAngularVelocity();
		}

		void RigidBodyComponent_SetAngularVelocity(uint64_t entityID, glm::vec3* inVelocity)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			if (inVelocity == nullptr)
			{
				ErrorWithTrace("Cannot set angular velocity to null for RigidBody '{}'", entity.Name());
				return;
			}

			rigidBody->SetAngularVelocity(*inVelocity);
		}

		float RigidBodyComponent_GetMaxLinearVelocity(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return 0.0f;
			}

			return rigidBody->GetMaxLinearVelocity();
		}

		void RigidBodyComponent_SetMaxLinearVelocity(uint64_t entityID, float maxVelocity)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			rigidBody->SetMaxLinearVelocity(maxVelocity);
			rigidBody->GetEntity().GetComponent<RigidBodyComponent>().MaxLinearVelocity = maxVelocity;
		}

		float RigidBodyComponent_GetMaxAngularVelocity(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return 0.0f;
			}

			return rigidBody->GetMaxAngularVelocity();
		}

		void RigidBodyComponent_SetMaxAngularVelocity(uint64_t entityID, float maxVelocity)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			rigidBody->SetMaxAngularVelocity(maxVelocity);
			rigidBody->GetEntity().GetComponent<RigidBodyComponent>().MaxAngularVelocity = maxVelocity;
		}

		float RigidBodyComponent_GetLinearDrag(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());
			return entity.GetComponent<RigidBodyComponent>().LinearDrag;
		}

		void RigidBodyComponent_SetLinearDrag(uint64_t entityID, float linearDrag)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			rigidBody->SetLinearDrag(linearDrag);

			entity.GetComponent<RigidBodyComponent>().LinearDrag = linearDrag;
		}

		float RigidBodyComponent_GetAngularDrag(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());
			return entity.GetComponent<RigidBodyComponent>().AngularDrag;
		}

		void RigidBodyComponent_SetAngularDrag(uint64_t entityID, float angularDrag)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			rigidBody->SetAngularDrag(angularDrag);

			entity.GetComponent<RigidBodyComponent>().AngularDrag = angularDrag;
		}

		void RigidBodyComponent_Rotate(uint64_t entityID, glm::vec3* inRotation)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			auto rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Can't rotate RigidBody because entity {} doesn't have a RigidBody!", entity.Name());
				return;
			}

			if (inRotation == nullptr)
			{
				ErrorWithTrace("Cannot rotate by 'null' rotation!");
				return;
			}

			rigidBody->Rotate(*inRotation);
		}

		uint32_t RigidBodyComponent_GetLayer(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());
			return entity.GetComponent<RigidBodyComponent>().LayerID;
		}

		void RigidBodyComponent_SetLayer(uint64_t entityID, uint32_t layerID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			if (!PhysicsLayerManager::IsLayerValid(layerID))
			{
				ErrorWithTrace("Invalid layer ID '{}'!", layerID);
				return;
			}

			rigidBody->SetCollisionLayer(layerID);
			auto& component = entity.GetComponent<RigidBodyComponent>();
			component.LayerID = layerID;
		}

		Coral::String RigidBodyComponent_GetLayerName(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			const auto& component = entity.GetComponent<RigidBodyComponent>();

			if (!PhysicsLayerManager::IsLayerValid(component.LayerID))
			{
				ErrorWithTrace("Can't find a layer with ID '{0}'!", component.LayerID);
				return Coral::String();
			}

			const auto& layer = PhysicsLayerManager::GetLayer(component.LayerID);
			return Coral::String::New(layer.Name);
		}

		void RigidBodyComponent_SetLayerByName(uint64_t entityID, Coral::String inName)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);

			if (rigidBody == nullptr)
			{
				ErrorWithTrace("Failed to find physics body for entity {}", entity.Name());
				return;
			}

			if (inName == Coral::String())
			{
				ErrorWithTrace("Name is null!");
				return;
			}

			if (!PhysicsLayerManager::IsLayerValid(inName))
			{
				ErrorWithTrace("Invalid layer name '{0}'!", std::string(inName));
				return;
			}

			const auto& layer = PhysicsLayerManager::GetLayer(inName);

			rigidBody->SetCollisionLayer(layer.LayerID);

			auto& component = entity.GetComponent<RigidBodyComponent>();
			component.LayerID = layer.LayerID;
		}

		float RigidBodyComponent_GetMass(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return 0.0f;
			}

			return rigidBody->GetMass();
		}

		void RigidBodyComponent_SetMass(uint64_t entityID, float mass)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			rigidBody->SetMass(mass);
		}

		EBodyType RigidBodyComponent_GetBodyType(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());
			return entity.GetComponent<RigidBodyComponent>().BodyType;
		}

		void RigidBodyComponent_SetBodyType(uint64_t entityID, EBodyType type)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			auto& rigidbodyComponent = entity.GetComponent<RigidBodyComponent>();

			// Don't bother doing anything if the type hasn't changed. It can be very expensive to recreate bodies.
			if (rigidbodyComponent.BodyType == type)
				return;

			if (!rigidbodyComponent.EnableDynamicTypeChange)
			{
				ErrorWithTrace("Cannot change the body type of a RigidBody during runtime. Please check \"Enable Dynamic Type Change\" in the component properties for entity {}", entity.Name());
				return;
			}

			rigidbodyComponent.BodyType = type;

			auto physicsScene = Scene::GetScene(entity.GetSceneUUID())->GetPhysicsScene();
			physicsScene->SetBodyType(entity, type);
		}

		bool RigidBodyComponent_IsTrigger(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return false;
			}

			return rigidBody->IsTrigger();
		}

		void RigidBodyComponent_SetTrigger(uint64_t entityID, bool isTrigger)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			rigidBody->ChangeTriggerState(isTrigger);
		}

		void RigidBodyComponent_MoveKinematic(uint64_t entityID, glm::vec3* inTargetPosition, glm::vec3* inTargetRotation, float inDeltaSeconds)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			if (inTargetPosition == nullptr || inTargetRotation == nullptr)
			{
				ErrorWithTrace("targetPosition or targetRotation is null!");
				return;
			}

			rigidBody->MoveKinematic(*inTargetPosition, glm::quat(*inTargetRotation), inDeltaSeconds);
		}

		void RigidBodyComponent_SetAxisLock(uint64_t entityID, EActorAxis axis, bool value, bool forceWake)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			rigidBody->SetAxisLock(axis, value, forceWake);
		}

		bool RigidBodyComponent_IsAxisLocked(uint64_t entityID, EActorAxis axis)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return false;
			}

			return rigidBody->IsAxisLocked(axis);
		}

		uint32_t RigidBodyComponent_GetLockedAxes(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return 0;
			}

			return static_cast<uint32_t>(rigidBody->GetLockedAxes());
		}

		bool RigidBodyComponent_IsSleeping(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return false;
			}

			return rigidBody->IsSleeping();
		}

		void RigidBodyComponent_SetIsSleeping(uint64_t entityID, bool isSleeping)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			rigidBody->SetSleepState(isSleeping);
		}

		void RigidBodyComponent_AddRadialImpulse(uint64_t entityID, glm::vec3* inOrigin, float radius, float strength, EFalloffMode falloff, bool velocityChange)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<RigidBodyComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			if (!rigidBody)
			{
				ErrorWithTrace("Couldn't find RigidBody for entity '{}'", entity.Name());
				return;
			}

			rigidBody->AddRadialImpulse(*inOrigin, radius, strength, falloff, velocityChange);
		}

#pragma endregion

#pragma region CharacterControllerComponent

		static inline Ref<CharacterController> GetPhysicsController(Entity entity)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_ASSERT(scene, "No scene active!");
			Ref<PhysicsScene> physicsScene = scene->GetPhysicsScene();
			HZ_CORE_ASSERT(physicsScene, "No physics scene active!");
			return physicsScene->GetCharacterController(entity);
		}

		bool CharacterControllerComponent_GetIsGravityEnabled(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CharacterControllerComponent>());

			auto controller = GetPhysicsController(entity);
			if (!controller)
			{
				ErrorWithTrace("Could not find CharacterController for entity '{}'!", entity.Name());
				return false;
			}

			return controller->IsGravityEnabled();
		}

		void CharacterControllerComponent_SetIsGravityEnabled(uint64_t entityID, bool enabled)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CharacterControllerComponent>());

			auto controller = GetPhysicsController(entity);
			if (!controller)
			{
				ErrorWithTrace("Could not find CharacterController for entity '{}'!", entity.Name());
				return;
			}

			controller->SetGravityEnabled(enabled);
		}

		float CharacterControllerComponent_GetSlopeLimit(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CharacterControllerComponent>());
			return entity.GetComponent<CharacterControllerComponent>().SlopeLimitDeg;
		}

		void CharacterControllerComponent_SetSlopeLimit(uint64_t entityID, float slopeLimit)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CharacterControllerComponent>());

			auto controller = GetPhysicsController(entity);
			if (!controller)
			{
				ErrorWithTrace("Could not find CharacterController for entity '{}'!", entity.Name());
				return;
			}

			auto slopeLimitClamped = glm::clamp(slopeLimit, 0.0f, 90.0f);
			entity.GetComponent<CharacterControllerComponent>().SlopeLimitDeg = slopeLimitClamped;
			controller->SetSlopeLimit(slopeLimitClamped);
		}

		float CharacterControllerComponent_GetStepOffset(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CharacterControllerComponent>());
			return entity.GetComponent<CharacterControllerComponent>().StepOffset;
		}

		void CharacterControllerComponent_SetStepOffset(uint64_t entityID, float stepOffset)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CharacterControllerComponent>());

			auto controller = GetPhysicsController(entity);
			if (!controller)
			{
				ErrorWithTrace("Could not find CharacterController for entity '{}'!", entity.Name());
				return;
			}

			entity.GetComponent<CharacterControllerComponent>().StepOffset = stepOffset;
			controller->SetStepOffset(stepOffset);
		}

		void CharacterControllerComponent_SetTranslation(uint64_t entityID, glm::vec3* inTranslation)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CharacterControllerComponent>());

			auto controller = GetPhysicsController(entity);
			if (!controller)
			{
				ErrorWithTrace("Could not find CharacterController for entity '{}'!", entity.Name());
				return;
			}

			if (inTranslation == nullptr)
			{
				ErrorWithTrace("Cannot set CharacterControllerComponent translation to a null vector!");
				return;
			}

			controller->SetTranslation(*inTranslation);
		}

		void CharacterControllerComponent_SetRotation(uint64_t entityID, glm::quat* inRotation)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CharacterControllerComponent>());

			auto controller = GetPhysicsController(entity);
			if (!controller)
			{
				ErrorWithTrace("Could not find CharacterController for entity '{}'!", entity.Name());
				return;
			}

			if (inRotation == nullptr)
			{
				ErrorWithTrace("Cannot set CharacterControllerComponent rotation to a null quaternion!");
				return;
			}

			controller->SetRotation(*inRotation);
		}

		void CharacterControllerComponent_Move(uint64_t entityID, glm::vec3* inDisplacement)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CharacterControllerComponent>());

			auto controller = GetPhysicsController(entity);
			if (!controller)
			{
				ErrorWithTrace("Could not find CharacterController for entity '{}'!", entity.Name());
				return;
			}

			if (inDisplacement == nullptr)
			{
				ErrorWithTrace("Cannot move CharacterControllerComponent by a null displacement!");
				return;
			}

			controller->Move(*inDisplacement);
		}

		void CharacterControllerComponent_Rotate(uint64_t entityID, glm::quat* inRotation)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CharacterControllerComponent>());

			auto controller = GetPhysicsController(entity);
			if (!controller)
			{
				ErrorWithTrace("Could not find CharacterController for entity '{}'!", entity.Name());
				return;
			}

			if (inRotation == nullptr)
			{
				ErrorWithTrace("Cannot rotate CharacterControllerComponent by a null rotation!");
				return;
			}

			controller->Rotate(*inRotation);
		}

		void CharacterControllerComponent_Jump(uint64_t entityID, float jumpPower)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CharacterControllerComponent>());

			auto controller = GetPhysicsController(entity);
			if (!controller)
			{
				ErrorWithTrace("Could not find CharacterController for entity '{}'!", entity.Name());
				return;
			}

			controller->Jump(jumpPower);
		}

		void CharacterControllerComponent_GetLinearVelocity(uint64_t entityID, glm::vec3* outVelocity)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CharacterControllerComponent>());

			auto controller = GetPhysicsController(entity);
			if (!controller)
			{
				ErrorWithTrace("Could not find CharacterController for entity '{}'!", entity.Name());
				return;
			}

			*outVelocity = controller->GetLinearVelocity();
		}

		void CharacterControllerComponent_SetLinearVelocity(uint64_t entityID, glm::vec3* inVelocity)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CharacterControllerComponent>());

			auto controller = GetPhysicsController(entity);
			if (!controller)
			{
				ErrorWithTrace("Could not find CharacterController for entity '{}'!", entity.Name());
				return;
			}

			controller->SetLinearVelocity(*inVelocity);
		}

		bool CharacterControllerComponent_IsGrounded(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CharacterControllerComponent>());

			auto controller = GetPhysicsController(entity);
			if (!controller)
			{
				ErrorWithTrace("Could not find CharacterController for entity '{}'!", entity.Name());
				return false;
			}

			return controller->IsGrounded();
		}

		ECollisionFlags CharacterControllerComponent_GetCollisionFlags(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CharacterControllerComponent>());

			auto controller = GetPhysicsController(entity);
			if (!controller)
			{
				ErrorWithTrace("Could not find CharacterController for entity '{}'!", entity.Name());
				return ECollisionFlags::None;
			}

			return controller->GetCollisionFlags();
		}

#pragma endregion

#pragma region BoxColliderComponent

		void BoxColliderComponent_GetHalfSize(uint64_t entityID, glm::vec3* outSize)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<BoxColliderComponent>());

			*outSize = entity.GetComponent<BoxColliderComponent>().HalfSize;
		}

		void BoxColliderComponent_GetOffset(uint64_t entityID, glm::vec3* outOffset)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<BoxColliderComponent>());

			*outOffset = entity.GetComponent<BoxColliderComponent>().Offset;
		}

		bool BoxColliderComponent_GetMaterial(uint64_t entityID, ColliderMaterial* outMaterial)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<BoxColliderComponent>());

			*outMaterial = entity.GetComponent<BoxColliderComponent>().Material;
			return true;
		}

		void BoxColliderComponent_SetMaterial(uint64_t entityID, ColliderMaterial* inMaterial)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<BoxColliderComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			HZ_CORE_VERIFY(rigidBody);

			for (uint32_t i = 0; i < rigidBody->GetShapeCount(ShapeType::Box); i++)
				rigidBody->GetShape(ShapeType::Box, i)->SetMaterial(*inMaterial);

			entity.GetComponent<BoxColliderComponent>().Material = *inMaterial;
		}

#pragma endregion

#pragma region SphereColliderComponent

		float SphereColliderComponent_GetRadius(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SphereColliderComponent>());
			return entity.GetComponent<SphereColliderComponent>().Radius;
		}

		void SphereColliderComponent_GetOffset(uint64_t entityID, glm::vec3* outOffset)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SphereColliderComponent>());

			*outOffset = entity.GetComponent<SphereColliderComponent>().Offset;
		}

		bool SphereColliderComponent_GetMaterial(uint64_t entityID, ColliderMaterial* outMaterial)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SphereColliderComponent>());

			*outMaterial = entity.GetComponent<SphereColliderComponent>().Material;
			return true;
		}

		void SphereColliderComponent_SetMaterial(uint64_t entityID, ColliderMaterial* inMaterial)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<SphereColliderComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			HZ_CORE_VERIFY(rigidBody);

			for (uint32_t i = 0; i < rigidBody->GetShapeCount(ShapeType::Sphere); i++)
				rigidBody->GetShape(ShapeType::Sphere, i)->SetMaterial(*inMaterial);

			entity.GetComponent<SphereColliderComponent>().Material = *inMaterial;
		}

#pragma endregion

#pragma region CapsuleColliderComponent

		float CapsuleColliderComponent_GetRadius(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CapsuleColliderComponent>());

			return entity.GetComponent<CapsuleColliderComponent>().Radius;
		}

		float CapsuleColliderComponent_GetHeight(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CapsuleColliderComponent>());

			return entity.GetComponent<CapsuleColliderComponent>().HalfHeight;
		}

		void CapsuleColliderComponent_GetOffset(uint64_t entityID, glm::vec3* outOffset)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CapsuleColliderComponent>());

			*outOffset = entity.GetComponent<CapsuleColliderComponent>().Offset;
		}

		bool CapsuleColliderComponent_GetMaterial(uint64_t entityID, ColliderMaterial* outMaterial)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CapsuleColliderComponent>());

			*outMaterial = entity.GetComponent<CapsuleColliderComponent>().Material;
			return true;
		}

		void CapsuleColliderComponent_SetMaterial(uint64_t entityID, ColliderMaterial* inMaterial)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<CapsuleColliderComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			HZ_CORE_VERIFY(rigidBody);

			for (uint32_t i = 0; i < rigidBody->GetShapeCount(ShapeType::Capsule); i++)
				rigidBody->GetShape(ShapeType::Capsule, i)->SetMaterial(*inMaterial);

			entity.GetComponent<CapsuleColliderComponent>().Material = *inMaterial;
		}

#pragma endregion

#pragma region MeshColliderComponent

		bool MeshColliderComponent_IsMeshStatic(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<MeshColliderComponent>());

			const auto& component = entity.GetComponent<MeshColliderComponent>();
			Ref<MeshColliderAsset> colliderAsset = AssetManager::GetAsset<MeshColliderAsset>(component.ColliderAsset);

			if (!colliderAsset)
			{
				ErrorWithTrace("Invalid collider asset!");
				return false;
			}

			if (!AssetManager::IsAssetHandleValid(colliderAsset->ColliderMesh))
			{
				ErrorWithTrace("Invalid mesh!");
				return false;
			}

			return AssetManager::GetAssetType(colliderAsset->ColliderMesh) == AssetType::StaticMesh;
		}

		bool MeshColliderComponent_IsColliderMeshValid(uint64_t entityID, Param<AssetHandle> meshHandle)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<MeshColliderComponent>());

			const auto& component = entity.GetComponent<MeshColliderComponent>();
			Ref<MeshColliderAsset> colliderAsset = AssetManager::GetAsset<MeshColliderAsset>(component.ColliderAsset);

			if (!colliderAsset)
			{
				ErrorWithTrace("Invalid collider asset!");
				return false;
			}

			return static_cast<AssetHandle>(meshHandle) == colliderAsset->ColliderMesh;
		}

		bool MeshColliderComponent_GetColliderMesh(uint64_t entityID, OutParam<AssetHandle> outHandle)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<MeshColliderComponent>());

			const auto& component = entity.GetComponent<MeshColliderComponent>();
			Ref<MeshColliderAsset> colliderAsset = AssetManager::GetAsset<MeshColliderAsset>(component.ColliderAsset);

			if (!colliderAsset)
			{
				ErrorWithTrace("Invalid collider asset!");
				return false;
			}

			if (!AssetManager::IsAssetHandleValid(colliderAsset->ColliderMesh))
			{
				ErrorWithTrace("This component doesn't have a valid collider mesh!");
				*outHandle = AssetHandle(0);
				return false;
			}

			*outHandle = colliderAsset->ColliderMesh;
			return true;
		}

		bool MeshColliderComponent_GetMaterial(uint64_t entityID, ColliderMaterial* outMaterial)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<MeshColliderComponent>());

			*outMaterial = entity.GetComponent<MeshColliderComponent>().Material;
			return true;
		}

		void MeshColliderComponent_SetMaterial(uint64_t entityID, ColliderMaterial* inMaterial)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<MeshColliderComponent>());

			Ref<PhysicsBody> rigidBody = GetRigidBody(entityID);
			HZ_CORE_VERIFY(rigidBody);

			for (uint32_t i = 0; i < rigidBody->GetShapeCount(ShapeType::ConvexMesh); i++)
				rigidBody->GetShape(ShapeType::ConvexMesh, i)->SetMaterial(*inMaterial);

			for (uint32_t i = 0; i < rigidBody->GetShapeCount(ShapeType::TriangleMesh); i++)
				rigidBody->GetShape(ShapeType::TriangleMesh, i)->SetMaterial(*inMaterial);

			entity.GetComponent<MeshColliderComponent>().Material = *inMaterial;
		}

#pragma endregion

#pragma region MeshCollider

		bool MeshCollider_IsStaticMesh(Param<AssetHandle> meshHandle)
		{
			if (!AssetManager::IsAssetHandleValid(meshHandle))
				return false;

			if (AssetManager::GetAssetType(meshHandle) != AssetType::StaticMesh && AssetManager::GetAssetType(meshHandle) != AssetType::Mesh)
			{
				WarnWithTrace("MeshCollider recieved AssetHandle to a non-mesh asset?");
				return false;
			}

			return AssetManager::GetAssetType(meshHandle) == AssetType::StaticMesh;
		}

#pragma endregion

#pragma region AudioComponent

		bool AudioComponent_IsPlaying(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AudioComponent>());

			return AudioPlayback::IsPlaying(entityID);
		}

		bool AudioComponent_Play(uint64_t entityID, float startTime)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AudioComponent>());

			return AudioPlayback::Play(entityID, startTime);
		}

		bool AudioComponent_Stop(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AudioComponent>());

			return AudioPlayback::StopActiveSound(entityID);
		}

		bool AudioComponent_Pause(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AudioComponent>());

			return AudioPlayback::PauseActiveSound(entityID);
		}

		bool AudioComponent_Resume(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AudioComponent>());

			return AudioPlayback::Resume(entityID);
		}

		float AudioComponent_GetVolumeMult(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AudioComponent>());

			return entity.GetComponent<AudioComponent>().VolumeMultiplier;
		}

		void AudioComponent_SetVolumeMult(uint64_t entityID, float volumeMultiplier)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AudioComponent>());

			entity.GetComponent<AudioComponent>().VolumeMultiplier = volumeMultiplier;
		}

		float AudioComponent_GetPitchMult(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AudioComponent>());

			return entity.GetComponent<AudioComponent>().PitchMultiplier;
		}

		void AudioComponent_SetPitchMult(uint64_t entityID, float pitchMultiplier)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AudioComponent>());

			entity.GetComponent<AudioComponent>().PitchMultiplier = pitchMultiplier;
		}

		void AudioComponent_SetEvent(uint64_t entityID, Audio::CommandID eventID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<AudioComponent>());

			if (!AudioCommandRegistry::DoesCommandExist<Audio::TriggerCommand>(eventID))
			{
				ErrorWithTrace("TriggerCommand with ID {0} does not exist!", (uint32_t)eventID);
				return;
			}

			auto& component = entity.GetComponent<AudioComponent>();
			component.StartCommandID = eventID;
			component.StartEvent = AudioCommandRegistry::GetCommand<Audio::TriggerCommand>(eventID).DebugName;
		}

#pragma endregion

#pragma region TextComponent

		size_t TextComponent_GetHash(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<TextComponent>());

			return entity.GetComponent<TextComponent>().TextHash;
		}

		Coral::String TextComponent_GetText(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<TextComponent>());

			const auto& component = entity.GetComponent<TextComponent>();
			return Coral::String::New(component.TextString);
		}

		void TextComponent_SetText(uint64_t entityID, Coral::String text)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<TextComponent>());

			auto& component = entity.GetComponent<TextComponent>();
			component.TextString = text;
			component.TextHash = std::hash<std::string>()(component.TextString);
		}

		void TextComponent_GetColor(uint64_t entityID, glm::vec4* outColor)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<TextComponent>());

			const auto& component = entity.GetComponent<TextComponent>();
			*outColor = component.Color;
		}

		void TextComponent_SetColor(uint64_t entityID, glm::vec4* inColor)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);
			HZ_ICALL_VALIDATE_PARAM(entity.HasComponent<TextComponent>());

			auto& component = entity.GetComponent<TextComponent>();
			component.Color = *inColor;
		}

#pragma endregion

#pragma region Audio

		uint32_t Audio_PostEvent(Audio::CommandID eventID, uint64_t entityID)
		{
			if (!AudioCommandRegistry::DoesCommandExist<Audio::TriggerCommand>(eventID))
				return 0;

			return AudioPlayback::PostTrigger(eventID, entityID);
		}

		uint32_t Audio_PostEventFromAC(Audio::CommandID eventID, uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);


			if (!AudioCommandRegistry::DoesCommandExist<Audio::TriggerCommand>(eventID))
			{
				ErrorWithTrace("Unable to find TriggerCommand with ID {0}", (uint32_t)eventID);
				return 0;
			}

			return AudioPlayback::PostTriggerFromAC(eventID, entityID);
		}

		uint32_t Audio_PostEventAtLocation(Audio::CommandID eventID, Transform* inLocation)
		{
			if (!AudioCommandRegistry::DoesCommandExist<Audio::TriggerCommand>(eventID))
			{
				ErrorWithTrace("Unable to find TriggerCommand with ID {0}", (uint32_t)eventID);
				return 0;
			}

			if (inLocation == nullptr)
			{
				ErrorWithTrace("Cannot post audio event at a location of 'null'!");
				return 0;
			}

			return AudioPlayback::PostTriggerAtLocation(eventID, inLocation->Translation, inLocation->Rotation, inLocation->Scale);
		}

		bool Audio_StopEventID(uint32_t playingEventID)
		{
			return AudioPlayback::StopEventID(playingEventID);
		}

		bool Audio_PauseEventID(uint32_t playingEventID)
		{
			return AudioPlayback::PauseEventID(playingEventID);
		}

		bool Audio_ResumeEventID(uint32_t playingEventID)
		{
			return AudioPlayback::ResumeEventID(playingEventID);
		}

		uint64_t Audio_CreateAudioEntity(Audio::CommandID eventID, Transform* inLocation, float volume, float pitch)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_VERIFY(scene, "No active scene");
			Entity entity = scene->CreateEntity("AudioEntity");

			if (!AudioCommandRegistry::DoesCommandExist<Audio::TriggerCommand>(eventID))
			{
				ErrorWithTrace("Unable to find TriggerCommand with ID {0}", (uint32_t)eventID);
				return 0;
			}

			auto& component = entity.AddComponent<AudioComponent>();
			component.StartCommandID = eventID;
			component.StartEvent = AudioCommandRegistry::GetCommand<Audio::TriggerCommand>(eventID).DebugName;
			component.VolumeMultiplier = volume;
			component.PitchMultiplier = pitch;

			return entity.GetUUID();
		}

#pragma endregion

#pragma region AudioCommandID

		uint32_t AudioCommandID_Constructor(Coral::String inCommandName)
		{
			std::string commandName = inCommandName;
			return Audio::CommandID(commandName.c_str());
		}

#pragma endregion

#pragma region AudioParameters
		//============================================================================================
		/// Audio Parameters Interface
		void Audio_SetParameterFloat(Audio::CommandID parameterID, uint64_t objectID, float value)
		{
			return AudioPlayback::SetParameterFloat(parameterID, objectID, value);
		}

		void Audio_SetParameterInt(Audio::CommandID parameterID, uint64_t objectID, int value)
		{
			return AudioPlayback::SetParameterInt(parameterID, objectID, value);
		}

		void Audio_SetParameterBool(Audio::CommandID parameterID, uint64_t objectID, bool value)
		{
			return AudioPlayback::SetParameterBool(parameterID, objectID, value);

		}

		void Audio_SetParameterFloatForAC(Audio::CommandID parameterID, uint64_t entityID, float value)
		{
			return AudioPlayback::SetParameterFloatForAC(parameterID, entityID, value);
		}

		void Audio_SetParameterIntForAC(Audio::CommandID parameterID, uint64_t entityID, int value)
		{
			return AudioPlayback::SetParameterIntForAC(parameterID, entityID, value);
		}

		void Audio_SetParameterBoolForAC(Audio::CommandID parameterID, uint64_t entityID, bool value)
		{
			return AudioPlayback::SetParameterBoolForAC(parameterID, entityID, value);
		}

		void Audio_SetParameterFloatForEvent(Audio::CommandID parameterID, uint32_t eventID, float value)
		{
			AudioPlayback::SetParameterFloat(parameterID, eventID, value);
		}

		void Audio_SetParameterIntForEvent(Audio::CommandID parameterID, uint32_t eventID, int value)
		{
			AudioPlayback::SetParameterInt(parameterID, eventID, value);
		}

		void Audio_SetParameterBoolForEvent(Audio::CommandID parameterID, uint32_t eventID, bool value)
		{
			AudioPlayback::SetParameterBool(parameterID, eventID, value);
		}

		//============================================================================================
		void Audio_PreloadEventSources(Audio::CommandID eventID)
		{
			AudioPlayback::PreloadEventSources(eventID);
		}

		void Audio_UnloadEventSources(Audio::CommandID eventID)
		{
			AudioPlayback::UnloadEventSources(eventID);
		}

		void Audio_SetLowPassFilterValue(uint64_t objectID, float value)
		{
			AudioPlayback::SetLowPassFilterValueObj(objectID, value);
		}

		void Audio_SetHighPassFilterValue(uint64_t objectID, float value)
		{
			AudioPlayback::SetHighPassFilterValueObj(objectID, value);
		}

		void Audio_SetLowPassFilterValue_Event(Audio::CommandID eventID, float value)
		{
			AudioPlayback::SetLowPassFilterValue(eventID, value);
		}

		void Audio_SetHighPassFilterValue_Event(Audio::CommandID eventID, float value)
		{
			AudioPlayback::SetHighPassFilterValue(eventID, value);
		}

		void Audio_SetLowPassFilterValue_AC(uint64_t entityID, float value)
		{
			AudioPlayback::SetLowPassFilterValueAC(entityID, value);
		}

		void Audio_SetHighPassFilterValue_AC(uint64_t entityID, float value)
		{
			AudioPlayback::SetHighPassFilterValueAC(entityID, value);
		}

#pragma endregion

#pragma region Texture2D

		bool Texture2D_Create(uint32_t width, uint32_t height, TextureWrap wrapMode, TextureFilter filterMode, OutParam<AssetHandle> outHandle)
		{
			TextureSpecification spec;
			spec.Width = width;
			spec.Height = height;
			spec.SamplerWrap = wrapMode;
			spec.SamplerFilter = filterMode;

			auto result = Texture2D::Create(spec);
			*outHandle = AssetManager::AddMemoryOnlyAsset<Texture2D>(result);
			return true;
		}

		void Texture2D_GetSize(Param<AssetHandle> inHandle, uint32_t* outWidth, uint32_t* outHeight)
		{
			Ref<Texture2D> instance = AssetManager::GetAsset<Texture2D>(inHandle);
			if (!instance)
			{
				ErrorWithTrace("Tried to get texture size using an invalid handle!");
				return;
			}

			*outWidth = instance->GetWidth();
			*outHeight = instance->GetHeight();
		}

		void Texture2D_SetData(Param<AssetHandle> inHandle, Coral::Array<glm::vec4> inData)
		{
			Ref<Texture2D> instance = AssetManager::GetAsset<Texture2D>(inHandle);

			if (!instance)
			{
				ErrorWithTrace("Tried to set texture data in an invalid texture!");
				return;
			}

			size_t length = inData.Length();
			uint32_t dataSize = (uint32_t)(length * sizeof(glm::vec4) / 4);

			instance->Lock();
			Buffer buffer = instance->GetWriteableBuffer();
			HZ_CORE_VERIFY(dataSize <= buffer.Size);
			// Convert RGBA32F color to RGBA8
			uint8_t* pixels = (uint8_t*)buffer.Data;
			uint32_t index = 0;

			for (uint32_t i = 0; i < instance->GetWidth() * instance->GetHeight(); i++)
			{
				const auto& value = inData[i];
				*pixels++ = (uint32_t)(value.x * 255.0f);
				*pixels++ = (uint32_t)(value.y * 255.0f);
				*pixels++ = (uint32_t)(value.z * 255.0f);
				*pixels++ = (uint32_t)(value.w * 255.0f);
			}

			instance->Unlock();
		}

		// TODO(Peter): Uncomment when Hazel can actually read texture data from the CPU or when image data is persistently stored in RAM
		/*MonoArray* Texture2D_GetData(OutParam<AssetHandle> inHandle)
		{
			Ref<Texture2D> instance = AssetManager::GetAsset<Texture2D>(*inHandle);

			if (!instance)
			{
				ErrorWithTrace("Tried to get texture data for an invalid texture!");
				return nullptr;
			}

			uint32_t width = instance->GetWidth();
			uint32_t height = instance->GetHeight();
			ManagedArray result = ManagedArray::Create<glm::vec4>(width * height);

			instance->Lock();
			Buffer buffer = instance->GetImage()->GetBuffer();
			uint8_t* pixels = (uint8_t*)buffer.Data;

			for (uint32_t i = 0; i < width * height; i++)
			{
				glm::vec4 value;
				value.r = (float)*pixels++ / 255.0f;
				value.g = (float)*pixels++ / 255.0f;
				value.b = (float)*pixels++ / 255.0f;
				value.a = (float)*pixels++ / 255.0f;

				result.Set(i, value);
			}

			instance->Unlock();
			return result;
		}*/

#pragma endregion

#pragma region Mesh

		bool Mesh_GetMaterialByIndex(Param<AssetHandle> meshHandle, int index, OutParam<AssetHandle> outHandle)
		{
			if (!AssetManager::IsAssetValid(meshHandle))
			{
				ErrorWithTrace("Invalid Mesh instance!");
				*outHandle = AssetHandle(0);
				return false;
			}

			Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(meshHandle);

			Ref<MaterialTable> materialTable = mesh->GetMaterials();
			if (materialTable == nullptr)
			{
				ErrorWithTrace("Mesh has no materials!");
				*outHandle = AssetHandle(0);
				return false;
			}

			if (materialTable->GetMaterialCount() == 0)
			{
				*outHandle = AssetHandle(0);
				return false;
			}

			if ((uint32_t)index >= materialTable->GetMaterialCount())
			{
				ErrorWithTrace("Material index out of range. Index: {0}, MaxIndex: {1}", index, materialTable->GetMaterialCount() - 1);
				*outHandle = AssetHandle(0);
				return false;
			}

			*outHandle = materialTable->GetMaterial(index);
			return true;
		}

		int Mesh_GetMaterialCount(Param<AssetHandle> meshHandle)
		{
			if (!AssetManager::IsAssetValid(meshHandle))
			{
				ErrorWithTrace("Invalid Mesh instance!");
				return 0;
			}

			Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(meshHandle);
			Ref<MaterialTable> materialTable = mesh->GetMaterials();
			if (materialTable == nullptr)
				return 0;

			return materialTable->GetMaterialCount();
		}

#pragma endregion

#pragma region StaticMesh

		bool StaticMesh_GetMaterialByIndex(Param<AssetHandle> meshHandle, int index, OutParam<AssetHandle> outHandle)
		{
			if (!AssetManager::IsAssetValid(meshHandle))
			{
				ErrorWithTrace("Invalid Mesh instance!");
				*outHandle = AssetHandle(0);
				return false;
			}

			Ref<StaticMesh> mesh = AssetManager::GetAsset<StaticMesh>(meshHandle);

			Ref<MaterialTable> materialTable = mesh->GetMaterials();
			if (materialTable == nullptr)
			{
				ErrorWithTrace("Mesh has no materials!");
				*outHandle = AssetHandle(0);
				return false;
			}

			if (materialTable->GetMaterialCount() == 0)
			{
				*outHandle = AssetHandle(0);
				return false;
			}

			if ((uint32_t)index >= materialTable->GetMaterialCount())
			{
				ErrorWithTrace("Material index out of range. Index: {0}, MaxIndex: {1}", index, materialTable->GetMaterialCount() - 1);
				*outHandle = AssetHandle(0);
				return false;
			}

			*outHandle = materialTable->GetMaterial(index);
			return true;
		}

		int StaticMesh_GetMaterialCount(Param<AssetHandle> meshHandle)
		{
			if (!AssetManager::IsAssetValid(meshHandle))
			{
				ErrorWithTrace("Invalid Mesh instance!");
				return 0;
			}

			Ref<StaticMesh> mesh = AssetManager::GetAsset<StaticMesh>(meshHandle);
			Ref<MaterialTable> materialTable = mesh->GetMaterials();
			if (materialTable == nullptr)
				return 0;

			return materialTable->GetMaterialCount();
		}

#pragma endregion

#pragma region Material

		static Ref<MaterialAsset> Material_GetMaterialAsset(const char* functionName, uint64_t entityID, AssetHandle meshHandle, AssetHandle materialHandle)
		{
			if (!AssetManager::IsAssetHandleValid(meshHandle))
			{
				// NOTE(Peter): This means the material is expected to be an actual asset, referenced directly
				Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(materialHandle);

				if (!material)
				{
					ErrorWithTrace("Failed to get a material asset with handle {}, no such asset exists!", materialHandle);
					return nullptr;
				}

				return material;
			}

			Ref<MaterialTable> materialTable = nullptr;
			if (entityID == 0)
			{

				if (AssetManager::GetAssetType(meshHandle) == AssetType::Mesh)
				{
					auto mesh = AssetManager::GetAsset<Mesh>(meshHandle);

					if (!mesh)
					{
						ErrorWithTrace("Invalid mesh instance!");
						return nullptr;
					}

					materialTable = mesh->GetMaterials();
				}
				else if (AssetManager::GetAssetType(meshHandle) == AssetType::StaticMesh)
				{
					auto staticMesh = AssetManager::GetAsset<StaticMesh>(meshHandle);

					if (!staticMesh)
					{
						ErrorWithTrace("Invalid mesh instance!");
						return nullptr;
					}

					materialTable = staticMesh->GetMaterials();
				}
				else
				{
					ErrorWithTrace("meshHandle doesn't correspond with a Mesh? AssetType: {}", Hazel::Utils::AssetTypeToString(AssetManager::GetAssetType(meshHandle)));
					return nullptr;
				}
			}
			else
			{
				// This material is expected to be on a component
				auto entity = GetEntity(entityID);
				HZ_ICALL_VALIDATE_PARAM_V(entity, entityID);

				if (entity.HasComponent<SubmeshComponent>())
				{
					materialTable = entity.GetComponent<SubmeshComponent>().MaterialTable;
				}
				else if (entity.HasComponent<StaticMeshComponent>())
				{
					materialTable = entity.GetComponent<StaticMeshComponent>().MaterialTable;
				}
				else
				{
					ErrorWithTrace("Invalid component!");
					return nullptr;
				}
			}

			if (materialTable == nullptr || materialTable->GetMaterialCount() == 0)
			{
				ErrorWithTrace("Mesh has no materials!");
				return nullptr;
			}

			Ref<MaterialAsset> materialInstance = nullptr;

			for (const auto& [materialIndex, material] : materialTable->GetMaterials())
			{
				if (material == materialHandle)
				{
					materialInstance = AssetManager::GetAsset<MaterialAsset>(material);
					break;
				}
			}

			if (materialInstance == nullptr)
				ErrorWithTrace("This appears to be an invalid Material!");

			return materialInstance;
		}

		void Material_GetAlbedoColor(uint64_t entityID, Param<AssetHandle> meshHandle, Param<AssetHandle> materialHandle, glm::vec3* outAlbedoColor)
		{
			Ref<MaterialAsset> materialInstance = Material_GetMaterialAsset("Material.GetAlbedoColor", entityID, meshHandle, materialHandle);

			if (materialInstance == nullptr)
			{
				*outAlbedoColor = glm::vec3(1.0f, 0.0f, 1.0f);
				return;
			}

			*outAlbedoColor = materialInstance->GetAlbedoColor();
		}

		void Material_SetAlbedoColor(uint64_t entityID, Param<AssetHandle> meshHandle, Param<AssetHandle> materialHandle, glm::vec3* inAlbedoColor)
		{
			Ref<MaterialAsset> materialInstance = Material_GetMaterialAsset("Material.SetAlbedoColor", entityID, meshHandle, materialHandle);

			if (materialInstance == nullptr)
				return;

			materialInstance->SetAlbedoColor(*inAlbedoColor);
		}

		float Material_GetMetalness(uint64_t entityID, Param<AssetHandle> meshHandle, Param<AssetHandle> materialHandle)
		{
			Ref<MaterialAsset> materialInstance = Material_GetMaterialAsset("Material.GetMetalness", entityID, meshHandle, materialHandle);

			if (materialInstance == nullptr)
				return 0.0f;

			return materialInstance->GetMetalness();
		}

		void Material_SetMetalness(uint64_t entityID, Param<AssetHandle> meshHandle, Param<AssetHandle> materialHandle, float inMetalness)
		{
			Ref<MaterialAsset> materialInstance = Material_GetMaterialAsset("Material.SetMetalness", entityID, meshHandle, materialHandle);

			if (materialInstance == nullptr)
				return;

			materialInstance->SetMetalness(inMetalness);
		}

		float Material_GetRoughness(uint64_t entityID, Param<AssetHandle> meshHandle, Param<AssetHandle> materialHandle)
		{
			Ref<MaterialAsset> materialInstance = Material_GetMaterialAsset("Material.GetRoughness", entityID, meshHandle, materialHandle);

			if (materialInstance == nullptr)
				return 0.0f;

			return materialInstance->GetRoughness();
		}

		void Material_SetRoughness(uint64_t entityID, Param<AssetHandle> meshHandle, Param<AssetHandle> materialHandle, float inRoughness)
		{
			Ref<MaterialAsset> materialInstance = Material_GetMaterialAsset("Material.SetRoughness", entityID, meshHandle, materialHandle);

			if (materialInstance == nullptr)
				return;

			materialInstance->SetRoughness(inRoughness);
		}

		float Material_GetEmission(uint64_t entityID, Param<AssetHandle> meshHandle, Param<AssetHandle> materialHandle)
		{
			Ref<MaterialAsset> materialInstance = Material_GetMaterialAsset("Material.GetEmission", entityID, meshHandle, materialHandle);

			if (materialInstance == nullptr)
				return 0.0f;

			return materialInstance->GetEmission();
		}

		void Material_SetEmission(uint64_t entityID, Param<AssetHandle> meshHandle, Param<AssetHandle> materialHandle, float inEmission)
		{
			Ref<MaterialAsset> materialInstance = Material_GetMaterialAsset("Material.SetEmission", entityID, meshHandle, materialHandle);

			if (materialInstance == nullptr)
				return;

			materialInstance->SetEmission(inEmission);
		}

		void Material_SetFloat(uint64_t entityID, Param<AssetHandle> meshHandle, Param<AssetHandle> materialHandle, Coral::String inUniform, float value)
		{
			Ref<MaterialAsset> materialInstance = Material_GetMaterialAsset("Material.Set", entityID, meshHandle, materialHandle);

			if (materialInstance == nullptr)
				return;

			std::string uniformName = inUniform;
			if (uniformName.empty())
			{
				WarnWithTrace("Cannot set uniform with empty name!");
				return;
			}

			materialInstance->GetMaterial()->Set(uniformName, value);
		}

		void Material_SetVector3(uint64_t entityID, Param<AssetHandle> meshHandle, Param<AssetHandle> materialHandle, Coral::String inUniform, glm::vec3* inValue)
		{
			Ref<MaterialAsset> materialInstance = Material_GetMaterialAsset("Material.Set", entityID, meshHandle, materialHandle);

			if (materialInstance == nullptr)
				return;

			std::string uniformName = inUniform;
			if (uniformName.empty())
			{
				WarnWithTrace("Cannot set uniform with empty name!");
				return;
			}

			materialInstance->GetMaterial()->Set(uniformName, *inValue);
		}

		void Material_SetVector4(uint64_t entityID, Param<AssetHandle> meshHandle, Param<AssetHandle> materialHandle, Coral::String inUniform, glm::vec3* inValue)
		{
			Ref<MaterialAsset> materialInstance = Material_GetMaterialAsset("Material.Set", entityID, meshHandle, materialHandle);

			if (materialInstance == nullptr)
				return;

			std::string uniformName = inUniform;
			if (uniformName.empty())
			{
				WarnWithTrace("Cannot set uniform with empty name!");
				return;
			}

			materialInstance->GetMaterial()->Set(uniformName, *inValue);
		}

		void Material_SetTexture(uint64_t entityID, Param<AssetHandle> meshHandle, Param<AssetHandle> materialHandle, Coral::String inUniform, Param<AssetHandle> inTexture)
		{
			Ref<MaterialAsset> materialInstance = Material_GetMaterialAsset("Material.Set", entityID, meshHandle, materialHandle);

			if (materialInstance == nullptr)
				return;

			std::string uniformName = inUniform;
			if (uniformName.empty())
			{
				WarnWithTrace("Cannot set uniform with empty name!");
				return;
			}

			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(inTexture);

			if (!texture)
			{
				ErrorWithTrace("Tried to set an invalid texture instance");
				return;
			}

			materialInstance->GetMaterial()->Set(uniformName, texture);
		}

#pragma endregion

#pragma region MeshFactory

		void* MeshFactory_CreatePlane(float width, float height)
		{
			// TODO: MeshFactory::CreatePlane(width, height, subdivisions)!
			return nullptr;
		}

#pragma endregion

#pragma region Physics

		bool Physics_CastRay(RaycastData* inRaycastData, ScriptRaycastHit* outHit)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_ASSERT(scene, "No active scene!");

			if (scene->IsEditorScene())
			{
				return false;
			}

			RayCastInfo rayCastInfo;
			rayCastInfo.Origin = inRaycastData->Origin;
			rayCastInfo.Direction = inRaycastData->Direction;
			rayCastInfo.MaxDistance = inRaycastData->MaxDistance;

			if (!inRaycastData->ExcludeEntities.IsEmpty())
			{
				size_t excludeEntitiesCount = inRaycastData->ExcludeEntities.Length();

				// NOTE(Peter): Same as calling the constructor with excludeEntitiesCount as the only argument
				rayCastInfo.ExcludedEntities.rehash(excludeEntitiesCount);
				
				for (size_t i = 0; i < excludeEntitiesCount; i++)
					rayCastInfo.ExcludedEntities.insert(inRaycastData->ExcludeEntities[i]);
			}

			SceneQueryHit tempHit;
			bool success = scene->GetPhysicsScene()->CastRay(&rayCastInfo, tempHit);

			if (success && !inRaycastData->RequiredComponentTypes.IsEmpty())
			{
				Entity entity = scene->GetEntityWithUUID(tempHit.HitEntity);
				size_t requiredComponentsCount = inRaycastData->RequiredComponentTypes.Length();

				bool foundRequiredComponent = false;
				for (size_t i = 0; i < requiredComponentsCount; i++)
				{
					Coral::Type& reflectionType = inRaycastData->RequiredComponentTypes[i];
					if (!reflectionType)
					{
						ErrorWithTrace("Physics.Raycast - Why did you feel the need to pass a \"null\" as a required component?");
						success = false;
						break;
					}

#ifdef HZ_DEBUG
					auto& baseType = reflectionType.GetBaseType();

					bool validComponentFilter = false;

					if (baseType)
					{
						Coral::ScopedString parentNameStr = baseType.GetFullName();
						std::string parentName = parentNameStr;
						validComponentFilter = parentName.find("Hazel.") != std::string::npos && parentName.find("Component") != std::string::npos;
					}

					if (!validComponentFilter)
					{
						Coral::ScopedString typeName = reflectionType.GetFullName();
						ErrorWithTrace("Physics.Raycast - {0} does not inherit from Hazel.Component!", std::string(typeName));
						success = false;
						break;
					}
#endif

					if (s_HasComponentFuncs[reflectionType.GetTypeId()](entity))
					{
						foundRequiredComponent = true;
						break;
					}
				}

				success = foundRequiredComponent;
			}

			if (success)
			{
				outHit->HitEntity = tempHit.HitEntity;
				outHit->Position = tempHit.Position;
				outHit->Normal = tempHit.Normal;
				outHit->Distance = tempHit.Distance;

				if (tempHit.HitCollider)
				{
					const auto& scriptEngine = ScriptEngine::GetInstance();

					Coral::ManagedObject shapeInstance;
					glm::vec3 offset(0.0f);

					auto createMeshShape = [&scriptEngine](std::string_view meshShapeClass, const MeshColliderComponent& colliderComp) -> Coral::ManagedObject
					{
						const auto colliderAsset = AssetManager::GetAsset<MeshColliderAsset>(colliderComp.ColliderAsset);
						const auto colliderMesh = AssetManager::GetAsset<Asset>(colliderAsset->ColliderMesh);

						if (colliderMesh->GetAssetType() == AssetType::StaticMesh)
						{
							const auto* staticMeshType = scriptEngine.GetTypeByName("Hazel.StaticMesh");
							auto staticMeshInstance = staticMeshType->CreateInstance(uint64_t(colliderComp.ColliderAsset));

							const auto* convexMeshShapeType = scriptEngine.GetTypeByName(meshShapeClass);
							return convexMeshShapeType->CreateInstance(staticMeshInstance);
						}
						else if (colliderMesh->GetAssetType() == AssetType::Mesh)
						{
							const auto* meshType = scriptEngine.GetTypeByName("Hazel.Mesh");
							auto meshInstance = meshType->CreateInstance(uint64_t(colliderComp.ColliderAsset));

							const auto* convexMeshShapeType = scriptEngine.GetTypeByName(meshShapeClass);
							return convexMeshShapeType->CreateInstance(meshInstance);
						}

						return {};
					};

					switch (tempHit.HitCollider->GetType())
					{
						case ShapeType::Box:
						{
							Entity hitEntity = GetEntity(tempHit.HitEntity);
							const auto& colliderComp = hitEntity.GetComponent<BoxColliderComponent>();
							offset = colliderComp.Offset;

							glm::vec3 halfSize = tempHit.HitCollider.As<BoxShape>()->GetHalfSize();

							const auto* boxShape = scriptEngine.GetTypeByName("Hazel.BoxShape");
							shapeInstance = boxShape->CreateInstance(halfSize);
							break;
						}
						case ShapeType::Sphere:
						{
							Entity hitEntity = GetEntity(tempHit.HitEntity);
							const auto& colliderComp = hitEntity.GetComponent<SphereColliderComponent>();
							offset = colliderComp.Offset;

							float radius = tempHit.HitCollider.As<SphereShape>()->GetRadius();
							const auto* sphereShape = scriptEngine.GetTypeByName("Hazel.SphereShape");
							shapeInstance = sphereShape->CreateInstance(radius);
							break;
						}
						case ShapeType::Capsule:
						{
							Entity hitEntity = GetEntity(tempHit.HitEntity);
							const auto& colliderComp = hitEntity.GetComponent<CapsuleColliderComponent>();
							offset = colliderComp.Offset;

							Ref<CapsuleShape> capsuleShape = tempHit.HitCollider.As<CapsuleShape>();
							float height = capsuleShape->GetHeight();
							float radius = capsuleShape->GetRadius();
							const auto* sphereShape = scriptEngine.GetTypeByName("Hazel.CapsuleShape");
							shapeInstance = sphereShape->CreateInstance(height, radius);
							break;
						}
						case ShapeType::ConvexMesh:
						{
							Entity hitEntity = GetEntity(tempHit.HitEntity);
							const auto& colliderComp = hitEntity.GetComponent<MeshColliderComponent>();
							shapeInstance = createMeshShape("Hazel.ConvexMeshShape", colliderComp);
							break;
						}
						case ShapeType::TriangleMesh:
						{
							Entity hitEntity = GetEntity(tempHit.HitEntity);
							const auto& colliderComp = hitEntity.GetComponent<MeshColliderComponent>();
							shapeInstance = createMeshShape("Hazel.TriangleMeshShape", colliderComp);
							break;
						}
					}

					if (shapeInstance.IsValid())
					{
						const auto* colliderType = scriptEngine.GetTypeByName("Hazel.Collider");
						outHit->HitCollider = colliderType->CreateInstance(tempHit.HitEntity, shapeInstance, offset);
					}
				}
			}
			else
			{
				*outHit = ScriptRaycastHit();
			}

			return success;
		}

		bool Physics_CastShape(ShapeQueryData* inShapeCastData, ScriptRaycastHit* outHit)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_ASSERT(scene, "No active scene!");

			if (scene->IsEditorScene())
			{
				return false;
			}

			const auto& shapeInstanceType = inShapeCastData->ShapeDataInstance.GetType();

			HZ_CORE_VERIFY(shapeInstanceType);

			const auto* shapeBaseType = ScriptEngine::GetInstance().GetTypeByName("Hazel.Shape");

			if (!shapeInstanceType.IsSubclassOf(*shapeBaseType))
			{
				HZ_CORE_VERIFY(false);
				return false;
			}

			ShapeCastInfo* shapeCastInfo = nullptr;
			ShapeType shapeType = inShapeCastData->ShapeDataInstance.GetPropertyValue<ShapeType>("ShapeType");

			switch (shapeType)
			{
				case ShapeType::Box:
				{
					BoxCastInfo* boxCastInfo = hnew BoxCastInfo();
					boxCastInfo->HalfExtent = inShapeCastData->ShapeDataInstance.GetPropertyValue<glm::vec3>("HalfExtent");
					shapeCastInfo = boxCastInfo;
					break;
				}
				case ShapeType::Sphere:
				{
					SphereCastInfo* sphereCastInfo = hnew SphereCastInfo();
					sphereCastInfo->Radius = inShapeCastData->ShapeDataInstance.GetPropertyValue<float>("Radius");
					shapeCastInfo = sphereCastInfo;
					break;
				}
				case ShapeType::Capsule:
				{
					CapsuleCastInfo* capsuleCastInfo = hnew CapsuleCastInfo();
					capsuleCastInfo->HalfHeight = inShapeCastData->ShapeDataInstance.GetPropertyValue<float>("HalfHeight");
					capsuleCastInfo->Radius = inShapeCastData->ShapeDataInstance.GetPropertyValue<float>("Radius");
					shapeCastInfo = capsuleCastInfo;
					break;
				}
				case ShapeType::ConvexMesh:
				case ShapeType::TriangleMesh:
				case ShapeType::CompoundShape:
				case ShapeType::MutableCompoundShape:
				{
					WarnWithTrace("Can't do a shape cast with Convex, Triangle or Compound shapes!");
					return false;
				}
			}

			if (shapeCastInfo == nullptr)
				return false;

			shapeCastInfo->Origin = inShapeCastData->Origin;
			shapeCastInfo->Direction = inShapeCastData->Direction;
			shapeCastInfo->MaxDistance = inShapeCastData->MaxDistance;

			if (!inShapeCastData->ExcludeEntities.IsEmpty())
			{
				// NOTE(Peter): Same as calling the constructor with excludeEntitiesCount as the only argument
				shapeCastInfo->ExcludedEntities.rehash(inShapeCastData->ExcludeEntities.Length());

				for (uint64_t entityID : inShapeCastData->ExcludeEntities)
					shapeCastInfo->ExcludedEntities.insert(entityID);
			}

			SceneQueryHit tempHit;
			bool success = scene->GetPhysicsScene()->CastShape(shapeCastInfo, tempHit);

			if (success && !inShapeCastData->RequiredComponentTypes.IsEmpty())
			{
				Entity entity = scene->GetEntityWithUUID(tempHit.HitEntity);

				for (auto reflectionType : inShapeCastData->RequiredComponentTypes)
				{
					Coral::Type& componentType = reflectionType;

					if (!componentType)
					{
						ErrorWithTrace("Why did you feel the need to pass a \"null\" as a required component?");
						success = false;
						break;
					}

#ifdef HZ_DEBUG
					auto& baseType = componentType.GetBaseType();

					bool validComponentFilter = false;

					if (baseType)
					{
						Coral::ScopedString parentNameStr = baseType.GetFullName();
						std::string parentName = parentNameStr;
						validComponentFilter = parentName.find("Hazel.") != std::string::npos && parentName.find("Component") != std::string::npos;
					}

					if (!validComponentFilter)
					{
						Coral::ScopedString typeName = componentType.GetFullName();
						ErrorWithTrace("Physics.CastShape - {0} does not inherit from Hazel.Component!", std::string(typeName));
						success = false;
						break;
					}
#endif

					if (!s_HasComponentFuncs[componentType](entity))
					{
						success = false;
						break;
					}
				}
			}

			if (success)
			{
				outHit->HitEntity = tempHit.HitEntity;
				outHit->Position = tempHit.Position;
				outHit->Normal = tempHit.Normal;
				outHit->Distance = tempHit.Distance;

				if (tempHit.HitCollider)
				{
					const auto& scriptEngine = ScriptEngine::GetInstance();

					Coral::ManagedObject shapeInstance;
					glm::vec3 offset(0.0f);

					auto createMeshShape = [&scriptEngine](std::string_view meshShapeClass, const MeshColliderComponent& colliderComp) -> Coral::ManagedObject
					{
						const auto colliderAsset = AssetManager::GetAsset<MeshColliderAsset>(colliderComp.ColliderAsset);
						const auto colliderMesh = AssetManager::GetAsset<Asset>(colliderAsset->ColliderMesh);

						if (colliderMesh->GetAssetType() == AssetType::StaticMesh)
						{
							const auto* staticMeshType = scriptEngine.GetTypeByName("Hazel.StaticMesh");
							auto staticMeshInstance = staticMeshType->CreateInstance(uint64_t(colliderComp.ColliderAsset));

							const auto* convexMeshShapeType = scriptEngine.GetTypeByName(meshShapeClass);
							return convexMeshShapeType->CreateInstance(staticMeshInstance);
						}
						else if (colliderMesh->GetAssetType() == AssetType::Mesh)
						{
							const auto* meshType = scriptEngine.GetTypeByName("Hazel.Mesh");
							auto meshInstance = meshType->CreateInstance(uint64_t(colliderComp.ColliderAsset));

							const auto* convexMeshShapeType = scriptEngine.GetTypeByName(meshShapeClass);
							return convexMeshShapeType->CreateInstance(meshInstance);
						}

						return {};
					};

					switch (tempHit.HitCollider->GetType())
					{
						case ShapeType::Box:
						{
							Entity hitEntity = GetEntity(tempHit.HitEntity);
							const auto& colliderComp = hitEntity.GetComponent<BoxColliderComponent>();
							offset = colliderComp.Offset;

							glm::vec3 halfSize = tempHit.HitCollider.As<BoxShape>()->GetHalfSize();

							const auto* boxShape = scriptEngine.GetTypeByName("Hazel.BoxShape");
							shapeInstance = boxShape->CreateInstance(halfSize);
							break;
						}
						case ShapeType::Sphere:
						{
							Entity hitEntity = GetEntity(tempHit.HitEntity);
							const auto& colliderComp = hitEntity.GetComponent<SphereColliderComponent>();
							offset = colliderComp.Offset;

							float radius = tempHit.HitCollider.As<SphereShape>()->GetRadius();
							const auto* sphereShape = scriptEngine.GetTypeByName("Hazel.SphereShape");
							shapeInstance = sphereShape->CreateInstance(radius);
							break;
						}
						case ShapeType::Capsule:
						{
							Entity hitEntity = GetEntity(tempHit.HitEntity);
							const auto& colliderComp = hitEntity.GetComponent<CapsuleColliderComponent>();
							offset = colliderComp.Offset;

							Ref<CapsuleShape> capsuleShape = tempHit.HitCollider.As<CapsuleShape>();
							float height = capsuleShape->GetHeight();
							float radius = capsuleShape->GetRadius();
							const auto* sphereShape = scriptEngine.GetTypeByName("Hazel.CapsuleShape");
							shapeInstance = sphereShape->CreateInstance(height, radius);
							break;
						}
						case ShapeType::ConvexMesh:
						{
							Entity hitEntity = GetEntity(tempHit.HitEntity);
							const auto& colliderComp = hitEntity.GetComponent<MeshColliderComponent>();
							shapeInstance = createMeshShape("Hazel.ConvexMeshShape", colliderComp);
							break;
						}
						case ShapeType::TriangleMesh:
						{
							Entity hitEntity = GetEntity(tempHit.HitEntity);
							const auto& colliderComp = hitEntity.GetComponent<MeshColliderComponent>();
							shapeInstance = createMeshShape("Hazel.TriangleMeshShape", colliderComp);
							break;
						}
					}

					if (shapeInstance.IsValid())
					{
						const auto* colliderType = scriptEngine.GetTypeByName("Hazel.Collider");
						outHit->HitCollider = colliderType->CreateInstance(tempHit.HitEntity, shapeInstance, offset);
					}
				}
			}
			else
			{
				*outHit = ScriptRaycastHit();
			}

			hdelete shapeCastInfo;
			return success;
		}

		Coral::Array<ScriptRaycastHit2D> Physics_Raycast2D(RaycastData2D* inRaycastData)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_ASSERT(scene, "No active scene!");

			if (scene->IsEditorScene())
			{
				return {};
			}

			std::vector<Raycast2DResult> raycastResults = Physics2D::Raycast(scene, inRaycastData->Origin, inRaycastData->Origin + inRaycastData->Direction * inRaycastData->MaxDistance);

			auto result = Coral::Array<ScriptRaycastHit2D>::New(raycastResults.size());

			for (size_t i = 0; i < raycastResults.size(); i++)
			{
				result[i] = {
					.EntityID = raycastResults[i].HitEntity.GetUUID(),
					.Position = raycastResults[i].Point,
					.Normal = raycastResults[i].Normal,
					.Distance = raycastResults[i].Distance,
				};
			}

			return result;
		}

		int32_t Physics_OverlapShape(ShapeQueryData* inOverlapData, Coral::Array<ScriptRaycastHit>* outHits)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_ASSERT(scene, "No active scene!");

			const auto& shapeInstanceType = inOverlapData->ShapeDataInstance.GetType();

			HZ_CORE_VERIFY(shapeInstanceType);

			const auto* shapeBaseType = ScriptEngine::GetInstance().GetTypeByName("Hazel.Shape");

			if (!shapeInstanceType.IsSubclassOf(*shapeBaseType))
			{
				HZ_CORE_VERIFY(false);
				return false;
			}

			ShapeOverlapInfo* shapeOverlapInfo = nullptr;

			ShapeType shapeType = inOverlapData->ShapeDataInstance.GetPropertyValue<ShapeType>("ShapeType");

			switch (shapeType)
			{
				case ShapeType::Box:
				{
					BoxOverlapInfo* boxOverlapInfo = hnew BoxOverlapInfo();
					boxOverlapInfo->HalfExtent = inOverlapData->ShapeDataInstance.GetPropertyValue<glm::vec3>("HalfExtent");
					shapeOverlapInfo = boxOverlapInfo;
					break;
				}
				case ShapeType::Sphere:
				{
					SphereOverlapInfo* sphereOverlapInfo = hnew SphereOverlapInfo();
					sphereOverlapInfo->Radius = inOverlapData->ShapeDataInstance.GetPropertyValue<float>("Radius");
					shapeOverlapInfo = sphereOverlapInfo;
					break;
				}
				case ShapeType::Capsule:
				{
					CapsuleOverlapInfo* capsuleOverlapInfo = hnew CapsuleOverlapInfo();
					capsuleOverlapInfo->HalfHeight = inOverlapData->ShapeDataInstance.GetPropertyValue<float>("HalfHeight");
					capsuleOverlapInfo->Radius = inOverlapData->ShapeDataInstance.GetPropertyValue<float>("Radius");
					shapeOverlapInfo = capsuleOverlapInfo;
					break;
				}
				case ShapeType::ConvexMesh:
				case ShapeType::TriangleMesh:
				case ShapeType::CompoundShape:
				case ShapeType::MutableCompoundShape:
				{
					WarnWithTrace("Can't do a shape overlap with Convex, Triangle or Compound shapes!");
					return false;
				}
			}

			if (shapeOverlapInfo == nullptr)
				return false;

			shapeOverlapInfo->Origin = inOverlapData->Origin;

			if (!inOverlapData->ExcludeEntities.IsEmpty())
			{
				// NOTE(Peter): Same as calling the constructor with excludeEntitiesCount as the only argument
				shapeOverlapInfo->ExcludedEntities.rehash(inOverlapData->ExcludeEntities.Length());

				for (uint64_t entityID : inOverlapData->ExcludeEntities)
					shapeOverlapInfo->ExcludedEntities.insert(entityID);
			}

			SceneQueryHit* hitArray;
			int32_t overlapCount = GetPhysicsScene()->OverlapShape(shapeOverlapInfo, &hitArray);

			if (overlapCount == 0)
				return 0;

			if (overlapCount > 0 && !inOverlapData->RequiredComponentTypes.IsEmpty())
			{
				for (size_t i = 0; i < size_t(overlapCount); i++)
				{
					Entity entity = scene->GetEntityWithUUID(hitArray[i].HitEntity);

					for (auto reflectionType : inOverlapData->RequiredComponentTypes)
					{
						Coral::Type& componentType = reflectionType;

						if (!componentType)
						{
							ErrorWithTrace("Why did you feel the need to pass a \"null\" as a required component?");
							overlapCount = 0;
							break;
						}

#ifdef HZ_DEBUG
						auto& baseType = componentType.GetBaseType();

						bool validComponentFilter = false;

						if (baseType)
						{
							Coral::ScopedString parentNameStr = baseType.GetFullName();
							std::string parentName = parentNameStr;
							validComponentFilter = parentName.find("Hazel.") != std::string::npos && parentName.find("Component") != std::string::npos;
						}

						if (!validComponentFilter)
						{
							Coral::ScopedString typeName = componentType.GetFullName();
							ErrorWithTrace("Physics.OverlapShape - {0} does not inherit from Hazel.Component!", std::string(typeName));
							overlapCount = 0;
							break;
						}
#endif

						if (!s_HasComponentFuncs[componentType](entity))
						{
							overlapCount = 0;
							break;
						}
					}
				}
			}

			if (overlapCount != 0 && (outHits->IsEmpty() || outHits->Length() < overlapCount))
				*outHits = Coral::Array<ScriptRaycastHit>::New(overlapCount);

			for (size_t i = 0; i < overlapCount; i++)
			{
				ScriptRaycastHit hitData;
				hitData.HitEntity = hitArray[i].HitEntity;
				hitData.Position = hitArray[i].Position;
				hitData.Normal = hitArray[i].Normal;
				hitData.Distance = hitArray[i].Distance;

				const auto& scriptEngine = ScriptEngine::GetInstance();

				auto createMeshShape = [&scriptEngine](std::string_view meshShapeClass, const MeshColliderComponent& colliderComp) -> Coral::ManagedObject
				{
					const auto colliderAsset = AssetManager::GetAsset<MeshColliderAsset>(colliderComp.ColliderAsset);
					const auto colliderMesh = AssetManager::GetAsset<Asset>(colliderAsset->ColliderMesh);

					if (colliderMesh->GetAssetType() == AssetType::StaticMesh)
					{
						const auto* staticMeshType = scriptEngine.GetTypeByName("Hazel.StaticMesh");
						auto staticMeshInstance = staticMeshType->CreateInstance(uint64_t(colliderComp.ColliderAsset));

						const auto* convexMeshShapeType = scriptEngine.GetTypeByName(meshShapeClass);
						return convexMeshShapeType->CreateInstance(staticMeshInstance);
					}
					else if (colliderMesh->GetAssetType() == AssetType::Mesh)
					{
						const auto* meshType = scriptEngine.GetTypeByName("Hazel.Mesh");
						auto meshInstance = meshType->CreateInstance(uint64_t(colliderComp.ColliderAsset));

						const auto* convexMeshShapeType = scriptEngine.GetTypeByName(meshShapeClass);
						return convexMeshShapeType->CreateInstance(meshInstance);
					}

					return {};
				};
				
				if (hitArray[i].HitCollider)
				{
					Coral::ManagedObject shapeInstance;
					glm::vec3 offset(0.0f);

					switch (hitArray[i].HitCollider->GetType())
					{
						case ShapeType::Box:
						{
							Entity hitEntity = GetEntity(hitArray[i].HitEntity);
							const auto& colliderComp = hitEntity.GetComponent<BoxColliderComponent>();
							offset = colliderComp.Offset;

							glm::vec3 halfSize = hitArray[i].HitCollider.As<BoxShape>()->GetHalfSize();
							const auto* boxShape = scriptEngine.GetTypeByName("Hazel.BoxShape");
							shapeInstance = boxShape->CreateInstance(halfSize);
							break;
						}
						case ShapeType::Sphere:
						{
							Entity hitEntity = GetEntity(hitArray[i].HitEntity);
							const auto& colliderComp = hitEntity.GetComponent<SphereColliderComponent>();
							offset = colliderComp.Offset;

							float radius = hitArray[i].HitCollider.As<SphereShape>()->GetRadius();
							const auto* sphereShape = scriptEngine.GetTypeByName("Hazel.SphereShape");
							shapeInstance = sphereShape->CreateInstance(radius);
							break;
						}
						case ShapeType::Capsule:
						{
							Entity hitEntity = GetEntity(hitArray[i].HitEntity);
							const auto& colliderComp = hitEntity.GetComponent<CapsuleColliderComponent>();
							offset = colliderComp.Offset;

							Ref<CapsuleShape> capsuleShape = hitArray[i].HitCollider.As<CapsuleShape>();
							float height = capsuleShape->GetHeight();
							float radius = capsuleShape->GetRadius();
							const auto* sphereShape = scriptEngine.GetTypeByName("Hazel.CapsuleShape");
							shapeInstance = sphereShape->CreateInstance(height, radius);
							break;
						}
						case ShapeType::ConvexMesh:
						{
							Entity hitEntity = GetEntity(hitArray[i].HitEntity);
							const auto& colliderComp = hitEntity.GetComponent<MeshColliderComponent>();
							shapeInstance = createMeshShape("Hazel.ConvexMeshShape", colliderComp);
							break;
						}
						case ShapeType::TriangleMesh:
						{
							Entity hitEntity = GetEntity(hitArray[i].HitEntity);
							const auto& colliderComp = hitEntity.GetComponent<MeshColliderComponent>();
							shapeInstance = createMeshShape("Hazel.TriangleMeshShape", colliderComp);
							break;
						}
					}

					if (shapeInstance.IsValid())
					{
						const auto* colliderType = scriptEngine.GetTypeByName("Hazel.Collider");
						hitData.HitCollider = colliderType->CreateInstance(hitArray[i].HitEntity, shapeInstance, offset);
					}

					(*outHits)[i] = hitData;
				}
			}

			hdelete shapeOverlapInfo;
			return overlapCount;
		}

		void Physics_GetGravity(glm::vec3* outGravity)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_ASSERT(scene, "No active scene!");
			*outGravity = scene->GetPhysicsScene()->GetGravity();
		}

		void Physics_SetGravity(glm::vec3* inGravity)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_ASSERT(scene, "No active scene!");
			scene->GetPhysicsScene()->SetGravity(*inGravity);
		}

		void Physics_AddRadialImpulse(glm::vec3* inOrigin, float radius, float strength, EFalloffMode falloff, bool velocityChange)
		{
			Ref<Scene> scene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_ASSERT(scene, "No active scene!");
			scene->GetPhysicsScene()->AddRadialImpulse(*inOrigin, radius, strength, falloff, velocityChange);
		}

#pragma endregion

#pragma region Matrix4
		void Matrix4_LookAt(glm::vec3* eye, glm::vec3* center, glm::vec3* up, glm::mat4* outMatrix)
		{
			*outMatrix = glm::lookAt(*eye, *center, *up);
		}

#pragma endregion

#pragma region Noise
		
		Noise* Noise_Constructor(int seed)
		{
			return hnew Noise(seed);
		}

		void Noise_Destructor(Noise* _this)
		{
			hdelete _this;
		}

		float Noise_GetFrequency(Noise* _this) { return _this->GetFrequency(); }
		void Noise_SetFrequency(Noise* _this, float frequency) { _this->SetFrequency(frequency); }

		int Noise_GetFractalOctaves(Noise* _this) { return _this->GetFractalOctaves(); }
		void Noise_SetFractalOctaves(Noise* _this, int octaves) { _this->SetFractalOctaves(octaves); }

		float Noise_GetFractalLacunarity(Noise* _this) { return _this->GetFractalLacunarity(); }
		void Noise_SetFractalLacunarity(Noise* _this, float lacunarity) { _this->SetFractalLacunarity(lacunarity); }

		float Noise_GetFractalGain(Noise* _this) { return _this->GetFractalGain(); }
		void Noise_SetFractalGain(Noise* _this, float gain) { _this->SetFractalGain(gain); }

		float Noise_Get(Noise* _this, float x, float y) { return _this->Get(x, y); }

		void Noise_SetSeed(int seed) { Noise::SetSeed(seed); }
		float Noise_Perlin(float x, float y) { return Noise::PerlinNoise(x, y); }

#pragma endregion

#pragma region Log

		void Log_LogMessage(LogLevel level, Coral::String inFormattedMessage)
		{
			std::string message = inFormattedMessage;
			switch (level)
			{
				case LogLevel::Trace:
					HZ_CONSOLE_LOG_TRACE(message);
					break;
				case LogLevel::Debug:
					HZ_CONSOLE_LOG_INFO(message);
					break;
				case LogLevel::Info:
					HZ_CONSOLE_LOG_INFO(message);
					break;
				case LogLevel::Warn:
					HZ_CONSOLE_LOG_WARN(message);
					break;
				case LogLevel::Error:
					HZ_CONSOLE_LOG_ERROR(message);
					break;
				case LogLevel::Critical:
					HZ_CONSOLE_LOG_FATAL(message);
					break;
			}
			Coral::String::Free(inFormattedMessage);
		}

#pragma endregion

#pragma region Input

		Coral::Bool32 Input_IsKeyPressed(KeyCode keycode) { return Input::IsKeyPressed(keycode); }
		Coral::Bool32 Input_IsKeyHeld(KeyCode keycode) { return Input::IsKeyHeld(keycode); }
		Coral::Bool32 Input_IsKeyDown(KeyCode keycode) { return Input::IsKeyDown(keycode); }
		Coral::Bool32 Input_IsKeyReleased(KeyCode keycode) { return Input::IsKeyReleased(keycode); }

		Coral::Bool32 Input_IsMouseButtonPressed(MouseButton button)
		{
			bool isPressed = Input::IsMouseButtonPressed(button);

			bool enableImGui = Application::Get().GetSpecification().EnableImGui;
			if (isPressed && enableImGui && GImGui->HoveredWindow != nullptr)
			{
				// Make sure we're in the viewport panel
				ImGuiWindow* viewportWindow = ImGui::FindWindowByName("Viewport");
				if (viewportWindow != nullptr)
					isPressed = GImGui->HoveredWindow->ID == viewportWindow->ID;
			}

			return isPressed;
		}
		Coral::Bool32 Input_IsMouseButtonHeld(MouseButton button)
		{
			bool isHeld = Input::IsMouseButtonHeld(button);

			bool enableImGui = Application::Get().GetSpecification().EnableImGui;
			if (isHeld && enableImGui && GImGui->HoveredWindow != nullptr)
			{
				// Make sure we're in the viewport panel
				ImGuiWindow* viewportWindow = ImGui::FindWindowByName("Viewport");
				if (viewportWindow != nullptr)
					isHeld = GImGui->HoveredWindow->ID == viewportWindow->ID;
			}

			return isHeld;
		}
		Coral::Bool32 Input_IsMouseButtonDown(MouseButton button)
		{
			bool isDown = Input::IsMouseButtonDown(button);

			bool enableImGui = Application::Get().GetSpecification().EnableImGui;
			if (isDown && enableImGui && GImGui->HoveredWindow != nullptr)
			{
				// Make sure we're in the viewport panel
				ImGuiWindow* viewportWindow = ImGui::FindWindowByName("Viewport");
				if (viewportWindow != nullptr)
					isDown = GImGui->HoveredWindow->ID == viewportWindow->ID;
			}

			return isDown;
		}
		Coral::Bool32 Input_IsMouseButtonReleased(MouseButton button)
		{
			bool released = Input::IsMouseButtonReleased(button);

			bool enableImGui = Application::Get().GetSpecification().EnableImGui;
			if (released && enableImGui && GImGui->HoveredWindow != nullptr)
			{
				// Make sure we're in the viewport panel
				ImGuiWindow* viewportWindow = ImGui::FindWindowByName("Viewport");
				if (viewportWindow != nullptr)
					released = GImGui->HoveredWindow->ID == viewportWindow->ID;
			}

			return released;
		}

		void Input_GetMousePosition(glm::vec2* outPosition)
		{
			auto [x, y] = Input::GetMousePosition();
			*outPosition = { x, y };
		}

		void Input_SetCursorMode(CursorMode mode) { Input::SetCursorMode(mode); }
		CursorMode Input_GetCursorMode() { return Input::GetCursorMode(); }
		Coral::Bool32 Input_IsControllerPresent(int id) { return Input::IsControllerPresent(id); }

		Coral::Array<int32_t> Input_GetConnectedControllerIDs()
		{
			return Coral::Array<int32_t>::New(Input::GetConnectedControllerIDs());
		}

		Coral::String Input_GetControllerName(int id)
		{
			auto name = Input::GetControllerName(id);
			if (name.empty())
				return {};
			return Coral::String::New(name);
		}

		Coral::Bool32 Input_IsControllerButtonPressed(int id, int button) { return Input::IsControllerButtonPressed(id, button); }
		Coral::Bool32 Input_IsControllerButtonHeld(int id, int button) { return Input::IsControllerButtonHeld(id, button); }
		Coral::Bool32 Input_IsControllerButtonDown(int id, int button) { return Input::IsControllerButtonDown(id, button); }
		Coral::Bool32 Input_IsControllerButtonReleased(int id, int button) { return Input::IsControllerButtonReleased(id, button); }


		float Input_GetControllerAxis(int id, int axis) { return Input::GetControllerAxis(id, axis); }
		uint8_t Input_GetControllerHat(int id, int hat) { return Input::GetControllerHat(id, hat); }

		float Input_GetControllerDeadzone(int id, int axis) { return Input::GetControllerDeadzone(id, axis); }
		void Input_SetControllerDeadzone(int id, int axis, float deadzone) { return Input::SetControllerDeadzone(id, axis, deadzone); }

#pragma endregion

#pragma region SceneRenderer

		float SceneRenderer_GetOpacity()
		{
			Ref<SceneRenderer> sceneRenderer = ScriptEngine::GetInstance().GetSceneRenderer();
			HZ_CORE_VERIFY(sceneRenderer);
			return sceneRenderer->GetOpacity();
		}

		void SceneRenderer_SetOpacity(float opacity)
		{
			Ref<SceneRenderer> sceneRenderer = ScriptEngine::GetInstance().GetSceneRenderer();
			HZ_CORE_VERIFY(sceneRenderer);
			sceneRenderer->SetOpacity(opacity);
		}

		bool SceneRenderer_DepthOfField_IsEnabled()
		{
			Ref<SceneRenderer> sceneRenderer = ScriptEngine::GetInstance().GetSceneRenderer();
			HZ_CORE_VERIFY(sceneRenderer);
			return sceneRenderer->GetDOFSettings().Enabled;
		}

		void SceneRenderer_DepthOfField_SetEnabled(bool enabled)
		{
			Ref<SceneRenderer> sceneRenderer = ScriptEngine::GetInstance().GetSceneRenderer();
			HZ_CORE_VERIFY(sceneRenderer);
			sceneRenderer->GetDOFSettings().Enabled = enabled;
		}

		float SceneRenderer_DepthOfField_GetFocusDistance()
		{
			Ref<SceneRenderer> sceneRenderer = ScriptEngine::GetInstance().GetSceneRenderer();
			HZ_CORE_VERIFY(sceneRenderer);
			return sceneRenderer->GetDOFSettings().FocusDistance;
		}

		void SceneRenderer_DepthOfField_SetFocusDistance(float focusDistance)
		{
			Ref<SceneRenderer> sceneRenderer = ScriptEngine::GetInstance().GetSceneRenderer();
			HZ_CORE_VERIFY(sceneRenderer);
			sceneRenderer->GetDOFSettings().FocusDistance = focusDistance;
		}

		float SceneRenderer_DepthOfField_GetBlurSize()
		{
			Ref<SceneRenderer> sceneRenderer = ScriptEngine::GetInstance().GetSceneRenderer();
			HZ_CORE_VERIFY(sceneRenderer);
			return sceneRenderer->GetDOFSettings().BlurSize;
		}

		void SceneRenderer_DepthOfField_SetBlurSize(float blurSize)
		{
			Ref<SceneRenderer> sceneRenderer = ScriptEngine::GetInstance().GetSceneRenderer();
			HZ_CORE_VERIFY(sceneRenderer);
			sceneRenderer->GetDOFSettings().BlurSize = blurSize;
		}

#pragma endregion

#pragma region DebugRenderer

		void DebugRenderer_DrawLine(glm::vec3* p0, glm::vec3* p1, glm::vec4* color)
		{
			Ref<SceneRenderer> sceneRenderer = ScriptEngine::GetInstance().GetSceneRenderer();
			Ref<DebugRenderer> debugRenderer = sceneRenderer->GetDebugRenderer();
			
			debugRenderer->DrawLine(*p0, *p1, *color);
		}

		void DebugRenderer_DrawQuadBillboard(glm::vec3* translation, glm::vec2* size, glm::vec4* color)
		{
			Ref<SceneRenderer> sceneRenderer = ScriptEngine::GetInstance().GetSceneRenderer();
			Ref<DebugRenderer> debugRenderer = sceneRenderer->GetDebugRenderer();

			debugRenderer->DrawQuadBillboard(*translation, *size, *color);
		}

		void DebugRenderer_SetLineWidth(float width)
		{
			Ref<SceneRenderer> sceneRenderer = ScriptEngine::GetInstance().GetSceneRenderer();
			Ref<DebugRenderer> debugRenderer = sceneRenderer->GetDebugRenderer();

			debugRenderer->SetLineWidth(width);
		}

#pragma endregion


#pragma region PerformanceTimers

		float PerformanceTimers_GetFrameTime()
		{
			return (float)Application::Get().GetFrametime().GetMilliseconds();
		}

		float PerformanceTimers_GetGPUTime()
		{
			Ref<SceneRenderer> sceneRenderer = ScriptEngine::GetInstance().GetSceneRenderer();
			return (float)sceneRenderer->GetStatistics().TotalGPUTime;
		}

		float PerformanceTimers_GetMainThreadWorkTime()
		{
			return (float)Application::Get().GetPerformanceTimers().MainThreadWorkTime;
		}

		float PerformanceTimers_GetMainThreadWaitTime()
		{
			return (float)Application::Get().GetPerformanceTimers().MainThreadWaitTime;
		}

		float PerformanceTimers_GetRenderThreadWorkTime()
		{
			return (float)Application::Get().GetPerformanceTimers().RenderThreadWorkTime;
		}

		float PerformanceTimers_GetRenderThreadWaitTime()
		{
			return (float)Application::Get().GetPerformanceTimers().RenderThreadWaitTime;
		}

		uint32_t PerformanceTimers_GetFramesPerSecond()
		{
			return (uint32_t)(1.0f / (float)Application::Get().GetFrametime());
		}

		uint32_t PerformanceTimers_GetEntityCount()
		{
			Ref<Scene> activeScene = ScriptEngine::GetInstance().GetCurrentScene();
			HZ_CORE_ASSERT(activeScene, "No active scene!");

			return (uint32_t) activeScene->GetEntityMap().size();
		}

		uint32_t PerformanceTimers_GetScriptEntityCount()
		{
			//return (uint32_t)ScriptEngine::GetEntityInstances().size();
			return 0;
		}

#pragma endregion

	}
}

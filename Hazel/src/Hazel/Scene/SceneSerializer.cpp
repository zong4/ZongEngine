#include "hzpch.h"
#include "SceneSerializer.h"

#include "Hazel/Scene/Scene.h"
#include "Components.h"

#include "Hazel/Animation/AnimationGraph.h"
#include "Hazel/Animation/Skeleton.h"
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Audio/AudioComponent.h"
#include "Hazel/Audio/AudioEngine.h"
#include "Hazel/Editor/NodeGraphEditor/AnimationGraph/AnimationGraphAsset.h" // TODO (0x): separate editor from runtime
#include "Hazel/Physics/PhysicsSystem.h"
#include "Hazel/Script/ScriptEngine.h"

#include "Hazel/Renderer/MeshFactory.h"
#include "Hazel/Renderer/UI/Font.h"
#include "Hazel/Utilities/SerializationMacros.h"
#include "Hazel/Utilities/YAMLSerializationHelpers.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <magic_enum.hpp>

#include "yaml-cpp/yaml.h"

#include <filesystem>
#include <fstream>

namespace Hazel {

	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: m_Scene(scene)
	{
	}

	void SceneSerializer::SerializeEntity(YAML::Emitter& out, Entity entity, Ref<Scene> scene)
	{
		UUID uuid = entity.GetComponent<IDComponent>().ID;
		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity";
		out << YAML::Value << uuid;

		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap; // TagComponent

			auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // TagComponent
		}

		if (entity.HasComponent<RelationshipComponent>())
		{
			auto& relationshipComponent = entity.GetComponent<RelationshipComponent>();
			out << YAML::Key << "Parent" << YAML::Value << relationshipComponent.ParentHandle;

			out << YAML::Key << "Children";
			out << YAML::Value << YAML::BeginSeq;

			for (auto child : relationshipComponent.Children)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Handle" << YAML::Value << child;
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;
		}

		if (entity.HasComponent<PrefabComponent>())
		{
			out << YAML::Key << "PrefabComponent";
			out << YAML::BeginMap; // PrefabComponent

			auto& prefabComponent = entity.GetComponent<PrefabComponent>();
			out << YAML::Key << "Prefab" << YAML::Value << prefabComponent.PrefabID;
			out << YAML::Key << "Entity" << YAML::Value << prefabComponent.EntityID;

			out << YAML::EndMap; // PrefabComponent
		}

		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent

			auto& transform = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Position" << YAML::Value << transform.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << transform.RotationEuler;
			out << YAML::Key << "Scale" << YAML::Value << transform.Scale;

			out << YAML::EndMap; // TransformComponent
		}

		if (entity.HasComponent<ScriptComponent>())
		{
			out << YAML::Key << "ScriptComponent";
			out << YAML::BeginMap; // ScriptComponent

			const auto& scriptEngine = ScriptEngine::GetInstance();
			const auto& sc = entity.GetComponent<ScriptComponent>();

			if (scriptEngine.IsValidScript(sc.ScriptID))
			{
				const auto& scriptMetadata = scriptEngine.GetScriptMetadata(sc.ScriptID);
				const auto& entityStorage = scene->m_ScriptStorage.EntityStorage.at(entity.GetUUID());

				out << YAML::Key << "ScriptID" << YAML::Value << sc.ScriptID;
				out << YAML::Key << "ScriptName" << YAML::Value << scriptMetadata.FullName;

				out << YAML::Key << "Fields" << YAML::Value << YAML::BeginSeq;
				for (const auto&[fieldID, fieldStorage] : entityStorage.Fields)
				{
					const auto& fieldMetadata = scriptMetadata.Fields.at(fieldID);

					out << YAML::BeginMap;
					out << YAML::Key << "ID" << YAML::Value << fieldID;
					out << YAML::Key << "Name" << YAML::Value << fieldMetadata.Name;
					out << YAML::Key << "Type" << YAML::Value << std::string(magic_enum::enum_name(fieldMetadata.Type));
					out << YAML::Key << "Value" << YAML::Value;

					if (fieldStorage.IsArray())
					{
						out << YAML::BeginSeq;

						for (int32_t i = 0; i < fieldStorage.GetLength(); i++)
						{
							switch (fieldMetadata.Type)
							{
								case DataType::SByte:
									out << fieldStorage.GetValue<int8_t>(i);
									break;
								case DataType::Byte:
									out << fieldStorage.GetValue<uint8_t>(i);
									break;
								case DataType::Short:
									out << fieldStorage.GetValue<int16_t>(i);
									break;
								case DataType::UShort:
									out << fieldStorage.GetValue<uint16_t>(i);
									break;
								case DataType::Int:
									out << fieldStorage.GetValue<int32_t>(i);
									break;
								case DataType::UInt:
									out << fieldStorage.GetValue<uint32_t>(i);
									break;
								case DataType::Long:
									out << fieldStorage.GetValue<int64_t>(i);
									break;
								case DataType::ULong:
									out << fieldStorage.GetValue<uint64_t>(i);
									break;
								case DataType::Float:
									out << fieldStorage.GetValue<float>(i);
									break;
								case DataType::Double:
									out << fieldStorage.GetValue<double>(i);
									break;
								case DataType::Vector2:
									out << fieldStorage.GetValue<glm::vec2>(i);
									break;
								case DataType::Vector3:
									out << fieldStorage.GetValue<glm::vec3>(i);
									break;
								case DataType::Vector4:
									out << fieldStorage.GetValue<glm::vec4>(i);
									break;
								case DataType::Entity:
									out << fieldStorage.GetValue<uint64_t>(i);
									break;
								case DataType::Prefab:
									out << fieldStorage.GetValue<uint64_t>(i);
									break;
								case DataType::Mesh:
									out << fieldStorage.GetValue<uint64_t>(i);
									break;
								case DataType::StaticMesh:
									out << fieldStorage.GetValue<uint64_t>(i);
									break;
								case DataType::Material:
									out << fieldStorage.GetValue<uint64_t>(i);
									break;
								case DataType::Texture2D:
									out << fieldStorage.GetValue<uint64_t>(i);
									break;
								case DataType::Scene:
									out << fieldStorage.GetValue<uint64_t>(i);
									break;
								default:
									break;
							}
						}

						out << YAML::EndSeq;
					}
					else
					{
						switch (fieldMetadata.Type)
						{
							case DataType::SByte:
								out << fieldStorage.GetValue<int8_t>();
								break;
							case DataType::Byte:
								out << fieldStorage.GetValue<uint8_t>();
								break;
							case DataType::Short:
								out << fieldStorage.GetValue<int16_t>();
								break;
							case DataType::UShort:
								out << fieldStorage.GetValue<uint16_t>();
								break;
							case DataType::Int:
								out << fieldStorage.GetValue<int32_t>();
								break;
							case DataType::UInt:
								out << fieldStorage.GetValue<uint32_t>();
								break;
							case DataType::Long:
								out << fieldStorage.GetValue<int64_t>();
								break;
							case DataType::ULong:
								out << fieldStorage.GetValue<uint64_t>();
								break;
							case DataType::Float:
								out << fieldStorage.GetValue<float>();
								break;
							case DataType::Double:
								out << fieldStorage.GetValue<double>();
								break;
							case DataType::Vector2:
								out << fieldStorage.GetValue<glm::vec2>();
								break;
							case DataType::Vector3:
								out << fieldStorage.GetValue<glm::vec2>();
								break;
							case DataType::Vector4:
								out << fieldStorage.GetValue<glm::vec2>();
								break;
							case DataType::Entity:
								out << fieldStorage.GetValue<uint64_t>();
								break;
							case DataType::Prefab:
								out << fieldStorage.GetValue<uint64_t>();
								break;
							case DataType::Mesh:
								out << fieldStorage.GetValue<uint64_t>();
								break;
							case DataType::StaticMesh:
								out << fieldStorage.GetValue<uint64_t>();
								break;
							case DataType::Material:
								out << fieldStorage.GetValue<uint64_t>();
								break;
							case DataType::Texture2D:
								out << fieldStorage.GetValue<uint64_t>();
								break;
							case DataType::Scene:
								out << fieldStorage.GetValue<uint64_t>();
								break;
							default:
								break;
						}
					}

					out << YAML::EndMap;
				}
				out << YAML::EndSeq;
			}

			out << YAML::EndMap; // ScriptComponent
		}

		if (entity.HasComponent<MeshComponent>())
		{
			out << YAML::Key << "MeshComponent";
			out << YAML::BeginMap; // MeshComponent

			auto& mc = entity.GetComponent<MeshComponent>();
			out << YAML::Key << "AssetID" << YAML::Value << mc.Mesh;

			out << YAML::EndMap; // MeshComponent
		}

		if (entity.HasComponent<MeshTagComponent>())
		{
			out << YAML::Key << "MeshTagComponent";
			out << YAML::BeginMap; // MeshTagComponent

			auto& mtc = entity.GetComponent<MeshTagComponent>();
			out << YAML::Key << "EntityID" << YAML::Value << mtc.MeshEntity;

			out << YAML::EndMap; // MeshTagComponent
		}

		if (entity.HasComponent<SubmeshComponent>())
		{
			out << YAML::Key << "SubmeshComponent";
			out << YAML::BeginMap; // SubmeshComponent

			auto& mc = entity.GetComponent<SubmeshComponent>();
			out << YAML::Key << "AssetID" << YAML::Value << mc.Mesh;
			out << YAML::Key << "SubmeshIndex" << YAML::Value << mc.SubmeshIndex;

			auto materialTable = mc.MaterialTable;
			if (materialTable->GetMaterialCount() > 0)
			{
				out << YAML::Key << "MaterialTable" << YAML::Value << YAML::BeginMap; // MaterialTable

				for (uint32_t i = 0; i < materialTable->GetMaterialCount(); i++)
				{
					AssetHandle handle = (materialTable->HasMaterial(i) ? materialTable->GetMaterial(i) : (AssetHandle)0);
					out << YAML::Key << i << YAML::Value << handle;

				}

				out << YAML::EndMap; // MaterialTable
			}

			out << YAML::Key << "Visible" << YAML::Value << mc.Visible;
			out << YAML::EndMap; // MeshComponent
		}

		if (entity.HasComponent<StaticMeshComponent>())
		{
			out << YAML::Key << "StaticMeshComponent";
			out << YAML::BeginMap; // StaticMeshComponent

			auto& smc = entity.GetComponent<StaticMeshComponent>();
			out << YAML::Key << "AssetID" << YAML::Value << smc.StaticMesh;

			auto materialTable = smc.MaterialTable;
			if (materialTable->GetMaterialCount() > 0)
			{
				out << YAML::Key << "MaterialTable" << YAML::Value << YAML::BeginMap; // MaterialTable

				for (uint32_t i = 0; i < materialTable->GetMaterialCount(); i++)
				{
					AssetHandle handle = (materialTable->HasMaterial(i) ? materialTable->GetMaterial(i) : (AssetHandle)0);
					out << YAML::Key << i << YAML::Value << handle;

				}

				out << YAML::EndMap; // MaterialTable
			}

			out << YAML::Key << "Visible" << YAML::Value << smc.Visible;
			out << YAML::EndMap; // StaticMeshComponent
		}

		if (entity.HasComponent<AnimationComponent>())
		{
			out << YAML::Key << "AnimationComponent";
			out << YAML::BeginMap; // AnimationComponent

			auto& anim = entity.GetComponent<AnimationComponent>();
			if (anim.AnimationGraph)
			{
				out << YAML::Key << "AnimationGraph" << YAML::Value << anim.AnimationGraphHandle;
				{
					out << YAML::Key << "GraphInputs";
					out << YAML::BeginMap; // GraphInputs
					if (anim.AnimationGraph)
					{
						for (auto [id, value] : anim.AnimationGraph->Ins)
						{

							// TODO: array inputs!

							if (value.isBool())         out << YAML::Key << id << YAML::Value << value.getBool();
							else if (value.isInt32())   out << YAML::Key << id << YAML::Value << value.getInt32();
							else if (value.isInt64())   out << YAML::Key << id << YAML::Value << value.getInt64();
							else if (value.isFloat32()) out << YAML::Key << id << YAML::Value << value.getFloat32();
							else if (value.isFloat64()) out << YAML::Key << id << YAML::Value << value.getFloat64();
							else if (value.isVoid());   // void value is for triggers.  No need to output anything;
							else if (value.isObjectWithClassName(type::type_name<glm::vec3>())) out << YAML::Key << id << YAML::Value << *static_cast<glm::vec3*>(value.getRawData());
							else
							{
								HZ_CORE_ASSERT(false, "Unknown type");
								out << YAML::Value << 0;
							}
						}
					}
				}
				out << YAML::EndMap; // GraphInputs
			}
			out << YAML::EndMap; // AnimationComponent
		}

		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap; // CameraComponent

			auto& cameraComponent = entity.GetComponent<CameraComponent>();
			auto& camera = cameraComponent.Camera;
			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap; // Camera
			out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
			out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetDegPerspectiveVerticalFOV();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.GetPerspectiveNearClip();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.GetPerspectiveFarClip();
			out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
			out << YAML::Key << "OrthographicNear" << YAML::Value << camera.GetOrthographicNearClip();
			out << YAML::Key << "OrthographicFar" << YAML::Value << camera.GetOrthographicFarClip();
			out << YAML::EndMap; // Camera
			out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;

			out << YAML::EndMap; // CameraComponent
		}

		if (entity.HasComponent<DirectionalLightComponent>())
		{
			out << YAML::Key << "DirectionalLightComponent";
			out << YAML::BeginMap; // DirectionalLightComponent

			auto& directionalLightComponent = entity.GetComponent<DirectionalLightComponent>();
			out << YAML::Key << "Intensity" << YAML::Value << directionalLightComponent.Intensity;
			out << YAML::Key << "Radiance" << YAML::Value << directionalLightComponent.Radiance;
			out << YAML::Key << "CastShadows" << YAML::Value << directionalLightComponent.CastShadows;
			out << YAML::Key << "SoftShadows" << YAML::Value << directionalLightComponent.SoftShadows;
			out << YAML::Key << "LightSize" << YAML::Value << directionalLightComponent.LightSize;
			out << YAML::Key << "ShadowAmount" << YAML::Value << directionalLightComponent.ShadowAmount;

			out << YAML::EndMap; // DirectionalLightComponent
		}

		if (entity.HasComponent<PointLightComponent>())
		{
			out << YAML::Key << "PointLightComponent";
			out << YAML::BeginMap; // PointLightComponent

			auto& lightComponent = entity.GetComponent<PointLightComponent>();
			out << YAML::Key << "Radiance" << YAML::Value << lightComponent.Radiance;
			out << YAML::Key << "Intensity" << YAML::Value << lightComponent.Intensity;
			out << YAML::Key << "CastShadows" << YAML::Value << lightComponent.CastsShadows;
			out << YAML::Key << "SoftShadows" << YAML::Value << lightComponent.SoftShadows;
			out << YAML::Key << "MinRadius" << YAML::Value << lightComponent.MinRadius;
			out << YAML::Key << "Radius" << YAML::Value << lightComponent.Radius;
			out << YAML::Key << "LightSize" << YAML::Value << lightComponent.LightSize;
			out << YAML::Key << "Falloff" << YAML::Value << lightComponent.Falloff;

			out << YAML::EndMap; // PointLightComponent
		}

		if (entity.HasComponent<SpotLightComponent>())
		{
			out << YAML::Key << "SpotLightComponent";
			out << YAML::BeginMap; // SpotLightComponent

			const auto& lightComponent = entity.GetComponent<SpotLightComponent>();
			out << YAML::Key << "Radiance" << YAML::Value << lightComponent.Radiance;
			out << YAML::Key << "Angle" << YAML::Value << lightComponent.Angle;
			out << YAML::Key << "AngleAttenuation" << YAML::Value << lightComponent.AngleAttenuation;
			out << YAML::Key << "CastsShadows" << YAML::Value << lightComponent.CastsShadows;
			out << YAML::Key << "SoftShadows" << YAML::Value << lightComponent.SoftShadows;
			out << YAML::Key << "Falloff" << YAML::Value << lightComponent.Falloff;
			out << YAML::Key << "Intensity" << YAML::Value << lightComponent.Intensity;
			out << YAML::Key << "Range" << YAML::Value << lightComponent.Range;

			out << YAML::EndMap; // SpotLightComponent
		}

		if (entity.HasComponent<SkyLightComponent>())
		{
			out << YAML::Key << "SkyLightComponent";
			out << YAML::BeginMap; // SkyLightComponent

			auto& skyLightComponent = entity.GetComponent<SkyLightComponent>();
			out << YAML::Key << "EnvironmentMap" << YAML::Value << (AssetManager::GetMemoryAsset(skyLightComponent.SceneEnvironment) ? (AssetHandle)0 : skyLightComponent.SceneEnvironment);
			out << YAML::Key << "Intensity" << YAML::Value << skyLightComponent.Intensity;
			out << YAML::Key << "Lod" << YAML::Value << skyLightComponent.Lod;
			out << YAML::Key << "DynamicSky" << YAML::Value << skyLightComponent.DynamicSky;
			if (skyLightComponent.DynamicSky)
				out << YAML::Key << "TurbidityAzimuthInclination" << YAML::Value << skyLightComponent.TurbidityAzimuthInclination;

			out << YAML::EndMap; // SkyLightComponent
		}

		if (entity.HasComponent<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent";
			out << YAML::BeginMap; // SpriteRendererComponent

			auto& spriteRendererComponent = entity.GetComponent<SpriteRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << spriteRendererComponent.Color;

			out << YAML::Key << "Texture" << YAML::Value << spriteRendererComponent.Texture;
			out << YAML::Key << "TilingFactor" << YAML::Value << spriteRendererComponent.TilingFactor;
			out << YAML::Key << "UVStart" << YAML::Value << spriteRendererComponent.UVStart;
			out << YAML::Key << "UVEnd" << YAML::Value << spriteRendererComponent.UVEnd;
			out << YAML::Key << "ScreenSpace" << YAML::Value << spriteRendererComponent.ScreenSpace;


			out << YAML::EndMap; // SpriteRendererComponent
		}

		if (entity.HasComponent<TextComponent>())
		{
			out << YAML::Key << "TextComponent";
			out << YAML::BeginMap; // TextComponent

			auto& textComponent = entity.GetComponent<TextComponent>();
			out << YAML::Key << "TextString" << YAML::Value << textComponent.TextString;
			out << YAML::Key << "FontHandle" << YAML::Value << textComponent.FontHandle;
			out << YAML::Key << "Color" << YAML::Value << textComponent.Color;
			out << YAML::Key << "LineSpacing" << YAML::Value << textComponent.LineSpacing;
			out << YAML::Key << "Kerning" << YAML::Value << textComponent.Kerning;
			out << YAML::Key << "MaxWidth" << YAML::Value << textComponent.MaxWidth;
			out << YAML::Key << "ScreenSpace" << YAML::Value << textComponent.ScreenSpace;
			out << YAML::Key << "DropShadow" << YAML::Value << textComponent.DropShadow;
			out << YAML::Key << "ShadowDistance" << YAML::Value << textComponent.ShadowDistance;
			out << YAML::Key << "ShadowColor" << YAML::Value << textComponent.ShadowColor;

			out << YAML::EndMap; // TextComponent
		}

		if (entity.HasComponent<RigidBody2DComponent>())
		{
			out << YAML::Key << "RigidBody2DComponent";
			out << YAML::BeginMap; // RigidBody2DComponent

			const auto& rigidbody2DComponent = entity.GetComponent<RigidBody2DComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << (int)rigidbody2DComponent.BodyType;
			out << YAML::Key << "FixedRotation" << YAML::Value << rigidbody2DComponent.FixedRotation;
			out << YAML::Key << "Mass" << YAML::Value << rigidbody2DComponent.Mass;
			out << YAML::Key << "LinearDrag" << YAML::Value << rigidbody2DComponent.LinearDrag;
			out << YAML::Key << "AngularDrag" << YAML::Value << rigidbody2DComponent.AngularDrag;
			out << YAML::Key << "GravityScale" << YAML::Value << rigidbody2DComponent.GravityScale;
			out << YAML::Key << "IsBullet" << YAML::Value << rigidbody2DComponent.IsBullet;

			out << YAML::EndMap; // RigidBody2DComponent
		}

		if (entity.HasComponent<BoxCollider2DComponent>())
		{
			out << YAML::Key << "BoxCollider2DComponent";
			out << YAML::BeginMap; // BoxCollider2DComponent

			auto& boxCollider2DComponent = entity.GetComponent<BoxCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << boxCollider2DComponent.Offset;
			out << YAML::Key << "Size" << YAML::Value << boxCollider2DComponent.Size;
			out << YAML::Key << "Density" << YAML::Value << boxCollider2DComponent.Density;
			out << YAML::Key << "Friction" << YAML::Value << boxCollider2DComponent.Friction;

			out << YAML::EndMap; // BoxCollider2DComponent
		}

		if (entity.HasComponent<CircleCollider2DComponent>())
		{
			out << YAML::Key << "CircleCollider2DComponent";
			out << YAML::BeginMap; // CircleCollider2DComponent

			auto& circleCollider2DComponent = entity.GetComponent<CircleCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << circleCollider2DComponent.Offset;
			out << YAML::Key << "Radius" << YAML::Value << circleCollider2DComponent.Radius;
			out << YAML::Key << "Density" << YAML::Value << circleCollider2DComponent.Density;
			out << YAML::Key << "Friction" << YAML::Value << circleCollider2DComponent.Friction;

			out << YAML::EndMap; // CircleCollider2DComponent
		}

		if (entity.HasComponent<RigidBodyComponent>())
		{
			out << YAML::Key << "RigidBodyComponent";
			out << YAML::BeginMap; // RigidBodyComponent

			auto& rigidbodyComponent = entity.GetComponent<RigidBodyComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << (int)rigidbodyComponent.BodyType;
			out << YAML::Key << "LayerID" << YAML::Value << rigidbodyComponent.LayerID;
			out << YAML::Key << "EnableDynamicTypeChange" << YAML::Value << rigidbodyComponent.EnableDynamicTypeChange;
			out << YAML::Key << "Mass" << YAML::Value << rigidbodyComponent.Mass;
			out << YAML::Key << "LinearDrag" << YAML::Value << rigidbodyComponent.LinearDrag;
			out << YAML::Key << "AngularDrag" << YAML::Value << rigidbodyComponent.AngularDrag;
			out << YAML::Key << "DisableGravity" << YAML::Value << rigidbodyComponent.DisableGravity;
			out << YAML::Key << "IsTrigger" << YAML::Value << rigidbodyComponent.IsTrigger;
			out << YAML::Key << "CollisionDetection" << YAML::Value << (uint32_t)rigidbodyComponent.CollisionDetection;
			out << YAML::Key << "InitialLinearVelocity" << YAML::Value << rigidbodyComponent.InitialLinearVelocity;
			out << YAML::Key << "InitialAngularVelocity" << YAML::Value << rigidbodyComponent.InitialAngularVelocity;
			out << YAML::Key << "MaxLinearVelocity" << YAML::Value << rigidbodyComponent.MaxLinearVelocity;
			out << YAML::Key << "MaxAngularVelocity" << YAML::Value << rigidbodyComponent.MaxAngularVelocity;
			out << YAML::Key << "LockedAxes" << YAML::Value << (uint32_t)rigidbodyComponent.LockedAxes;

			out << YAML::EndMap; // RigidBodyComponent
		}

		if (entity.HasComponent<CharacterControllerComponent>())
		{
			out << YAML::Key << "CharacterControllerComponent";
			out << YAML::BeginMap; // CharacterControllerComponent

			auto& ccc = entity.GetComponent<CharacterControllerComponent>();
			out << YAML::Key << "Layer" << YAML::Value << ccc.LayerID;
			out << YAML::Key << "DisableGravity" << YAML::Value << ccc.DisableGravity;
			out << YAML::Key << "ControlMovementInAir" << YAML::Value << ccc.ControlMovementInAir;
			out << YAML::Key << "ControlRotationInAir" << YAML::Value << ccc.ControlRotationInAir;
			out << YAML::Key << "SlopeLimit" << YAML::Value << ccc.SlopeLimitDeg;
			out << YAML::Key << "StepOffset" << YAML::Value << ccc.StepOffset;

			out << YAML::EndMap; // CharacterControllerComponent
		}

		if (entity.HasComponent<CompoundColliderComponent>())
		{
			out << YAML::Key << "CompoundColliderComponent";
			out << YAML::BeginMap; // CompoundColliderComponent

			auto& compoundColliderComponent = entity.GetComponent<CompoundColliderComponent>();
			out << YAML::Key << "IncludeStaticChildColliders" << YAML::Value << compoundColliderComponent.IncludeStaticChildColliders;
			out << YAML::Key << "IsImmutable" << YAML::Value << compoundColliderComponent.IsImmutable;
			out << YAML::Key << "CompoundedColliderEntities" << YAML::Value << YAML::BeginSeq;

			for (const auto& entityID : compoundColliderComponent.CompoundedColliderEntities)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "ID" << YAML::Value << entityID;
				out << YAML::EndMap;
			}

			out << YAML::EndSeq;

			out << YAML::EndMap; // CompoundColliderComponent
		}

		if (entity.HasComponent<BoxColliderComponent>())
		{
			out << YAML::Key << "BoxColliderComponent";
			out << YAML::BeginMap; // BoxColliderComponent

			auto& boxColliderComponent = entity.GetComponent<BoxColliderComponent>();
			out << YAML::Key << "Offset" << YAML::Value << boxColliderComponent.Offset;
			out << YAML::Key << "HalfSize" << YAML::Value << boxColliderComponent.HalfSize;
			out << YAML::Key << "Friction" << YAML::Value << boxColliderComponent.Material.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << boxColliderComponent.Material.Restitution;

			out << YAML::EndMap; // BoxColliderComponent
		}

		if (entity.HasComponent<SphereColliderComponent>())
		{
			out << YAML::Key << "SphereColliderComponent";
			out << YAML::BeginMap; // SphereColliderComponent

			auto& sphereColliderComponent = entity.GetComponent<SphereColliderComponent>();
			out << YAML::Key << "Radius" << YAML::Value << sphereColliderComponent.Radius;
			out << YAML::Key << "Offset" << YAML::Value << sphereColliderComponent.Offset;
			out << YAML::Key << "Friction" << YAML::Value << sphereColliderComponent.Material.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << sphereColliderComponent.Material.Restitution;

			out << YAML::EndMap; // SphereColliderComponent
		}

		if (entity.HasComponent<CapsuleColliderComponent>())
		{
			out << YAML::Key << "CapsuleColliderComponent";
			out << YAML::BeginMap; // CapsuleColliderComponent

			auto& capsuleColliderComponent = entity.GetComponent<CapsuleColliderComponent>();
			out << YAML::Key << "Radius" << YAML::Value << capsuleColliderComponent.Radius;
			out << YAML::Key << "HalfHeight" << YAML::Value << capsuleColliderComponent.HalfHeight;
			out << YAML::Key << "Offset" << YAML::Value << capsuleColliderComponent.Offset;
			out << YAML::Key << "Friction" << YAML::Value << capsuleColliderComponent.Material.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << capsuleColliderComponent.Material.Restitution;

			out << YAML::EndMap; // CapsuleColliderComponent
		}

		if (entity.HasComponent<MeshColliderComponent>())
		{
			out << YAML::Key << "MeshColliderComponent";
			out << YAML::BeginMap; // MeshColliderComponent

			auto& meshColliderComponent = entity.GetComponent<MeshColliderComponent>();

			// don't serialize asset handles of memory-only assets (because this causes spurious differences in the scene file every time it is serialized)
			if (!AssetManager::GetMemoryAsset(meshColliderComponent.ColliderAsset))
			{
				out << YAML::Key << "ColliderHandle" << YAML::Value << meshColliderComponent.ColliderAsset;
			}

			out << YAML::Key << "UseSharedShape" << YAML::Value << meshColliderComponent.UseSharedShape;
			out << YAML::Key << "Friction" << YAML::Value << meshColliderComponent.Material.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << meshColliderComponent.Material.Restitution;
			out << YAML::Key << "CollisionComplexity" << YAML::Value << (uint8_t)meshColliderComponent.CollisionComplexity;
			out << YAML::EndMap; // MeshColliderComponent
		}

		if (entity.HasComponent<AudioComponent>())
		{
			out << YAML::Key << "AudioComponent";
			out << YAML::BeginMap; // AudioComponent
			auto& audioComponent = entity.GetComponent<AudioComponent>();

			HZ_SERIALIZE_PROPERTY(StartEvent,			 audioComponent.StartEvent,				out);
			HZ_SERIALIZE_PROPERTY(StartCommandID,		 audioComponent.StartCommandID,			out);

			HZ_SERIALIZE_PROPERTY(PlayOnAwake,			 audioComponent.bPlayOnAwake,			out);
			HZ_SERIALIZE_PROPERTY(StopIfEntityDestroyed, audioComponent.bStopWhenEntityDestroyed, out);
			HZ_SERIALIZE_PROPERTY(VolumeMultiplier,		 audioComponent.VolumeMultiplier,		out);
			HZ_SERIALIZE_PROPERTY(PitchMultiplier,		 audioComponent.PitchMultiplier,		out);
			HZ_SERIALIZE_PROPERTY(AutoDestroy,			 audioComponent.bAutoDestroy,			out);

			out << YAML::EndMap; // AudioComponent
		}

		if (entity.HasComponent<AudioListenerComponent>())
		{
			out << YAML::Key << "AudioListenerComponent";
			out << YAML::BeginMap; // AudioComponent
			auto& audioListenerComponent = entity.GetComponent<AudioListenerComponent>();
			HZ_SERIALIZE_PROPERTY(Active,				audioListenerComponent.Active,					out);
			HZ_SERIALIZE_PROPERTY(ConeInnerAngle,		audioListenerComponent.ConeInnerAngleInRadians, out);
			HZ_SERIALIZE_PROPERTY(ConeOuterAngle,		audioListenerComponent.ConeOuterAngleInRadians, out);
			HZ_SERIALIZE_PROPERTY(ConeOuterGain,		audioListenerComponent.ConeOuterGain,			out);
			out << YAML::EndMap; // AudioComponent
		}

		out << YAML::EndMap; // Entity
	}

	void SceneSerializer::Serialize(const std::filesystem::path& filepath)
	{
		YAML::Emitter out;
		SerializeToYAML(out);

		// if extension is .auto, then only save if the scene has actually changed (determined by hashing serialized string)
		if (auto hash = std::hash<std::string>()(out.c_str()); (filepath.extension() != ".auto") || (hash != m_Scene->m_LastSerializeHash)) {
			std::ofstream fout(filepath);
			fout << out.c_str();
			m_Scene->m_LastSerializeHash = hash;
		}
	}

	void SceneSerializer::SerializeToYAML(YAML::Emitter& out)
	{
		// Re-initialise all animation components as otherwise each time we serialize then
		// all of the bone entity transforms will be slightly different which is annoying
		// (e.g. for version control of scene files)
		auto entities = m_Scene->GetAllEntitiesWith<AnimationComponent>();
		for (auto e : entities)
		{
			Entity entity = { e, m_Scene.Raw() };
			auto& anim = entity.GetComponent<AnimationComponent>();
			if (anim.AnimationGraph)
			{
				anim.AnimationGraph->Init();
			}
		}
		m_Scene->UpdateAnimation(0.0f, false, nullptr);

		out << YAML::BeginMap;
		out << YAML::Key << "Scene";
		out << YAML::Value << m_Scene->GetName();

		out << YAML::Key << "Entities";
		out << YAML::Value << YAML::BeginSeq;

		// Sort entities by UUID (for better serializing)
		std::map<UUID, entt::entity> sortedEntityMap;
		auto idComponentView = m_Scene->m_Registry.view<IDComponent>();
		for (auto entity : idComponentView)
			sortedEntityMap[idComponentView.get<IDComponent>(entity).ID] = entity;

		// Serialize sorted entities
		for (auto [id, entity] : sortedEntityMap)
			SerializeEntity(out, { entity, m_Scene.Raw() }, m_Scene);

		out << YAML::EndSeq;

		// Scene Audio
		MiniAudioEngine::Get().SerializeSceneAudio(out, m_Scene);

		out << YAML::EndMap;
	}

	bool SceneSerializer::DeserializeFromYAML(const std::string& yamlString)
	{
		YAML::Node data = YAML::Load(yamlString);
		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		HZ_CORE_INFO_TAG("AssetManager", "Deserializing scene '{0}'", sceneName);
		m_Scene->SetName(sceneName);

		auto entities = data["Entities"];
		if (entities)
			DeserializeEntities(entities, m_Scene);

		auto view = m_Scene->GetAllEntitiesWith<TagComponent>();
		for (auto entity : view)
		{
			Entity e = { entity, m_Scene.Raw() };
			if (!e.GetParent())
				m_Scene->BuildBoneEntityIds(e);
		}

		auto sceneAudio = data["SceneAudio"];
		if (sceneAudio)
			MiniAudioEngine::Get().DeserializeSceneAudio(sceneAudio);

		// Sort IdComponent by by entity handle (which is essentially the order in which they were created)
		// This ensures a consistent ordering when iterating IdComponent (for example: when rendering scene hierarchy panel)
		m_Scene->m_Registry.sort<IDComponent>([this](const auto lhs, const auto rhs)
		{
			auto lhsEntity = m_Scene->m_EntityIDMap.find(lhs.ID);
			auto rhsEntity = m_Scene->m_EntityIDMap.find(rhs.ID);
			return static_cast<uint32_t>(lhsEntity->second) < static_cast<uint32_t>(rhsEntity->second);
		});

		for (auto entity : m_Scene->GetAllEntitiesWith<AnimationComponent>())
		{
			Entity e = { entity, m_Scene.Raw() };
			if(auto& anim = e.GetComponent<AnimationComponent>(); anim.AnimationGraph)
			{
				anim.BoneEntityIds = m_Scene->FindBoneEntityIds(e, e, anim.AnimationGraph->GetSkeleton());
			}
		}

		return true;
	}

	void SceneSerializer::SerializeRuntime(AssetHandle scene)
	{
		// Not implemented
		HZ_CORE_ASSERT(false);
	}

	void SceneSerializer::DeserializeEntities(YAML::Node& entitiesNode, Ref<Scene> scene)
	{
		for (auto entity : entitiesNode)
		{
			uint64_t uuid = entity["Entity"].as<uint64_t>();

			std::string name;
			auto tagComponent = entity["TagComponent"];
			if (tagComponent)
				name = tagComponent["Tag"].as<std::string>();

			//HZ_CORE_INFO("Deserialized Entity '{0}' with ID '{1}'", name, uuid);

			Entity deserializedEntity = scene->CreateEntityWithID(uuid, name, false);

			auto& relationshipComponent = deserializedEntity.GetComponent<RelationshipComponent>();
			uint64_t parentHandle = entity["Parent"] ? entity["Parent"].as<uint64_t>() : 0;
			relationshipComponent.ParentHandle = parentHandle;

			auto children = entity["Children"];
			if (children)
			{
				for (auto child : children)
				{
					uint64_t childHandle = child["Handle"].as<uint64_t>();
					relationshipComponent.Children.push_back(childHandle);
				}
			}

			auto prefabComponent = entity["PrefabComponent"];
			if (prefabComponent)
			{
				auto& pb = deserializedEntity.AddComponent<PrefabComponent>();

				pb.PrefabID = prefabComponent["Prefab"].as<uint64_t>();
				pb.EntityID = prefabComponent["Entity"].as<uint64_t>();
			}

			auto transformComponent = entity["TransformComponent"];
			if (transformComponent)
			{
				// Entities always have transforms
				auto& transform = deserializedEntity.GetComponent<TransformComponent>();
				transform.Translation = transformComponent["Position"].as<glm::vec3>(glm::vec3(0.0f));
				auto rotationNode = transformComponent["Rotation"];
				// Some versions of Hazel serialized rotations as quaternions
				// They should be serialized as Euler angles (this is the only way to support rotations > 360 degrees)
				// If you encounter this VERIFY, then you can uncomment this code, load your scene in and then save it
				// That will convert rotations back to Euler angles, and you can then re-comment out this code.
				//HZ_CORE_VERIFY(rotationNode.size() == 3, "Transform component rotation should be serialized as Euler angles. Found Quaternions!");
				if (rotationNode.size() == 4)
				{
					transform.SetRotation(transformComponent["Rotation"].as<glm::quat>(glm::quat()));
				}
				else
				{
					transform.SetRotationEuler(transformComponent["Rotation"].as<glm::vec3>(glm::vec3(0.0f)));
				}
				transform.Scale = transformComponent["Scale"].as<glm::vec3>();
			}

			auto scriptComponent = entity["ScriptComponent"];
			if (scriptComponent)
			{
				uint64_t scriptID = scriptComponent["ScriptID"].as<uint64_t>(0);

				if (scriptID == 0)
				{
					scriptID = scriptComponent["ClassHandle"].as<uint64_t>(0);
				}

				if (scriptID != 0)
				{
					auto& scriptEngine = ScriptEngine::GetMutable();

					if (scriptEngine.IsValidScript(scriptID))
					{
						const auto& scriptMetadata = scriptEngine.GetScriptMetadata(scriptID);

						ScriptComponent& sc = deserializedEntity.AddComponent<ScriptComponent>();
						sc.ScriptID = scriptID;

						scene->m_ScriptStorage.InitializeEntityStorage(scriptID, deserializedEntity.GetUUID());

						bool oldFormat = false;

						auto fieldsArray = scriptComponent["Fields"];

						if (!fieldsArray)
						{
							fieldsArray = scriptComponent["StoredFields"];
							oldFormat = true;
						}

						for (auto field : fieldsArray)
						{
							uint32_t fieldID = field["ID"].as<uint32_t>(0);
							auto fieldName = field["Name"].as<std::string>("");

							if (oldFormat)
							{
								// Old format, try generating id from name
								auto fullFieldName = std::format("{}.{}", scriptMetadata.FullName, fieldName);
								fieldID = Hash::GenerateFNVHash(fullFieldName);
							}

							if (scriptMetadata.Fields.contains(fieldID))
							{
								const auto& fieldMetadata = scriptMetadata.Fields.at(fieldID);
								auto& fieldStorage = scene->m_ScriptStorage.EntityStorage.at(deserializedEntity.GetUUID()).Fields[fieldID];

								auto valueNode = oldFormat ? field["Data"] : field["Value"];

								if (fieldStorage.IsArray())
								{
									HZ_CORE_VERIFY(valueNode.IsSequence());
									fieldStorage.Resize(valueNode.size());

									for (int32_t i = 0; i < valueNode.size(); i++)
									{
										switch (fieldMetadata.Type)
										{
											case DataType::SByte:
											{
												fieldStorage.SetValue(valueNode[i].as<int8_t>(), i);
												break;
											}
											case DataType::Byte:
											{
												fieldStorage.SetValue(valueNode[i].as<uint8_t>(), i);
												break;
											}
											case DataType::Short:
											{
												fieldStorage.SetValue(valueNode[i].as<int16_t>(), i);
												break;
											}
											case DataType::UShort:
											{
												fieldStorage.SetValue(valueNode[i].as<uint16_t>(), i);
												break;
											}
											case DataType::Int:
											{
												fieldStorage.SetValue(valueNode[i].as<int32_t>(), i);
												break;
											}
											case DataType::UInt:
											{
												fieldStorage.SetValue(valueNode[i].as<uint32_t>(), i);
												break;
											}
											case DataType::Long:
											{
												fieldStorage.SetValue(valueNode[i].as<int64_t>(), i);
												break;
											}
											case DataType::ULong:
											{
												fieldStorage.SetValue(valueNode[i].as<uint64_t>(), i);
												break;
											}
											case DataType::Float:
											{
												fieldStorage.SetValue(valueNode[i].as<float>(), i);
												break;
											}
											case DataType::Double:
											{
												fieldStorage.SetValue(valueNode[i].as<double>(), i);
												break;
											}
											case DataType::Vector2:
											{
												fieldStorage.SetValue(valueNode[i].as<glm::vec2>(), i);
												break;
											}
											case DataType::Vector3:
											{
												fieldStorage.SetValue(valueNode[i].as<glm::vec3>(), i);
												break;
											}
											case DataType::Vector4:
											{
												fieldStorage.SetValue(valueNode[i].as<glm::vec4>(), i);
												break;
											}
											case DataType::Entity:
											{
												fieldStorage.SetValue(valueNode[i].as<uint64_t>(), i);
												break;
											}
											case DataType::Prefab:
											{
												fieldStorage.SetValue(valueNode[i].as<uint64_t>(), i);
												break;
											}
											case DataType::Mesh:
											{
												fieldStorage.SetValue(valueNode[i].as<uint64_t>(), i);
												break;
											}
											case DataType::StaticMesh:
											{
												fieldStorage.SetValue(valueNode[i].as<uint64_t>(), i);
												break;
											}
											case DataType::Material:
											{
												fieldStorage.SetValue(valueNode[i].as<uint64_t>(), i);
												break;
											}
											case DataType::Texture2D:
											{
												fieldStorage.SetValue(valueNode[i].as<uint64_t>(), i);
												break;
											}
											case DataType::Scene:
											{
												fieldStorage.SetValue(valueNode[i].as<uint64_t>(), i);
												break;
											}
											default:
												break;
										}
									}
								}
								else
								{
									switch (fieldMetadata.Type)
									{
										case DataType::SByte:
										{
											fieldStorage.SetValue(valueNode.as<int8_t>());
											break;
										}
										case DataType::Byte:
										{
											fieldStorage.SetValue(valueNode.as<uint8_t>());
											break;
										}
										case DataType::Short:
										{
											fieldStorage.SetValue(valueNode.as<int16_t>());
											break;
										}
										case DataType::UShort:
										{
											fieldStorage.SetValue(valueNode.as<uint16_t>());
											break;
										}
										case DataType::Int:
										{
											fieldStorage.SetValue(valueNode.as<int32_t>());
											break;
										}
										case DataType::UInt:
										{
											fieldStorage.SetValue(valueNode.as<uint32_t>());
											break;
										}
										case DataType::Long:
										{
											fieldStorage.SetValue(valueNode.as<int64_t>());
											break;
										}
										case DataType::ULong:
										{
											fieldStorage.SetValue(valueNode.as<uint64_t>());
											break;
										}
										case DataType::Float:
										{
											fieldStorage.SetValue(valueNode.as<float>());
											break;
										}
										case DataType::Double:
										{
											fieldStorage.SetValue(valueNode.as<double>());
											break;
										}
										case DataType::Vector2:
										{
											fieldStorage.SetValue(valueNode.as<glm::vec2>());
											break;
										}
										case DataType::Vector3:
										{
											fieldStorage.SetValue(valueNode.as<glm::vec3>());
											break;
										}
										case DataType::Vector4:
										{
											fieldStorage.SetValue(valueNode.as<glm::vec4>());
											break;
										}
										case DataType::Bool:
											break;
										case DataType::Entity:
										{
											fieldStorage.SetValue(valueNode.as<uint64_t>());
											break;
										}
										case DataType::Prefab:
										{
											fieldStorage.SetValue(valueNode.as<uint64_t>());
											break;
										}
										case DataType::Mesh:
										{
											fieldStorage.SetValue(valueNode.as<uint64_t>());
											break;
										}
										case DataType::StaticMesh:
										{
											fieldStorage.SetValue(valueNode.as<uint64_t>());
											break;
										}
										case DataType::Material:
										{
											fieldStorage.SetValue(valueNode.as<uint64_t>());
											break;
										}
										case DataType::Texture2D:
										{
											fieldStorage.SetValue(valueNode.as<uint64_t>());
											break;
										}
										case DataType::Scene:
										{
											fieldStorage.SetValue(valueNode.as<uint64_t>());
											break;
										}
										default:
											break;
									}
								}
							}
						}
					}
				}
				else
				{
					HZ_CORE_ERROR_TAG("Scripting", "Failed to deserialize ScriptComponent on entity '{}', script id of 0 is not valid.", uuid);
				}
			}

			auto deserializeSubmesh = [&deserializedEntity](YAML::Node submeshNode) {
				auto& component = deserializedEntity.AddComponent<SubmeshComponent>();

				AssetHandle assetHandle = submeshNode["AssetID"].as<uint64_t>();
				if (AssetManager::IsAssetHandleValid(assetHandle))
				{
					AssetType type = AssetManager::GetAssetType(assetHandle);
					if (type == AssetType::Mesh)
						component.Mesh = assetHandle;
				}

				if (submeshNode["SubmeshIndex"])
					component.SubmeshIndex = submeshNode["SubmeshIndex"].as<uint32_t>();

				if (submeshNode["MaterialTable"])
				{
					YAML::Node materialTableNode = submeshNode["MaterialTable"];
					for (auto materialEntry : materialTableNode)
					{
						uint32_t index = materialEntry.first.as<uint32_t>();
						AssetHandle materialAsset = materialEntry.second.as<AssetHandle>();
						if (materialAsset && AssetManager::IsAssetHandleValid(materialAsset))
							component.MaterialTable->SetMaterial(index, materialAsset);
					}
				}

				if (submeshNode["Visible"])
					component.Visible = submeshNode["Visible"].as<bool>();
			};

			auto meshComponent = entity["MeshComponent"];
			if (meshComponent)
			{
				auto& component = deserializedEntity.AddComponent<MeshComponent>();

				AssetHandle assetHandle = meshComponent["AssetID"].as<uint64_t>();
				if (AssetManager::IsAssetHandleValid(assetHandle))
				{
					AssetType type = AssetManager::GetAssetType(assetHandle);
					if (type == AssetType::Mesh)
						component.Mesh = assetHandle;
				}
				if (meshComponent["SubmeshIndex"])
				{
					deserializeSubmesh(meshComponent);
				}
			}

			auto submeshComponent = entity["SubmeshComponent"];
			if (submeshComponent)
				deserializeSubmesh(submeshComponent);

			auto meshTagComponent = entity["MeshTagComponent"];
			if (meshTagComponent)
			{
				auto& component = deserializedEntity.AddComponent<MeshTagComponent>();
				component.MeshEntity = meshTagComponent["MeshEntity"].as<uint64_t>(0);
			}

			auto staticMeshComponent = entity["StaticMeshComponent"];
			if (staticMeshComponent)
			{
				auto& component = deserializedEntity.AddComponent<StaticMeshComponent>();

				AssetHandle assetHandle = staticMeshComponent["AssetID"].as<uint64_t>();
				if (AssetManager::IsAssetHandleValid(assetHandle))
					component.StaticMesh = assetHandle;

				if (staticMeshComponent["MaterialTable"])
				{
					YAML::Node materialTableNode = staticMeshComponent["MaterialTable"];
					for (auto materialEntry : materialTableNode)
					{
						uint32_t index = materialEntry.first.as<uint32_t>();
						AssetHandle materialAsset = materialEntry.second.as<AssetHandle>(0);
						if (materialAsset && AssetManager::IsAssetHandleValid(materialAsset))
							component.MaterialTable->SetMaterial(index, materialAsset);
					}
				}

				if (staticMeshComponent["Visible"])
					component.Visible = staticMeshComponent["Visible"].as<bool>();
			}

			auto animationComponent = entity["AnimationComponent"];
			if (animationComponent)
			{
				auto& component = deserializedEntity.AddComponent<AnimationComponent>();
				AssetHandle animationGraphHandle = animationComponent["AnimationGraph"].as<uint64_t>(0);
				if (AssetManager::IsAssetHandleValid(animationGraphHandle))
				{
					AssetType type = AssetManager::GetAssetType(animationGraphHandle);
					if (type == AssetType::AnimationGraph)
					{
						component.AnimationGraphHandle = animationGraphHandle;
						Ref<AnimationGraphAsset> animGraph = AssetManager::GetAsset<AnimationGraphAsset>(animationGraphHandle);
						if (animGraph)
						{
							component.AnimationGraph = animGraph->CreateInstance();
						}
					}
				}
				auto graphInputs = animationComponent["GraphInputs"];
				if (graphInputs && component.AnimationGraph)
				{
					for (auto input : graphInputs)
					{
						auto id = input.first.as<uint32_t>();
						try
						{
							auto value = component.AnimationGraph->InValue(id);
							if (value.isBool())         value.set(input.second.as<bool>());
							else if (value.isInt32())   value.set(input.second.as<int32_t>());
							else if (value.isInt64())   value.set(input.second.as<int64_t>());
							else if(value.isFloat32())  value.set(input.second.as<float>());
							else if (value.isFloat64()) value.set(input.second.as<double>());
							else if (value.isObjectWithClassName(type::type_name<glm::vec3>()))
							{
								glm::vec3 v = input.second.as<glm::vec3>();
								value.getObjectMemberAt(0).value.set(v.x);
								value.getObjectMemberAt(1).value.set(v.y);
								value.getObjectMemberAt(2).value.set(v.z);
							}
						}
						catch (const YAML::Exception& e)
						{
							// data type of input has changed since the graph was serialized.
							HZ_CONSOLE_LOG_WARN("Input with id {0}: {1} while derializing animation component.", id, e.what());
						}
						catch (const std::out_of_range&)
						{
							// id in scene is not present in animation graph.  This can happen if animation graph has been changed since scene was serialized.
							// Just ignore it.
							HZ_CONSOLE_LOG_WARN("Input with id {0} was not found in animation graph, while deserializing animation component.", id);
						}
					}
				}
			}

			auto cameraComponent = entity["CameraComponent"];
			if (cameraComponent)
			{
				auto& component = deserializedEntity.AddComponent<CameraComponent>();
				const auto& cameraNode = cameraComponent["Camera"];

				component.Camera = SceneCamera();
				auto& camera = component.Camera;

				if (cameraNode.IsMap())
				{
					if (cameraNode["ProjectionType"])
						camera.SetProjectionType((SceneCamera::ProjectionType)cameraNode["ProjectionType"].as<int>());
					if (cameraNode["PerspectiveFOV"])
						camera.SetDegPerspectiveVerticalFOV(cameraNode["PerspectiveFOV"].as<float>());
					if (cameraNode["PerspectiveNear"])
						camera.SetPerspectiveNearClip(cameraNode["PerspectiveNear"].as<float>());
					if (cameraNode["PerspectiveFar"])
						camera.SetPerspectiveFarClip(cameraNode["PerspectiveFar"].as<float>());
					if (cameraNode["OrthographicSize"])
						camera.SetOrthographicSize(cameraNode["OrthographicSize"].as<float>());
					if (cameraNode["OrthographicNear"])
						camera.SetOrthographicNearClip(cameraNode["OrthographicNear"].as<float>());
					if (cameraNode["OrthographicFar"])
						camera.SetOrthographicFarClip(cameraNode["OrthographicFar"].as<float>());
				}

				component.Primary = cameraComponent["Primary"].as<bool>();
			}

			if (auto directionalLightComponent = entity["DirectionalLightComponent"]; directionalLightComponent)
			{
				auto& component = deserializedEntity.AddComponent<DirectionalLightComponent>();
				component.Intensity = directionalLightComponent["Intensity"].as<float>(1.0f);
				component.Radiance = directionalLightComponent["Radiance"].as<glm::vec3>(glm::vec3(1.0f));
				component.CastShadows = directionalLightComponent["CastShadows"].as<bool>(true);
				component.SoftShadows = directionalLightComponent["SoftShadows"].as<bool>(true);
				component.LightSize = directionalLightComponent["LightSize"].as<float>(0.5f);
				component.ShadowAmount = directionalLightComponent["ShadowAmount"].as<float>(1.0f);
			}

			if (auto pointLightComponent = entity["PointLightComponent"]; pointLightComponent)
			{
				auto& component = deserializedEntity.AddComponent<PointLightComponent>();
				component.Radiance = pointLightComponent["Radiance"].as<glm::vec3>();
				component.Intensity = pointLightComponent["Intensity"].as<float>();
				component.CastsShadows = pointLightComponent["CastShadows"].as<bool>();
				component.SoftShadows = pointLightComponent["SoftShadows"].as<bool>();
				component.LightSize = pointLightComponent["LightSize"].as<float>();
				component.Radius = pointLightComponent["Radius"].as<float>();
				component.MinRadius = pointLightComponent["MinRadius"].as<float>();
				component.Falloff = pointLightComponent["Falloff"].as<float>();
			}

			auto spotLightComponent = entity["SpotLightComponent"];
			if (spotLightComponent)
			{
				auto& component = deserializedEntity.AddComponent<SpotLightComponent>();
				component.Radiance = spotLightComponent["Radiance"].as<glm::vec3>();
				component.Intensity = spotLightComponent["Intensity"].as<float>();
				component.CastsShadows = spotLightComponent["CastsShadows"].as<bool>();
				component.SoftShadows = spotLightComponent["SoftShadows"].as<bool>();
				component.Angle = spotLightComponent["Angle"].as<float>();
				component.Range = spotLightComponent["Range"].as<float>();
				component.Falloff = spotLightComponent["Falloff"].as<float>();
				component.AngleAttenuation = spotLightComponent["AngleAttenuation"].as<float>();
			}

			auto skyLightComponent = entity["SkyLightComponent"];
			if (skyLightComponent)
			{
				auto& component = deserializedEntity.AddComponent<SkyLightComponent>();

				if (skyLightComponent["EnvironmentMap"])
				{
					AssetHandle assetHandle = skyLightComponent["EnvironmentMap"].as<uint64_t>();
					if (AssetManager::IsAssetHandleValid(assetHandle))
					{
						component.SceneEnvironment = assetHandle;
					}
				}

				component.Intensity = skyLightComponent["Intensity"].as<float>();
				component.Lod = skyLightComponent["Lod"].as<float>(1.0f); // default matches original SkyLightComponent.Lod default

				if (skyLightComponent["DynamicSky"])
				{
					component.DynamicSky = skyLightComponent["DynamicSky"].as<bool>();
					if (component.DynamicSky)
					{
						component.TurbidityAzimuthInclination = skyLightComponent["TurbidityAzimuthInclination"].as<glm::vec3>();
						//skyLightComponent.SceneEnvironment = AssetManager::CreateMemoryOnlyAsset<Environment>(preethamEnv, preethamEnv);
					}
				}

				//if (!AssetManager::IsAssetHandleValid(component.SceneEnvironment))
				//{
				//	HZ_CORE_ERROR("Tried to load invalid environment map in Entity {0}", deserializedEntity.GetUUID());
				//}
			}

			auto spriteRendererComponent = entity["SpriteRendererComponent"];
			if (spriteRendererComponent)
			{
				auto& component = deserializedEntity.AddComponent<SpriteRendererComponent>();
				component.Color = spriteRendererComponent["Color"].as<glm::vec4>();
				if (spriteRendererComponent["Texture"])
					component.Texture = spriteRendererComponent["Texture"].as<AssetHandle>();
				component.TilingFactor = spriteRendererComponent["TilingFactor"].as<float>();
				if (spriteRendererComponent["UVStart"])
					component.UVStart = spriteRendererComponent["UVStart"].as<glm::vec2>();
				if (spriteRendererComponent["UVEnd"])
					component.UVEnd = spriteRendererComponent["UVEnd"].as<glm::vec2>();
				if (spriteRendererComponent["ScreenSpace"])
					component.ScreenSpace = spriteRendererComponent["ScreenSpace"].as<bool>();
			}

			auto textComponent = entity["TextComponent"];
			if (textComponent)
			{
				auto& component = deserializedEntity.AddComponent<TextComponent>();
				component.TextString = textComponent["TextString"].as<std::string>();
				component.TextHash = std::hash<std::string>()(component.TextString);
				AssetHandle fontHandle = textComponent["FontHandle"].as<uint64_t>();
				if (AssetManager::IsAssetHandleValid(fontHandle))
					component.FontHandle = fontHandle;
				else
					component.FontHandle = Font::GetDefaultFont()->Handle;
				component.Color = textComponent["Color"].as<glm::vec4>();
				component.LineSpacing = textComponent["LineSpacing"].as<float>();
				component.Kerning = textComponent["Kerning"].as<float>();
				component.MaxWidth = textComponent["MaxWidth"].as<float>();
				if (textComponent["ScreenSpace"])
					component.ScreenSpace = textComponent["ScreenSpace"].as<bool>();
				if (textComponent["DropShadow"])
					component.DropShadow = textComponent["DropShadow"].as<bool>();
				if (textComponent["ShadowDistance"])
					component.ShadowDistance = textComponent["ShadowDistance"].as<float>();
				if (textComponent["ShadowColor"])
					component.ShadowColor = textComponent["ShadowColor"].as<glm::vec4>();
			}

			auto rigidBody2DComponent = entity["RigidBody2DComponent"];
			if (rigidBody2DComponent)
			{
				auto& component = deserializedEntity.AddComponent<RigidBody2DComponent>();
				component.BodyType = (RigidBody2DComponent::Type)rigidBody2DComponent["BodyType"].as<int>();
				component.FixedRotation = rigidBody2DComponent["FixedRotation"] ? rigidBody2DComponent["FixedRotation"].as<bool>() : false;
				component.Mass = rigidBody2DComponent["Mass"].as<float>(1.0f);
				component.LinearDrag = rigidBody2DComponent["LinearDrag"].as<float>(0.01f);
				component.AngularDrag = rigidBody2DComponent["AngularDrag"].as<float>(0.05f);
				component.GravityScale = rigidBody2DComponent["GravityScale"].as<float>(1.0f);
				component.IsBullet = rigidBody2DComponent["IsBullet"].as<bool>(false);
			}

			auto boxCollider2DComponent = entity["BoxCollider2DComponent"];
			if (boxCollider2DComponent)
			{
				auto& component = deserializedEntity.AddComponent<BoxCollider2DComponent>();
				component.Offset = boxCollider2DComponent["Offset"].as<glm::vec2>();
				component.Size = boxCollider2DComponent["Size"].as<glm::vec2>();
				component.Density = boxCollider2DComponent["Density"] ? boxCollider2DComponent["Density"].as<float>() : 1.0f;
				component.Friction = boxCollider2DComponent["Friction"] ? boxCollider2DComponent["Friction"].as<float>() : 1.0f;
			}

			auto circleCollider2DComponent = entity["CircleCollider2DComponent"];
			if (circleCollider2DComponent)
			{
				auto& component = deserializedEntity.AddComponent<CircleCollider2DComponent>();
				component.Offset = circleCollider2DComponent["Offset"].as<glm::vec2>();
				component.Radius = circleCollider2DComponent["Radius"].as<float>();
				component.Density = circleCollider2DComponent["Density"] ? circleCollider2DComponent["Density"].as<float>() : 1.0f;
				component.Friction = circleCollider2DComponent["Friction"] ? circleCollider2DComponent["Friction"].as<float>() : 1.0f;
			}

			auto rigidBodyComponent = entity["RigidBodyComponent"];
			if (rigidBodyComponent)
			{
				auto& component = deserializedEntity.AddComponent<RigidBodyComponent>();
				component.BodyType = (EBodyType)rigidBodyComponent["BodyType"].as<int>(0);

				if (rigidBodyComponent["IsKinematic"].as<bool>(false))
					component.BodyType = EBodyType::Kinematic;

				component.LayerID = rigidBodyComponent["LayerID"].as<uint32_t>(0);
				component.EnableDynamicTypeChange = rigidBodyComponent["EnableDynamicTypeChange"].as<bool>(false);
				component.Mass = rigidBodyComponent["Mass"].as<float>(1.0f);
				component.LinearDrag = rigidBodyComponent["LinearDrag"].as<float>(0.0f);
				component.AngularDrag = rigidBodyComponent["AngularDrag"].as<float>(0.05f);
				component.DisableGravity = rigidBodyComponent["DisableGravity"].as<bool>(false);
				component.IsTrigger = rigidBodyComponent["IsTrigger"].as<bool>(false);
				component.CollisionDetection = (ECollisionDetectionType)rigidBodyComponent["CollisionDetection"].as<uint32_t>(0);
				component.InitialLinearVelocity = rigidBodyComponent["InitialLinearVelocity"].as<glm::vec3>(glm::vec3(0.0f));
				component.InitialAngularVelocity = rigidBodyComponent["InitialAngularVelocity"].as<glm::vec3>(glm::vec3(0.0f));
				component.MaxLinearVelocity = rigidBodyComponent["MaxLinearVelocity"].as<float>(500.0f);
				component.MaxAngularVelocity = rigidBodyComponent["MaxAngularVelocity"].as<float>(50.0f);

				auto lockedAxesNode = rigidBodyComponent["LockedAxes"] ? rigidBodyComponent["LockedAxes"] : rigidBodyComponent["LockFlags"];
				component.LockedAxes = (EActorAxis)lockedAxesNode.as<uint32_t>(0);
			}

			auto characterControllerComponent = entity["CharacterControllerComponent"];
			if (characterControllerComponent)
			{
				auto& component = deserializedEntity.AddComponent<CharacterControllerComponent>();
				component.LayerID = characterControllerComponent["Layer"].as<uint32_t>(0);
				component.DisableGravity = characterControllerComponent["DisableGravity"].as<bool>(false);
				component.ControlMovementInAir = characterControllerComponent["ControlMovementInAir"].as<bool>(false);
				component.ControlRotationInAir = characterControllerComponent["ControlRotationInAir"].as<bool>(false);
				component.SlopeLimitDeg = characterControllerComponent["SlopeLimit"].as<float>(0.0f);
				component.StepOffset = characterControllerComponent["StepOffset"].as<float>(0.0f);
			}

			auto compoundColliderComponent = entity["CompoundColliderComponent"];
			if (compoundColliderComponent)
			{
				auto& component = deserializedEntity.AddComponent<CompoundColliderComponent>();
				component.IncludeStaticChildColliders = compoundColliderComponent["IncludeStaticChildColliders"].as<bool>(false);
				component.IsImmutable = compoundColliderComponent["IsImmutable"].as<bool>(false);

				auto compoundedChildEntities = compoundColliderComponent["CompoundedColliderEntities"];
				if (compoundedChildEntities)
				{
					for (auto childID : compoundedChildEntities)
					{
						component.CompoundedColliderEntities.push_back(childID["ID"].as<UUID>(0));
					}
				}
			}

			auto boxColliderComponent = entity["BoxColliderComponent"];
			if (boxColliderComponent)
			{
				auto& component = deserializedEntity.AddComponent<BoxColliderComponent>();
				component.Offset = boxColliderComponent["Offset"].as<glm::vec3>();
				if (boxColliderComponent["Size"])
					component.HalfSize = boxColliderComponent["Size"].as<glm::vec3>() / 2.0f;
				else
					component.HalfSize = boxColliderComponent["HalfSize"].as<glm::vec3>(glm::vec3(0.5f));

				if (boxColliderComponent["IsTrigger"] && deserializedEntity.HasComponent<RigidBodyComponent>())
					deserializedEntity.GetComponent<RigidBodyComponent>().IsTrigger = boxColliderComponent["IsTrigger"].as<bool>(false);

				component.Material.Friction = boxColliderComponent["Friction"].as<float>(0.5f);
				component.Material.Restitution = boxColliderComponent["Restitution"].as<float>(0.15f);
			}

			auto sphereColliderComponent = entity["SphereColliderComponent"];
			if (sphereColliderComponent)
			{
				auto& component = deserializedEntity.AddComponent<SphereColliderComponent>();
				component.Radius = sphereColliderComponent["Radius"].as<float>();
				component.Offset = sphereColliderComponent["Offset"].as<glm::vec3>(glm::vec3(0.0f));

				if (sphereColliderComponent["IsTrigger"] && deserializedEntity.HasComponent<RigidBodyComponent>())
					deserializedEntity.GetComponent<RigidBodyComponent>().IsTrigger = sphereColliderComponent["IsTrigger"].as<bool>(false);

				component.Material.Friction = sphereColliderComponent["Friction"].as<float>(0.5f);
				component.Material.Restitution = sphereColliderComponent["Restitution"].as<float>(0.15f);
			}

			auto capsuleColliderComponent = entity["CapsuleColliderComponent"];
			if (capsuleColliderComponent)
			{
				auto& component = deserializedEntity.AddComponent<CapsuleColliderComponent>();
				component.Radius = capsuleColliderComponent["Radius"].as<float>(0.5f);

				if (capsuleColliderComponent["Height"])
					component.HalfHeight = capsuleColliderComponent["Height"].as<float>(1.0f) / 2.0f;
				else
					component.HalfHeight = capsuleColliderComponent["HalfHeight"].as<float>(0.5f);

				component.Offset = capsuleColliderComponent["Offset"].as<glm::vec3>(glm::vec3{ 0.0f, 0.0f, 0.0f });

				if (capsuleColliderComponent["IsTrigger"] && deserializedEntity.HasComponent<RigidBodyComponent>())
					deserializedEntity.GetComponent<RigidBodyComponent>().IsTrigger = capsuleColliderComponent["IsTrigger"].as<bool>(false);

				component.Material.Friction = capsuleColliderComponent["Friction"].as<float>(0.5f);
				component.Material.Restitution = capsuleColliderComponent["Restitution"].as<float>(0.15f);
			}

			auto meshColliderComponent = entity["MeshColliderComponent"];
			if (meshColliderComponent)
			{
				auto& component = deserializedEntity.AddComponent<MeshColliderComponent>();

				component.ColliderAsset = meshColliderComponent["ColliderHandle"].as<AssetHandle>(0);
				Ref<MeshColliderAsset> colliderAsset = AssetManager::GetAsset<MeshColliderAsset>(component.ColliderAsset);
				if (!colliderAsset)
				{
					// Most likely a memory only asset from a previous session, re-create the asset
					AssetHandle colliderMesh = 0;
					if (deserializedEntity.HasComponent<SubmeshComponent>())
						colliderMesh = deserializedEntity.GetComponent<SubmeshComponent>().Mesh;
					else if (deserializedEntity.HasComponent<StaticMeshComponent>())
						colliderMesh = deserializedEntity.GetComponent<StaticMeshComponent>().StaticMesh;

					bool overrideMesh = meshColliderComponent["OverrideMesh"].as<bool>(false);
					if (overrideMesh)
					{
						AssetHandle tempHandle = meshColliderComponent["AssetID"].as<uint64_t>(0);
						overrideMesh = AssetManager::IsAssetHandleValid(tempHandle);
						colliderMesh = overrideMesh ? tempHandle : colliderMesh;
					}

					component.ColliderAsset = AssetManager::AddMemoryOnlyAsset(Ref<MeshColliderAsset>::Create(colliderMesh));
					colliderAsset = AssetManager::GetAsset<MeshColliderAsset>(component.ColliderAsset);
					if (!colliderAsset)
					{
						HZ_CORE_WARN("MeshColliderComponent in use without valid mesh!");
					}

					if (auto colliderAsset = AssetManager::GetAsset<MeshColliderAsset>(component.ColliderAsset); colliderAsset)
					{
					}
					else
					{
						HZ_CORE_WARN("MeshColliderComponent in use without valid mesh!");
					}
				}

				component.UseSharedShape = meshColliderComponent["UseSharedShape"].as<bool>(false);
				//component.OverrideMaterial = meshColliderComponent["OverrideMaterial"].as<AssetHandle>(0);
				colliderAsset->CollisionComplexity = component.CollisionComplexity;
				component.Material.Friction = meshColliderComponent["Friction"].as<float>(0.5f);
				component.Material.Restitution = meshColliderComponent["Restitution"].as<float>(0.15f);
				if (colliderAsset && !PhysicsSystem::GetMeshCache().Exists(colliderAsset))
				{
					PhysicsSystem::GetMeshCookingFactory()->CookMesh(component.ColliderAsset);
				}

				if (meshColliderComponent["IsTrigger"] && deserializedEntity.HasComponent<RigidBodyComponent>())
					deserializedEntity.GetComponent<RigidBodyComponent>().IsTrigger = meshColliderComponent["IsTrigger"].as<bool>(false);
			}

			// NOTE(Peter): This can probably be removed eventually
			auto physicsLayerComponent = entity["PhysicsLayerComponent"];
			if (physicsLayerComponent)
			{
				if (!deserializedEntity.HasComponent<RigidBodyComponent>() && deserializedEntity.HasAny<BoxColliderComponent, SphereColliderComponent, CapsuleColliderComponent, MeshColliderComponent>())
					deserializedEntity.AddComponent<RigidBodyComponent>();

				if (deserializedEntity.HasComponent<RigidBodyComponent>())
					deserializedEntity.GetComponent<RigidBodyComponent>().LayerID = physicsLayerComponent["LayerID"].as<uint32_t>(0);
			}

			auto audioComponent = entity["AudioComponent"];
			if (audioComponent)
			{
				auto& component = deserializedEntity.AddComponent<AudioComponent>();

				HZ_DESERIALIZE_PROPERTY(StartEvent, component.StartEvent, audioComponent, std::string(""));
				component.StartCommandID = audioComponent["StartCommandID"] ? Audio::CommandID::FromUnsignedInt(audioComponent["StartCommandID"].as<uint32_t>()) : Audio::CommandID::InvalidID();

				HZ_DESERIALIZE_PROPERTY(PlayOnAwake, component.bPlayOnAwake, audioComponent, false);
				HZ_DESERIALIZE_PROPERTY(StopIfEntityDestroyed, component.bStopWhenEntityDestroyed, audioComponent, true);
				HZ_DESERIALIZE_PROPERTY(VolumeMultiplier, component.VolumeMultiplier, audioComponent, 1.0f);
				HZ_DESERIALIZE_PROPERTY(PitchMultiplier, component.PitchMultiplier, audioComponent, 1.0f);
				HZ_DESERIALIZE_PROPERTY(AutoDestroy, component.bAutoDestroy, audioComponent, false);
			}

			auto audioListener = entity["AudioListenerComponent"];
			if (audioListener)
			{
				auto& component = deserializedEntity.AddComponent<AudioListenerComponent>();
				component.Active = audioListener["Active"] ? audioListener["Active"].as<bool>() : false;
				component.ConeInnerAngleInRadians = audioListener["ConeInnerAngle"] ? audioListener["ConeInnerAngle"].as<float>() : 6.283185f;
				component.ConeOuterAngleInRadians = audioListener["ConeOuterAngle"] ? audioListener["ConeOuterAngle"].as<float>() : 6.283185f;
				component.ConeOuterGain = audioListener["ConeOuterGain"] ? audioListener["ConeOuterGain"].as<float>() : 1.0f;
			}
		}

		scene->SortEntities();
	}

	bool SceneSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		HZ_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		try
		{
			DeserializeFromYAML(strStream.str());
		}
		catch (const YAML::Exception& e)
		{
			HZ_CONSOLE_LOG_ERROR("Failed to deserialize scene '{0}': {1}", filepath.string(), e.what());
			return false;
		}

		// Asset handle
		const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(filepath);
		m_Scene->Handle = metadata.Handle;

		// NOTE(Peter): Fix for "UntitledScene" name, hardcoded which isn't good
		if (m_Scene->GetName() == "UntitledScene")
			m_Scene->SetName(Utils::RemoveExtension(filepath.filename().string()));

		return true;
	}

	bool SceneSerializer::DeserializeRuntime(AssetHandle scene)
	{
		// Not implemented
		HZ_CORE_ASSERT(false);
		return false;
	}

	bool SceneSerializer::SerializeToAssetPack(FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		YAML::Emitter out;
		SerializeToYAML(out);

		outInfo.Offset = stream.GetStreamPosition();
		std::string yamlString = out.c_str();
		stream.WriteString(yamlString);
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	bool SceneSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::SceneInfo& sceneInfo)
	{
		stream.SetStreamPosition(sceneInfo.PackedOffset);
		std::string sceneYAML;
		stream.ReadString(sceneYAML);

		return DeserializeFromYAML(sceneYAML);
	}

	bool SceneSerializer::DeserializeReferencedPrefabs(const std::filesystem::path& filepath, std::unordered_set<AssetHandle>& outPrefabs)
	{
		std::ifstream stream(filepath);
		HZ_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Scene"])
			return false;

		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				auto scriptComponent = entity["ScriptComponent"];
				auto storedFields = scriptComponent["StoredFields"];
				if (storedFields)
				{
					for (auto field : storedFields)
					{
						uint32_t id = field["ID"].as<uint32_t>(0);
						std::string name = field["Name"].as<std::string>();
						uint32_t type = field["Type"].as<uint32_t>();
						if (type == 18) // Prefab
						{
							uint64_t data = field["Data"].as<uint64_t>();
							if (AssetManager::IsAssetHandleValid((AssetHandle)data))
							{
								HZ_CORE_INFO("Adding {}", name);
								outPrefabs.insert((AssetHandle)data);
							}
						}
					}
				}
			}
		}
		return outPrefabs.size() > 0;
	}

}

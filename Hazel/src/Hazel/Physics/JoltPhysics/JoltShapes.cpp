#include "hzpch.h"
#include "JoltShapes.h"
#include "JoltAPI.h"
#include "JoltBinaryStream.h"

#include "Hazel/Physics/PhysicsSystem.h"

#include "Hazel/Asset/AssetManager.h"

#include "Hazel/Debug/Profiler.h"

#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

namespace Hazel {

	JoltImmutableCompoundShape::JoltImmutableCompoundShape(Entity entity)
		: ImmutableCompoundShape(entity)
	{
	}

	void JoltImmutableCompoundShape::SetMaterial(const ColliderMaterial& material)
	{
	}

	void JoltImmutableCompoundShape::AddShape(const Ref<PhysicsShape>& shape)
	{
		Ref<Scene> scene = Scene::GetScene(m_Entity.GetSceneUUID());
		const TransformComponent rigidBodyTransform = scene->GetWorldSpaceTransform(m_Entity);
		const TransformComponent& shapeTransform = shape->GetEntity().Transform();

		JPH::Shape* nativeShape = static_cast<JPH::Shape*>(shape->GetNativeShape());

		JPH::Vec3 translation = JPH::Vec3::sZero();
		JPH::Quat rotation = JPH::Quat::sIdentity();

		if (m_Entity != shape->GetEntity())
		{
			translation = JoltUtils::ToJoltVector(shapeTransform.Translation * rigidBodyTransform.Scale);
			rotation = JoltUtils::ToJoltQuat(shapeTransform.GetRotation());
		}

		m_Settings.AddShape(translation, rotation, nativeShape);
	}

	void JoltImmutableCompoundShape::RemoveShape(const Ref<PhysicsShape>& shape)
	{
		HZ_CORE_VERIFY(false, "Cannot remove shape from an ImmutableCompoundShape!");
	}

	void JoltImmutableCompoundShape::Create()
	{
		const JoltAPI* api = (const JoltAPI*)PhysicsSystem::GetAPI();

		JPH::ShapeSettings::ShapeResult result = m_Settings.Create(*api->GetTempAllocator());

		if (result.HasError())
		{
			HZ_CORE_ERROR_TAG("Physics", "Failed to create Immutable Compound Shape. Error: {}", result.GetError());
			return;
		}

		m_Shape = JoltUtils::CastJoltRef<JPH::StaticCompoundShape>(result.Get());
		m_Shape->SetUserData(reinterpret_cast<JPH::uint64>(this));
	}

	JoltMutableCompoundShape::JoltMutableCompoundShape(Entity entity)
		: MutableCompoundShape(entity)
	{

	}

	void JoltMutableCompoundShape::SetMaterial(const ColliderMaterial& material)
	{

	}

	void JoltMutableCompoundShape::AddShape(const Ref<PhysicsShape>& shape)
	{
		Ref<Scene> scene = Scene::GetScene(m_Entity.GetSceneUUID());
		const TransformComponent rigidBodyTransform = scene->GetWorldSpaceTransform(m_Entity);
		const TransformComponent& shapeTransform = shape->GetEntity().Transform();

		JPH::Shape* nativeShape = static_cast<JPH::Shape*>(shape->GetNativeShape());

		JPH::Vec3 translation = JPH::Vec3::sZero();
		JPH::Quat rotation = JPH::Quat::sIdentity();

		if (m_Entity != shape->GetEntity())
		{
			translation = JoltUtils::ToJoltVector(shapeTransform.Translation * rigidBodyTransform.Scale);
			rotation = JoltUtils::ToJoltQuat(shapeTransform.GetRotation());
		}

		m_Settings.AddShape(translation, rotation, nativeShape);
	}

	void JoltMutableCompoundShape::RemoveShape(const Ref<PhysicsShape>& shape)
	{
	}

	void JoltMutableCompoundShape::Create()
	{
		JPH::ShapeSettings::ShapeResult result = m_Settings.Create();

		if (result.HasError())
		{
			HZ_CORE_ERROR_TAG("Physics", "Failed to create Mutable Compound Shape. Error: {}", result.GetError());
			return;
		}

		m_Shape = JoltUtils::CastJoltRef<JPH::MutableCompoundShape>(result.Get());
		m_Shape->SetUserData(reinterpret_cast<JPH::uint64>(this));
	}

	JoltBoxShape::JoltBoxShape(Entity entity, float totalBodyMass)
		: BoxShape(entity)
	{
		auto scene = Scene::GetScene(entity.GetSceneUUID());
		TransformComponent worldTransform = scene->GetWorldSpaceTransform(entity);
		const auto& boxColliderComponent = entity.GetComponent<BoxColliderComponent>();

		glm::vec3 halfColliderSize = glm::abs(worldTransform.Scale * boxColliderComponent.HalfSize);
		float volume = halfColliderSize.x * 2.0f * halfColliderSize.y * 2.0f * halfColliderSize.z * 2.0f;

		m_Material = JoltMaterial::FromColliderMaterial(boxColliderComponent.Material);

		m_Settings = new JPH::BoxShapeSettings(JoltUtils::ToJoltVector(halfColliderSize), 0.01f);
		m_Settings->mDensity = totalBodyMass / volume;
		m_Settings->mMaterial = m_Material;

		JPH::RotatedTranslatedShapeSettings offsetShapeSettings(JoltUtils::ToJoltVector(boxColliderComponent.Offset), JPH::Quat::sIdentity(), m_Settings);

		JPH::ShapeSettings::ShapeResult result = offsetShapeSettings.Create();

		if (result.HasError())
		{
			HZ_CORE_ERROR_TAG("Physics", "Failed to create BoxShape. Error: {}", result.GetError());
			return;
		}

		m_Shape = JoltUtils::CastJoltRef<JPH::RotatedTranslatedShape>(result.Get());
		m_Shape->SetUserData(reinterpret_cast<JPH::uint64>(this));
	}
	
	void JoltBoxShape::SetMaterial(const ColliderMaterial& material)
	{
		m_Material->SetFriction(material.Friction);
		m_Material->SetRestitution(material.Restitution);
	}

	JoltSphereShape::JoltSphereShape(Entity entity, float totalBodyMass)
		: SphereShape(entity)
	{
		auto scene = Scene::GetScene(entity.GetSceneUUID());
		TransformComponent worldTransform = scene->GetWorldSpaceTransform(entity);
		const auto& sphereColliderComponent = entity.GetComponent<SphereColliderComponent>();

		float largestComponent = glm::abs(glm::max(worldTransform.Scale.x, glm::max(worldTransform.Scale.y, worldTransform.Scale.z)));
		float scaledRadius = largestComponent * sphereColliderComponent.Radius;
		float volume = (4.0f / 3.0f) * glm::pi<float>() * (scaledRadius * scaledRadius * scaledRadius);

		m_Material = JoltMaterial::FromColliderMaterial(sphereColliderComponent.Material);

		m_Settings = new JPH::SphereShapeSettings(scaledRadius);
		m_Settings->mDensity = totalBodyMass / volume;
		m_Settings->mMaterial = m_Material;
		
		JPH::RotatedTranslatedShapeSettings offsetShapeSettings(JoltUtils::ToJoltVector(sphereColliderComponent.Offset), JPH::Quat::sIdentity(), m_Settings);
		JPH::ShapeSettings::ShapeResult result = offsetShapeSettings.Create();

		if (result.HasError())
		{
			HZ_CORE_ERROR_TAG("Physics", "Failed to create SphereShape. Error: {}", result.GetError());
			return;
		}

		m_Shape = JoltUtils::CastJoltRef<JPH::RotatedTranslatedShape>(result.Get());
		m_Shape->SetUserData(reinterpret_cast<JPH::uint64>(this));
	}

	void JoltSphereShape::SetMaterial(const ColliderMaterial& material)
	{
		m_Material->SetFriction(material.Friction);
		m_Material->SetRestitution(material.Restitution);
	}

	JoltCapsuleShape::JoltCapsuleShape(Entity entity, float totalBodyMass)
		: CapsuleShape(entity)
	{
		auto scene = Scene::GetScene(entity.GetSceneUUID());
		TransformComponent worldTransform = scene->GetWorldSpaceTransform(entity);
		const auto& capsuleColliderComponent = entity.GetComponent<CapsuleColliderComponent>();

		float largestRadius = glm::abs(glm::max(worldTransform.Scale.x, worldTransform.Scale.z));
		float scaledRadius = capsuleColliderComponent.Radius * largestRadius;
		float scaledHalfHeight = glm::abs(capsuleColliderComponent.HalfHeight * worldTransform.Scale.y);
		float volume = glm::pi<float>() * (scaledRadius * scaledRadius) * ((4.0f / 3.0f) * scaledRadius + (scaledHalfHeight * 2.0f));

		m_Material = JoltMaterial::FromColliderMaterial(capsuleColliderComponent.Material);

		m_Settings = new JPH::CapsuleShapeSettings(scaledHalfHeight, scaledRadius);
		m_Settings->mDensity = totalBodyMass / volume;
		m_Settings->mMaterial = m_Material;

		JPH::RotatedTranslatedShapeSettings offsetShapeSettings(JoltUtils::ToJoltVector(capsuleColliderComponent.Offset), JPH::Quat::sIdentity(), m_Settings);
		JPH::ShapeSettings::ShapeResult result = offsetShapeSettings.Create();

		if (result.HasError())
		{
			HZ_CORE_ERROR_TAG("Physics", "Failed to create CapsuleShape. Error: {}", result.GetError());
			return;
		}

		m_Shape = JoltUtils::CastJoltRef<JPH::RotatedTranslatedShape>(result.Get());
		m_Shape->SetUserData(reinterpret_cast<JPH::uint64>(this));
	}

	void JoltCapsuleShape::SetMaterial(const ColliderMaterial& material)
	{
		m_Material->SetFriction(material.Friction);
		m_Material->SetRestitution(material.Restitution);
	}

	JoltConvexMeshShape::JoltConvexMeshShape(Entity entity, float totalBodyMass)
		: ConvexMeshShape(entity)
	{
		HZ_PROFILE_FUNC();

		auto& component = entity.GetComponent<MeshColliderComponent>();

		Ref<MeshColliderAsset> colliderAsset = AssetManager::GetAsset<MeshColliderAsset>(component.ColliderAsset);
		HZ_CORE_VERIFY(colliderAsset);
		
		auto scene = Scene::GetScene(entity.GetSceneUUID());
		TransformComponent worldTransform = scene->GetWorldSpaceTransform(entity);

		const CachedColliderData& colliderData = PhysicsSystem::GetMeshCache().GetMeshData(colliderAsset);
		const auto& meshData = colliderData.SimpleColliderData;
		HZ_CORE_ASSERT(meshData.Submeshes.size() > component.SubmeshIndex);

		const SubmeshColliderData& submesh = meshData.Submeshes[component.SubmeshIndex];
		glm::vec3 submeshTranslation, submeshScale;
		glm::quat submeshRotation;
		Math::DecomposeTransform(submesh.Transform, submeshTranslation, submeshRotation, submeshScale);

		JoltBinaryStreamReader binaryReader(submesh.ColliderData);
		JPH::Shape::ShapeResult result = JPH::Shape::sRestoreFromBinaryState(binaryReader);

		if (result.HasError())
		{
			HZ_CORE_ERROR_TAG("Physics", "Failed to construct ConvexMesh: {}", result.GetError());
			return;
		}

		JPH::Ref<JPH::ConvexHullShape> convexShape = JoltUtils::CastJoltRef<JPH::ConvexHullShape>(result.Get());
		convexShape->SetDensity(totalBodyMass / convexShape->GetVolume());
		convexShape->SetMaterial(JoltMaterial::FromColliderMaterial(component.Material));

		m_Shape = new JPH::ScaledShape(convexShape, JoltUtils::ToJoltVector(submeshScale * worldTransform.Scale));
		m_Shape->SetUserData(reinterpret_cast<JPH::uint64>(this));
	}

	void JoltConvexMeshShape::SetMaterial(const ColliderMaterial& material)
	{
	}

	// NOTE(Peter): Having materials for triangle shapes is a bit tricky in Jolt...
	JoltTriangleMeshShape::JoltTriangleMeshShape(Entity entity)
		: TriangleMeshShape(entity)
	{
		HZ_PROFILE_FUNC();

		auto& component = entity.GetComponent<MeshColliderComponent>();

		Ref<MeshColliderAsset> colliderAsset = AssetManager::GetAsset<MeshColliderAsset>(component.ColliderAsset);
		HZ_CORE_VERIFY(colliderAsset);

		auto scene = Scene::GetScene(entity.GetSceneUUID());
		TransformComponent worldTransform = scene->GetWorldSpaceTransform(entity);

		const CachedColliderData& colliderData = PhysicsSystem::GetMeshCache().GetMeshData(colliderAsset);
		const auto& meshData = colliderData.ComplexColliderData;

		JPH::StaticCompoundShapeSettings compoundShapeSettings;
		for (const auto& submeshData : meshData.Submeshes)
		{
			glm::vec3 submeshTranslation, submeshScale;
			glm::quat submeshRotation;
			Math::DecomposeTransform(submeshData.Transform, submeshTranslation, submeshRotation, submeshScale);

			JoltBinaryStreamReader binaryReader(submeshData.ColliderData);
			JPH::Shape::ShapeResult result = JPH::Shape::sRestoreFromBinaryState(binaryReader);

			if (result.HasError())
			{
				HZ_CORE_ERROR_TAG("Physics", "Failed to construct ConvexMesh!");
				return;
			}

			compoundShapeSettings.AddShape(JPH::Vec3::sZero(), JPH::Quat::sIdentity(), new JPH::ScaledShape(result.Get(), JoltUtils::ToJoltVector(submeshScale * worldTransform.Scale)));
		}

		JPH::Shape::ShapeResult result = compoundShapeSettings.Create();

		if (result.HasError())
		{
			HZ_CORE_ERROR_TAG("Physics", "Failed to construct ConvexMesh: {}", result.GetError());
			return;
		}

		m_Shape = JoltUtils::CastJoltRef<JPH::StaticCompoundShape>(result.Get());
		m_Shape->SetUserData(reinterpret_cast<JPH::uint64>(this));
	}

	void JoltTriangleMeshShape::SetMaterial(const ColliderMaterial& material)
	{
	}

}

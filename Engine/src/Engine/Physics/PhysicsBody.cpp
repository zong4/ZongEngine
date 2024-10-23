#include "hzpch.h"
#include "PhysicsBody.h"
#include "PhysicsSystem.h"
#include "PhysicsAPI.h"

#include "Engine/Asset/AssetManager.h"

#include <magic_enum.hpp>
using namespace magic_enum::bitwise_operators; 

namespace Hazel {

	void PhysicsBody::SetAxisLock(EActorAxis axis, bool locked, bool forceWake)
	{
		if (locked)
			m_LockedAxes |= axis;
		else
			m_LockedAxes &= ~axis;

		OnAxisLockUpdated(forceWake);
	}

	bool PhysicsBody::IsAxisLocked(EActorAxis axis) const
	{
		return (m_LockedAxes & axis) != EActorAxis::None;
	}

	void PhysicsBody::CreateCollisionShapesForEntity(Entity entity, bool ignoreCompoundShapes)
	{
		Ref<Scene> scene = Scene::GetScene(entity.GetSceneUUID());

		if (entity.HasComponent<CompoundColliderComponent>() && !ignoreCompoundShapes)
		{
			const auto& compoundColliderComponent = entity.GetComponent<CompoundColliderComponent>();

			Ref<CompoundShape> compoundShape = nullptr;

			if (compoundColliderComponent.IsImmutable)
				compoundShape = ImmutableCompoundShape::Create(entity);
			else
				compoundShape = MutableCompoundShape::Create(entity);

			for (auto colliderEntityID : compoundColliderComponent.CompoundedColliderEntities)
			{
				Entity colliderEntity = scene->TryGetEntityWithUUID(colliderEntityID);

				if (!colliderEntity)
					continue;

				CreateCollisionShapesForEntity(colliderEntity, colliderEntity == m_Entity || colliderEntity == entity);
			}

			for (const auto& [shapeType, shapeStorage] : m_Shapes)
			{
				for (const auto& shape : shapeStorage)
					compoundShape->AddShape(shape);
			}

			compoundShape->Create();
			m_Shapes[compoundShape->GetType()].push_back(compoundShape);
			return;
		}

		const auto& rigidBodyComponent = m_Entity.GetComponent<RigidBodyComponent>();

		if (entity.HasComponent<BoxColliderComponent>())
			m_Shapes[ShapeType::Box].push_back(BoxShape::Create(entity, rigidBodyComponent.Mass));

		if (entity.HasComponent<SphereColliderComponent>())
			m_Shapes[ShapeType::Sphere].push_back(SphereShape::Create(entity, rigidBodyComponent.Mass));

		if (entity.HasComponent<CapsuleColliderComponent>())
			m_Shapes[ShapeType::Capsule].push_back(CapsuleShape::Create(entity, rigidBodyComponent.Mass));

		if (entity.HasComponent<MeshColliderComponent>())
		{
			const auto& component = entity.GetComponent<MeshColliderComponent>();

			auto colliderAsset = AssetManager::GetAsset<MeshColliderAsset>(component.ColliderAsset);
			HZ_CORE_VERIFY(colliderAsset);

			auto& meshCache = PhysicsSystem::GetMeshCache();

			if (!meshCache.Exists(colliderAsset))
			{
				auto[simpleColliderResult, complexColliderResult] = PhysicsSystem::GetMeshCookingFactory()->CookMesh(colliderAsset);

				if (simpleColliderResult != ECookingResult::Success && complexColliderResult != ECookingResult::Success)
				{
					HZ_CORE_WARN_TAG("Physics", "Tried to add an Mesh Collider with an invalid collider asset. Please make sure your collider has been cooked.");
					return;
				}
			}

			const CachedColliderData& colliderData = meshCache.GetMeshData(colliderAsset);

			if (colliderAsset->CollisionComplexity == ECollisionComplexity::UseComplexAsSimple && rigidBodyComponent.BodyType == EBodyType::Dynamic)
			{
				HZ_CONSOLE_LOG_ERROR("Entity '{0}' ({1}) has a dynamic RigidBodyComponent along with a Complex MeshColliderComponent! This isn't allowed.", m_Entity.Name(), m_Entity.GetUUID());
				return;
			}

			// Create and add simple collider
			if (colliderData.SimpleColliderData.Submeshes.size() > 0 && colliderAsset->CollisionComplexity != ECollisionComplexity::UseComplexAsSimple)
				m_Shapes[ShapeType::ConvexMesh].push_back(ConvexMeshShape::Create(entity, rigidBodyComponent.Mass));

			// Create and add complex collider
			if (colliderData.ComplexColliderData.Submeshes.size() > 0 && colliderAsset->CollisionComplexity != ECollisionComplexity::UseSimpleAsComplex)
				m_Shapes[ShapeType::TriangleMesh].push_back(TriangleMeshShape::Create(entity));
		}
	}

}

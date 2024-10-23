#include "pch.h"
#include "PhysicsShapes.h"
#include "PhysicsAPI.h"

#include "Engine/Physics/JoltPhysics/JoltShapes.h"

namespace Hazel {

	Ref<ImmutableCompoundShape> ImmutableCompoundShape::Create(Entity entity)
	{
		switch (PhysicsAPI::Current())
		{
			case PhysicsAPIType::Jolt: return Ref<JoltImmutableCompoundShape>::Create(entity);
		}

		ZONG_CORE_VERIFY(false);
		return nullptr;
	}

	Ref<MutableCompoundShape> MutableCompoundShape::Create(Entity entity)
	{
		switch (PhysicsAPI::Current())
		{
			case PhysicsAPIType::Jolt: return Ref<JoltMutableCompoundShape>::Create(entity);
		}

		ZONG_CORE_VERIFY(false);
		return nullptr;
	}

	Ref<BoxShape> BoxShape::Create(Entity entity, float totalBodyMass)
	{
		switch (PhysicsAPI::Current())
		{
			case PhysicsAPIType::Jolt: return Ref<JoltBoxShape>::Create(entity, totalBodyMass);
		}

		ZONG_CORE_VERIFY(false);
		return nullptr;
	}

	Ref<SphereShape> SphereShape::Create(Entity entity, float totalBodyMass)
	{
		switch (PhysicsAPI::Current())
		{
			case PhysicsAPIType::Jolt: return Ref<JoltSphereShape>::Create(entity, totalBodyMass);
		}

		ZONG_CORE_VERIFY(false);
		return nullptr;
	}

	Ref<CapsuleShape> CapsuleShape::Create(Entity entity, float totalBodyMass)
	{
		switch (PhysicsAPI::Current())
		{
			case PhysicsAPIType::Jolt: return Ref<JoltCapsuleShape>::Create(entity, totalBodyMass);
		}

		ZONG_CORE_VERIFY(false);
		return nullptr;
	}

	Ref<ConvexMeshShape> ConvexMeshShape::Create(Entity entity, float totalBodyMass)
	{
		switch (PhysicsAPI::Current())
		{
			case PhysicsAPIType::Jolt: return Ref<JoltConvexMeshShape>::Create(entity, totalBodyMass);
		}

		ZONG_CORE_VERIFY(false);
		return nullptr;
	}

	Ref<TriangleMeshShape> TriangleMeshShape::Create(Entity entity)
	{
		switch (PhysicsAPI::Current())
		{
			case PhysicsAPIType::Jolt: return Ref<JoltTriangleMeshShape>::Create(entity);
		}

		ZONG_CORE_VERIFY(false);
		return nullptr;
	}

}

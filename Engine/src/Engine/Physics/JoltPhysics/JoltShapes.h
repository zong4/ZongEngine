#pragma once

#include "Engine/Physics/PhysicsShapes.h"
#include "JoltUtils.h"
#include "JoltMaterial.h"

#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/MutableCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

namespace Hazel {

	class JoltImmutableCompoundShape : public ImmutableCompoundShape
	{
	public:
		JoltImmutableCompoundShape(Entity entity);
		~JoltImmutableCompoundShape()
		{
			Destroy();
		}

		virtual void SetMaterial(const ColliderMaterial& material) override;

		virtual void AddShape(const Ref<PhysicsShape>& shape) override;
		virtual void RemoveShape(const Ref<PhysicsShape>& shape) override;
		virtual void Create() override;

		virtual uint32_t GetNumShapes() const override { return 1; }
		virtual void* GetNativeShape(uint32_t index = 0) const override { return m_Shape.GetPtr(); };
		virtual void Destroy() override {}

	private:
		JPH::Ref<JPH::StaticCompoundShape> m_Shape = nullptr;
		JPH::StaticCompoundShapeSettings m_Settings;

	};

	class JoltMutableCompoundShape : public MutableCompoundShape
	{
	public:
		JoltMutableCompoundShape(Entity entity);
		~JoltMutableCompoundShape()
		{
			Destroy();
		}

		virtual void SetMaterial(const ColliderMaterial& material) override;

		virtual void AddShape(const Ref<PhysicsShape>& shape) override;
		virtual void RemoveShape(const Ref<PhysicsShape>& shape) override;
		virtual void Create() override;

		virtual uint32_t GetNumShapes() const override { return 1; }
		virtual void* GetNativeShape(uint32_t index = 0) const override { return m_Shape.GetPtr(); };
		virtual void Destroy() override {}

	private:
		JPH::Ref<JPH::MutableCompoundShape> m_Shape = nullptr;
		JPH::MutableCompoundShapeSettings m_Settings;

	};

	class JoltBoxShape : public BoxShape
	{
	public:
		JoltBoxShape(Entity entity, float totalBodyMass);
		~JoltBoxShape()
		{
			Destroy();
		}

		virtual void SetMaterial(const ColliderMaterial& material) override;

		virtual uint32_t GetNumShapes() const override { return 1; }
		virtual void* GetNativeShape(uint32_t index = 0) const override { return m_Shape.GetPtr(); }

		virtual glm::vec3 GetHalfSize() const override { return JoltUtils::FromJoltVector(((const JPH::BoxShape*)m_Shape->GetInnerShape())->GetHalfExtent()); }

		virtual void Destroy() override
		{

		}

	private:
		JoltMaterial* m_Material = nullptr;
		JPH::Ref<JPH::BoxShapeSettings> m_Settings;
		JPH::Ref<JPH::RotatedTranslatedShape> m_Shape = nullptr;
	};

	class JoltSphereShape : public SphereShape
	{
	public:
		JoltSphereShape(Entity entity, float totalBodyMass);
		~JoltSphereShape()
		{
			Destroy();
		}

		virtual void SetMaterial(const ColliderMaterial& material) override;

		virtual float GetRadius() const override { return((const JPH::CapsuleShape*)m_Shape->GetInnerShape())->GetRadius(); }

		virtual uint32_t GetNumShapes() const override { return 1; }
		virtual void* GetNativeShape(uint32_t index = 0) const override { return m_Shape.GetPtr(); }

		virtual void Destroy() override {}

	private:
		JoltMaterial* m_Material = nullptr;
		JPH::Ref<JPH::SphereShapeSettings> m_Settings;
		JPH::Ref<JPH::RotatedTranslatedShape> m_Shape = nullptr;
	};

	class JoltCapsuleShape : public CapsuleShape
	{
	public:
		JoltCapsuleShape(Entity entity, float totalBodyMass);
		~JoltCapsuleShape()
		{
			Destroy();
		}

		virtual void SetMaterial(const ColliderMaterial& material) override;

		virtual float GetRadius() const override { return ((const JPH::CapsuleShape*)m_Shape->GetInnerShape())->GetRadius(); }
		virtual float GetHeight() const override { return ((const JPH::CapsuleShape*)m_Shape->GetInnerShape())->GetHalfHeightOfCylinder() * 2.0f; }

		virtual uint32_t GetNumShapes() const override { return 1; }
		virtual void* GetNativeShape(uint32_t index = 0) const override { return m_Shape.GetPtr(); }

		virtual void Destroy() override {}

	private:
		JoltMaterial* m_Material = nullptr;
		JPH::Ref<JPH::CapsuleShapeSettings> m_Settings;
		JPH::Ref<JPH::RotatedTranslatedShape> m_Shape = nullptr;
	};

	class JoltConvexMeshShape : public ConvexMeshShape
	{
	public:
		JoltConvexMeshShape(Entity entity, float totalBodyMass);
		~JoltConvexMeshShape()
		{
			Destroy();
		}

		virtual void SetMaterial(const ColliderMaterial& material) override;

		virtual uint32_t GetNumShapes() const override { return 1; }
		virtual void* GetNativeShape(uint32_t index = 0) const override
		{
			HZ_CORE_VERIFY(index < 1);
			return m_Shape.GetPtr();
		}

		virtual void Destroy() override {}

	private:
		JPH::Ref<JPH::ScaledShape> m_Shape;
	};

	class JoltTriangleMeshShape : public TriangleMeshShape
	{
	public:
		JoltTriangleMeshShape(Entity entity);
		~JoltTriangleMeshShape()
		{
			Destroy();
		}

		virtual void SetMaterial(const ColliderMaterial& material) override;

		virtual uint32_t GetNumShapes() const override { return m_Shape->GetNumSubShapes(); }
		virtual void* GetNativeShape(uint32_t index = 0) const override
		{
			HZ_CORE_VERIFY(index < 1);
			return m_Shape.GetPtr();
		}

		virtual void Destroy() override {}

	private:
		JPH::Ref<JPH::StaticCompoundShape> m_Shape;
	};

}

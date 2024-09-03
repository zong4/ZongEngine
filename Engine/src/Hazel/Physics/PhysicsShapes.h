#pragma once

#include "Hazel/Scene/Entity.h"

namespace Hazel {

	enum class ShapeType { Box, Sphere, Capsule, ConvexMesh, TriangleMesh, CompoundShape, MutableCompoundShape, LAST };

	namespace ShapeUtils {
		constexpr size_t MaxShapeTypes = (size_t)ShapeType::LAST;

		inline const char* ShapeTypeToString(ShapeType type)
		{
			switch (type)
			{
				case ShapeType::CompoundShape: return "CompoundShape";
				case ShapeType::MutableCompoundShape: return "MutableCompoundShape";
				case ShapeType::Box: return "Box";
				case ShapeType::Sphere: return "Sphere";
				case ShapeType::Capsule: return "Capsule";
				case ShapeType::ConvexMesh: return "ConvexMesh";
				case ShapeType::TriangleMesh: return "TriangleMesh";
			}

			HZ_CORE_VERIFY(false);
			return "";
		}

	}

	class PhysicsShape : public RefCounted
	{
	public:
		virtual ~PhysicsShape() = default;

		virtual uint32_t GetNumShapes() const = 0;
		virtual void* GetNativeShape(uint32_t index = 0) const = 0;

		virtual void SetMaterial(const ColliderMaterial& material) = 0;

		virtual void Destroy() = 0;

		Entity GetEntity() const { return m_Entity; }
		ShapeType GetType() const { return m_Type; }

	protected:
		PhysicsShape(ShapeType type, Entity entity)
			: m_Type(type), m_Entity(entity) {}

	protected:
		Entity m_Entity;

	private:
		ShapeType m_Type;
	};

	class CompoundShape : public PhysicsShape
	{
	public:
		CompoundShape(Entity entity, const bool isImmutable)
			: PhysicsShape(isImmutable ? ShapeType::CompoundShape : ShapeType::MutableCompoundShape, entity)
		{
		}
		virtual ~CompoundShape() = default;

		virtual void AddShape(const Ref<PhysicsShape>& shape) = 0;
		virtual void RemoveShape(const Ref<PhysicsShape>& shape) = 0;
		virtual void Create() = 0;
	};

	class ImmutableCompoundShape : public CompoundShape
	{
	public:
		ImmutableCompoundShape(Entity entity)
			: CompoundShape(entity, true) {}

		virtual ~ImmutableCompoundShape() = default;

	public:
		static Ref<ImmutableCompoundShape> Create(Entity entity);
	};

	class MutableCompoundShape : public CompoundShape
	{
	public:
		MutableCompoundShape(Entity entity)
			: CompoundShape(entity, false)
		{
		}

		virtual ~MutableCompoundShape() = default;

	public:
		static Ref<MutableCompoundShape> Create(Entity entity);
	};

	class BoxShape : public PhysicsShape
	{
	public:
		BoxShape(Entity entity)
			: PhysicsShape(ShapeType::Box, entity) {}
		virtual ~BoxShape() = default;

		virtual glm::vec3 GetHalfSize() const = 0;

	public:
		static Ref<BoxShape> Create(Entity entity, float totalBodyMass);
	};

	class SphereShape : public PhysicsShape
	{
	public:
		SphereShape(Entity entity)
			: PhysicsShape(ShapeType::Sphere, entity) {}

		virtual ~SphereShape() = default;

		virtual float GetRadius() const = 0;

	public:
		static Ref<SphereShape> Create(Entity entity, float totalBodyMass);
	};

	class CapsuleShape : public PhysicsShape
	{
	public:
		CapsuleShape(Entity entity)
			: PhysicsShape(ShapeType::Capsule, entity)
		{
		}

		virtual ~CapsuleShape() = default;

		virtual float GetRadius() const = 0;

		virtual float GetHeight() const = 0;

	public:
		static Ref<CapsuleShape> Create(Entity entity, float totalBodyMass);
	};

	class ConvexMeshShape : public PhysicsShape
	{
	public:
		ConvexMeshShape(Entity entity)
			: PhysicsShape(ShapeType::ConvexMesh, entity) {}

		virtual ~ConvexMeshShape() = default;

	public:
		static Ref<ConvexMeshShape> Create(Entity entity, float totalBodyMass);
	};

	class TriangleMeshShape : public PhysicsShape
	{
	public:
		TriangleMeshShape(Entity entity)
			: PhysicsShape(ShapeType::TriangleMesh, entity)
		{
		}

		virtual ~TriangleMeshShape() = default;

	public:
		static Ref<TriangleMeshShape> Create(Entity entity);
	};

}

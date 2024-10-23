using System;
using Hazel;

namespace Hazel {

	public enum EShapeType { Box, Sphere, Capsule, ConvexMesh, TriangleMesh, CompoundShape, MutableCompoundShape }

	public class Shape
	{
		protected Shape(EShapeType shapeType)
		{
			ShapeType = shapeType;
		}

		public EShapeType ShapeType { get; private set; }
	}

	public class BoxShape : Shape
	{
		public BoxShape(Vector3 halfExtent)
			: base(EShapeType.Box)
		{
			HalfExtent = halfExtent;
		}

		public Vector3 HalfExtent { get; private set; }
	}

	public class SphereShape : Shape
	{
		public SphereShape(float radius)
			: base(EShapeType.Sphere)
		{
			Radius = radius;
		}

		public float Radius { get; private set; }
	}

	public class CapsuleShape : Shape
	{
		public CapsuleShape(float halfHeight, float radius)
			: base(EShapeType.Capsule)
		{
			HalfHeight = halfHeight;
			Radius = radius;
		}

		public float HalfHeight { get; private set; }
		public float Radius { get; private set; }
	}

	public class ConvexMeshShape : Shape
	{
		public ConvexMeshShape(MeshBase shapeMesh)
			: base(EShapeType.ConvexMesh)
		{
			ShapeMesh = shapeMesh;
		}

		public MeshBase ShapeMesh { get; private set; }
	}

	public class TriangleMeshShape : Shape
	{
		public TriangleMeshShape(MeshBase shapeMesh)
			: base(EShapeType.TriangleMesh)
		{
			ShapeMesh = shapeMesh;
		}

		public MeshBase ShapeMesh { get; private set; }
	}

}

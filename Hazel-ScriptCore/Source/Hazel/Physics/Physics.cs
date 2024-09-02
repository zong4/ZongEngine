using System;
using System.Runtime.InteropServices;

using Hazel.Interop;
using Coral.Managed.Interop;

namespace Hazel
{
	[StructLayout(LayoutKind.Sequential)]
	public struct SceneQueryHit
	{
		public readonly ulong EntityID;
		public readonly Vector3 Position;
		public readonly Vector3 Normal;
		public readonly float Distance;
		private readonly float Padding; // NOTE(Peter): Manual padding because C# doesn't fully respect the 8-byte alignment here
		public readonly NativeInstance<Collider> HitCollider;

		public SceneQueryHit()
		{
			HitCollider = new();
		}

		public Entity Entity => Scene.FindEntityByID(EntityID);
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct RaycastData
	{
		public Vector3 Origin;
		public Vector3 Direction;
		public float MaxDistance;
		public float Padding0;
		public NativeTypeArray RequiredComponents;
		public NativeArray<ulong> ExcludedEntities;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct ShapeQueryData
	{
		public Vector3 Origin;
		public Vector3 Direction;
		public float MaxDistance;
		public float Padding0;
		public NativeInstance<Shape> ShapeData;
		public NativeTypeArray RequiredComponents;
		public NativeArray<ulong> ExcludedEntities;
	}

	public enum EActorAxis : uint
	{
		TranslationX	= 1 << 0,
		TranslationY	= 1 << 1,
		TranslationZ	= 1 << 2,
		TranslationXYZ = TranslationX | TranslationY | TranslationZ,

		RotationX		= 1 << 3,
		RotationY		= 1 << 4,
		RotationZ		= 1 << 5,
		RotationXYZ = RotationX | RotationY | RotationZ
	}

	public enum EFalloffMode { Constant, Linear }

	public static class Physics
	{
		public static Vector3 Gravity
		{
			get
			{
				Vector3 gravity;
				unsafe { InternalCalls.Physics_GetGravity(&gravity); }
				return gravity;
			}

			set
			{
				unsafe { InternalCalls.Physics_SetGravity(&value); }
			}
		}

		/// <summary>
		/// Adds a radial impulse to the scene. Any entity within the radius of the origin will be pushed/pulled according to the strength.
		/// You'd use this method for something like an explosion.
		/// </summary>
		/// <param name="origin">The origin of the impulse in world space</param>
		/// <param name="radius">The radius of the area affected by the impulse (1 unit = 1 meter radius)</param>
		/// <param name="strength">The strength of the impulse</param>
		/// <param name="falloff">The falloff method used when calculating force over distance</param>
		/// <param name="velocityChange">Setting this value to <b>true</b> will make this impulse ignore an actors mass</param>
		public static void AddRadialImpulse(Vector3 origin, float radius, float strength, EFalloffMode falloff = EFalloffMode.Constant, bool velocityChange = false)
		{
			unsafe { InternalCalls.Physics_AddRadialImpulse(&origin, radius, strength, falloff, velocityChange); }
		}

		public static bool CastRay(RaycastData raycastData, out SceneQueryHit hit)
		{
			unsafe
			{
				fixed (SceneQueryHit* hitPtr = &hit)
				{
					return InternalCalls.Physics_CastRay(&raycastData, hitPtr);
				}
			}
		}

		public static bool CastShape(ShapeQueryData shapeCastData, out SceneQueryHit outHit)
		{
			unsafe
			{
				fixed (SceneQueryHit* hitPtr = &outHit)
				{
					return InternalCalls.Physics_CastShape(&shapeCastData, hitPtr);
				}
			}
		}

		/// <summary>
		/// Performs an overlap scene query, returning an array of any colliders that the provided shape overlaps.
		/// Only allocates when necessary, you should preferably provide a pre-allocated array to the <paramref name="outHits"/> parameter.
		/// </summary>
		/// <param name="shapeQueryData"></param>
		/// <param name="outHits"></param>
		/// <returns></returns>
		public static int OverlapShapeNonAlloc(ShapeQueryData shapeQueryData, ref SceneQueryHit[] outHits)
		{
			unsafe
			{
				var mappedArray = NativeArray<SceneQueryHit>.Map(outHits);
				int length = InternalCalls.Physics_OverlapShape(&shapeQueryData, &mappedArray);

				if (length > outHits.Length)
				{
					// Array was resized
					outHits = mappedArray;
				}

				NativeArray<SceneQueryHit>.Unmap(ref mappedArray);
				return length;
			}
		}

		/// <summary>
		/// Performs an overlap scene query, returning an array of any colliders that the provided shape overlaps.
		/// Always allocates.
		/// </summary>
		/// <param name="shapeQueryData"></param>
		/// <param name="outHits"></param>
		/// <returns></returns>
		public static int OverlapShape(ShapeQueryData shapeQueryData, out SceneQueryHit[] outHits)
		{
			unsafe
			{
				NativeArray<SceneQueryHit> hits;
				int length = InternalCalls.Physics_OverlapShape(&shapeQueryData, &hits);
				outHits = hits;
				hits.Dispose();
				return length;
			}
		}
	}
}

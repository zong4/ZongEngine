using System;
using System.Runtime.InteropServices;

namespace Hazel
{
	[StructLayout(LayoutKind.Sequential)]
	public struct SceneQueryHit
	{
		public ulong EntityID { get; internal set; }
		public Vector3 Position { get; internal set; }
		public Vector3 Normal { get; internal set; }
		public float Distance { get; internal set; }
		public Collider HitCollider { get; internal set; }

		public Entity Entity => Scene.FindEntityByID(EntityID);
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct RaycastData
	{
		public Vector3 Origin;
		public Vector3 Direction;
		public float MaxDistance;
		public Type[] RequiredComponents;
		public ulong[] ExcludedEntities;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct ShapeQueryData
	{
		public Vector3 Origin;
		public Vector3 Direction;
		public float MaxDistance;
		public Shape ShapeData;
		public Type[] RequiredComponents;
		public ulong[] ExcludedEntities;
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
				InternalCalls.Physics_GetGravity(out Vector3 gravity);
				return gravity;
			}

			set => InternalCalls.Physics_SetGravity(ref value);
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
			=> InternalCalls.Physics_AddRadialImpulse(ref origin, radius, strength, falloff, velocityChange);

		public static bool CastRay(RaycastData raycastData, out SceneQueryHit hit) => InternalCalls.Physics_CastRay(ref raycastData, out hit);
		public static bool CastShape(ShapeQueryData shapeCastData, out SceneQueryHit outHit) => InternalCalls.Physics_CastShape(ref shapeCastData, out outHit);

		/// <summary>
		/// Performs an overlap scene query, returning an array of any colliders that the provided shape overlaps.
		/// Only allocates when necessary, you should preferably provide a pre-allocated array to the <paramref name="outHits"/> parameter.
		/// </summary>
		/// <param name="shapeQueryData"></param>
		/// <param name="outHits"></param>
		/// <returns></returns>
		public static int OverlapShape(ShapeQueryData shapeQueryData, out SceneQueryHit[] outHits) => InternalCalls.Physics_OverlapShape(ref shapeQueryData, out outHits);
	}
}

using System;
using System.Runtime.InteropServices;

using Coral.Managed.Interop;

namespace Hazel
{
	[StructLayout(LayoutKind.Sequential)]
	public struct RaycastData2D
	{
		public Vector2 Origin;
		public Vector2 Direction;
		public float MaxDistance;
		private readonly float Padding; // NOTE(Peter): Manual padding because C# doesn't fully respect the 8-byte alignment here
		public NativeArray<ReflectionType> RequiredComponents;
	}

	public struct RaycastHit2D
	{
		public readonly ulong EntityID;
		public readonly Vector2 Position;
		public readonly Vector2 Normal;
		public readonly float Distance;

		public Entity? Entity => Scene.FindEntityByID(EntityID);
	}

	public static class Physics2D
	{
		public static RaycastHit2D[] Raycast2D(RaycastData2D raycastData)
		{
			unsafe { return InternalCalls.Physics_Raycast2D(&raycastData); }
		}

		public static RaycastHit2D[] Raycast2D(Vector2 origin, Vector2 direction, float maxDistance, params ReflectionType[] componentFilters)
		{
			Console.WriteLine($"Origin: {Marshal.OffsetOf<RaycastData2D>("Origin")}");
			Console.WriteLine($"Direction: {Marshal.OffsetOf<RaycastData2D>("Direction")}");
			Console.WriteLine($"MaxDistance: {Marshal.OffsetOf<RaycastData2D>("MaxDistance")}");
			Console.WriteLine($"RequiredComponents: {Marshal.OffsetOf<RaycastData2D>("RequiredComponents")}");

			s_RaycastData2D.Origin = origin;
			s_RaycastData2D.Direction = direction;
			s_RaycastData2D.MaxDistance = maxDistance;
			s_RaycastData2D.RequiredComponents = componentFilters;

			unsafe
			{
				fixed (RaycastData2D* data = &s_RaycastData2D)
				{
					using var result = InternalCalls.Physics_Raycast2D(data);
					return result;
				}
			}
		}

		private static RaycastData2D s_RaycastData2D;
	}
}

using System;
using System.Runtime.InteropServices;

namespace Engine
{
	[StructLayout(LayoutKind.Sequential)]
	public struct RaycastData2D
	{
		public Vector2 Origin;
		public Vector2 Direction;
		public float MaxDistance;
		public Type[] RequiredComponents;
	}

	// NOTE(Peter): This can be converted to a struct (would make it less memory intensive)
	public class RaycastHit2D
	{
		public Entity Entity { get; internal set; }
		public Vector2 Position { get; internal set; }
		public Vector2 Normal { get; internal set; }
		public float Distance { get; internal set; }

		internal RaycastHit2D()
		{
			Entity = null;
			Position = Vector2.Zero;
			Normal = Vector2.Zero;
			Distance = 0.0f;
		}

		internal RaycastHit2D(Entity entity, Vector2 position, Vector2 normal, float distance)
		{
			Entity = entity;
			Position = position;
			Normal = normal;
			Distance = distance;
		}
	}

	public static class Physics2D
	{
		public static RaycastHit2D[] Raycast2D(RaycastData2D raycastData) => InternalCalls.Physics_Raycast2D(ref raycastData);
		public static RaycastHit2D[] Raycast2D(Vector2 origin, Vector2 direction, float maxDistance, params Type[] componentFilters)
		{
			s_RaycastData2D.Origin = origin;
			s_RaycastData2D.Direction = direction;
			s_RaycastData2D.MaxDistance = maxDistance;
			s_RaycastData2D.RequiredComponents = componentFilters;
			return InternalCalls.Physics_Raycast2D(ref s_RaycastData2D);
		}

		private static RaycastData2D s_RaycastData2D;
	}
}

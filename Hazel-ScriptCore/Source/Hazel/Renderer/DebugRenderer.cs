
namespace Hazel
{
	public static class DebugRenderer
	{

		public static void DrawLine(Vector3 p0, Vector3 p1, Vector4 color)
		{
			InternalCalls.DebugRenderer_DrawLine(ref p0, ref p1, ref color);
		}

		public static void DrawQuadBillboard(Vector3 translation, Vector2 size, Vector4 color)
		{
			InternalCalls.DebugRenderer_DrawQuadBillboard(ref translation, ref size, ref color);
		}

		public static float LineWidth { set => InternalCalls.DebugRenderer_SetLineWidth(value); }

	}
}

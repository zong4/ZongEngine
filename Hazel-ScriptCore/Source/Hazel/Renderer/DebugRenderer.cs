
namespace Hazel
{
	public static class DebugRenderer
	{

		public static void DrawLine(Vector3 p0, Vector3 p1, Vector4 color)
		{
			unsafe { InternalCalls.DebugRenderer_DrawLine(&p0, &p1, &color); }
		}

		public static void DrawQuadBillboard(Vector3 translation, Vector2 size, Vector4 color)
		{
			unsafe { InternalCalls.DebugRenderer_DrawQuadBillboard(&translation, &size, &color); }
		}

		public static float LineWidth
		{
			set { unsafe { InternalCalls.DebugRenderer_SetLineWidth(value); } }
		}

	}
}

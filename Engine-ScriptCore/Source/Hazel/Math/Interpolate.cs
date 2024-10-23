namespace Hazel
{
	public static class Interpolate
	{

		public static float Linear(float p1, float p2, float t)
		{
			if (t < 0.0f)
				return p1;
			else if (t > 1.0f)
				return p2;

			return p1 + ((p2 - p1) * t);
		}

		public static Vector2 Linear(Vector2 p1, Vector2 p2, float t)
		{
			if (t < 0.0f)
				return p1;
			else if (t > 1.0f)
				return p2;

			return p1 + ((p2 - p1) * t);
		}

		public static Vector3 Linear(Vector3 p1, Vector3 p2, float t)
		{
			if (t < 0.0f)
				return p1;
			else if (t > 1.0f)
				return p2;

			return p1 + ((p2 - p1) * t);
		}

		public static Vector4 Linear(Vector4 p1, Vector4 p2, float t)
		{
			if (t < 0.0f)
				return p1;
			else if (t > 1.0f)
				return p2;

			return p1 + ((p2 - p1) * t);
		}

		public static float EaseIn(float a, float b, float t)
		{
			return Linear(a, b, EaseIn_Time(t));
		}

		public static float EaseOut(float a, float b, float t)
		{
			return Linear(a, b, EaseOut_Time(t));
		}

		public static float EaseInOut(float a, float b, float t)
		{
			return Linear(a, b, EaseInOut_Time(t));
		}

		public static Vector2 EaseIn(Vector2 p1, Vector2 p2, float t)
		{
			return Linear(p1, p2, EaseIn_Time(t));
		}

		public static Vector2 EaseOut(Vector2 p1, Vector2 p2, float t)
		{
			return Linear(p1, p2, EaseOut_Time(t));
		}

		public static Vector2 EaseInOut(Vector2 p1, Vector2 p2, float t)
		{
			return Linear(p1, p2, EaseInOut_Time(t));
		}

		public static Vector3 EaseIn(Vector3 p1, Vector3 p2, float t)
		{
			return Linear(p1, p2, EaseIn_Time(t));
		}

		public static Vector3 EaseOut(Vector3 p1, Vector3 p2, float t)
		{
			return Linear(p1, p2, EaseOut_Time(t));
		}

		public static Vector3 EaseInOut(Vector3 p1, Vector3 p2, float t)
		{
			return Linear(p1, p2, EaseInOut_Time(t));
		}

		public static Vector4 EaseIn(Vector4 p1, Vector4 p2, float t)
		{
			return Linear(p1, p2, EaseIn_Time(t));
		}

		public static Vector4 EaseOut(Vector4 p1, Vector4 p2, float t)
		{
			return Linear(p1, p2, EaseOut_Time(t));
		}

		public static Vector4 EaseInOut(Vector4 p1, Vector4 p2, float t)
		{
			return Linear(p1, p2, EaseInOut_Time(t));
		}

		private static float EaseIn_Time(float t)
		{
			return Square(t);
		}

		private static float EaseOut_Time(float t)
		{
			return Invert(Square(Invert(t)));
		}

		private static float EaseInOut_Time(float t)
		{
			return Linear(EaseIn_Time(t), EaseOut_Time(t), t);
		}

		internal static float Invert(float x)
		{
			return 1.0f - x;
		}

		internal static float Square(float x)
		{
			return x * x;
		}

	}
}

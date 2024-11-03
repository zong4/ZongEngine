using System;
using System.Runtime.InteropServices;

namespace Engine
{
	public static class Mathf
	{

		public const float Epsilon = 0.00001f;
		public const float PI = (float)Math.PI;
		public const float PIonTwo = (float)(Math.PI / 2.0f);
		public const float TwoPI = (float)(Math.PI * 2.0);

		public const float Deg2Rad = PI / 180.0f;
		public const float Rad2Deg = 180.0f / PI;

		public static float Sin(float value) => (float)Math.Sin(value);
		public static float Cos(float value) => (float)Math.Cos(value);
		public static float Tan(float value) => (float)Math.Tan(value);

		public static float Clamp(float value, float min, float max)
		{
			if (value < min)
				return min;
			return value > max ? max : value;
		}

		public static int Clamp(int value, int min, int max)
		{
			if (value < min)
				return min;
			return value > max ? max : value;
		}

		public static float Asin(float value) => (float)Math.Asin(value);
		public static float Acos(float value) => (float)Math.Acos(value);
		public static float Atan(float value) => (float)Math.Atan(value);
		public static float Atan2(float v0, float v1) => (float)Math.Atan2(v0, v1);

		public static float Sinh(float value) => (float)Math.Sinh(value);
		public static float Cosh(float value) => (float)Math.Cosh(value);
		public static float Tanh(float value) => (float)Math.Tanh(value);

		public static float Sign(float value) { return (float)Math.Sign(value); }

		public static float Log(float value) => (float)Math.Log(value);
		public static float Log10(float value) => (float)Math.Log10(value);
		public static float Log10(float value, float newBase) => (float)Math.Log(value, newBase);

		public static float Exp(float value) => (float)Math.Exp(value);
		public static float Pow(float v0, float v1) => (float)Math.Pow(v0, v1);

		public static float Min(float v0, float v1) => v0 < v1 ? v0 : v1;
		public static float Max(float v0, float v1) => v0 > v1 ? v0 : v1;

		public static float Sqrt(float value) => (float)Math.Sqrt(value);

		public static float Abs(float value) => Math.Abs(value);
		public static int Abs(int value) => Math.Abs(value);

		public static bool EqualEpsilon(float v0, float v1) { return v0 >= v1 - float.Epsilon && v0 <= v1 + float.Epsilon; }

		public static Vector3 Abs(Vector3 value)
		{
			return new Vector3(Math.Abs(value.X), Math.Abs(value.Y), Math.Abs(value.Z));
		}

		public static float Lerp(float p1, float p2, float t) => Interpolate.Linear(p1, p2, t);
		public static Vector3 Lerp(Vector3 p1, Vector3 p2, float t) => Interpolate.Linear(p1, p2, t);

		public static float Floor(float value) => (float)Math.Floor(value);

		// not the same as a%b
		public static float Modulo(float v0, float v1) => v0 - v1 * (float)Math.Floor(v0 / v1);

		public static float Distance(float p1, float p2) => Abs(p1 - p2);

		public static float PerlinNoise(float v0, float v1) => Noise.Perlin(v0, v1);

		public static int CeilToInt(float value) => (int)Math.Ceiling(value);
		public static int FloorToInt(float value) => (int)Math.Floor(value);
		
		public static float Round(float value) => (float)Math.Round(value);

		public static int DivRem(int v0, int v1)
		{
			Math.DivRem(v0, v1, out int result);
			return result;
		}

	}
}

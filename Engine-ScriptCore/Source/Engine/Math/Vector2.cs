using System;
using System.Runtime.InteropServices;

namespace Engine
{
	[StructLayout(LayoutKind.Sequential)]
	public struct Vector2
	{
		public static Vector2 Zero = new Vector2(0, 0);
		public static Vector2 One = new Vector2(1, 1);
		public static Vector2 Right = new Vector2(1, 0);
		public static Vector2 Left = new Vector2(-1, 0);
		public static Vector2 Up = new Vector2(0, 1);
		public static Vector2 Down = new Vector2(0, -1);

		public float X;
		public float Y;

		public Vector2(float scalar) => X = Y = scalar;

		public Vector2(float x, float y)
		{
			X = x;
			Y = y;
		}

		public Vector2(Vector3 vector)
		{
			X = vector.X;
			Y = vector.Y;
		}

		public void Clamp(Vector2 min, Vector2 max)
		{
			X = Mathf.Clamp(X, min.X, max.X);
			Y = Mathf.Clamp(Y, min.Y, max.Y);
		}

		public float Length() => (float)Math.Sqrt(X * X + Y * Y);

		public Vector2 Normalized()
		{
			float length = Length();
			return length > 0.0f ? New(v => v / length) : new Vector2(X, Y);
		}

		public void Normalize()
		{
			float length = Length();
			if (length == 0.0f)
				return;

			Apply(v => v / length);
		}

		public float Distance(Vector2 other)
		{
			return (float)Math.Sqrt(Math.Pow(other.X - X, 2) +
									Math.Pow(other.Y - Y, 2));
		}

		public static float Distance(Vector2 p1, Vector2 p2)
		{
			return (float)Math.Sqrt(Math.Pow(p2.X - p1.X, 2) +
									Math.Pow(p2.Y - p1.Y, 2));
		}

		//Lerps from p1 to p2
		public static Vector2 Lerp(Vector2 p1, Vector2 p2, float maxDistanceDelta)
		{
			if (maxDistanceDelta < 0.0f)
				return p1;
			if (maxDistanceDelta > 1.0f)
				return p2;

			return p1 + ((p2 - p1) * maxDistanceDelta);
		}

		public static float Dot(Vector2 lhs, Vector2 rhs) { return lhs.X * rhs.X + lhs.Y * rhs.Y; }

		public static bool EpsilonEquals(Vector2 p1, Vector2 p2, float epsilon = float.Epsilon)
		{
			return Mathf.Abs(p1.X - p2.X) < epsilon && Mathf.Abs(p1.Y - p2.Y) < epsilon;
		}

		/// <summary>
		/// Allows you to pass in a delegate that takes in
		/// a double to process the vector per axis.
		/// i.e. (Math.Cos) or a lambda (v => v * 3)
		/// </summary>
		/// <param name="func">Delegate 'double' method to act as a scalar to process X and Y</param>
		public void Apply(Func<double, double> func)
		{
			X = (float)func(X);
			Y = (float)func(Y);
		}
		
		/// <summary>
		/// Allows you to pass in a delegate that takes in and returns
		/// a new Vector processed per axis.
		/// i.e. (Math.Cos) or a lambda (v => v * 3)
		/// </summary>
		/// <param name="func">Delegate 'double' method to act as a scalar to process X and Y</param>
		public Vector2 New(Func<double, double> func) => new Vector2((float)func(X), (float)func(Y));

		public static Vector2 operator *(Vector2 left, float scalar) => new Vector2(left.X * scalar, left.Y * scalar);
		public static Vector2 operator *(float scalar, Vector2 right) => new Vector2(scalar * right.X, scalar * right.Y);
		public static Vector2 operator *(Vector2 left, Vector2 right) => new Vector2(left.X * right.X, left.Y * right.Y);
		public static Vector2 operator /(Vector2 left, Vector2 right) => new Vector2(left.X / right.X, left.Y / right.Y);
		public static Vector2 operator /(Vector2 left, float scalar) => new Vector2(left.X / scalar, left.Y / scalar);
		public static Vector2 operator /(float scalar, Vector2 right) => new Vector2(scalar/ right.X, scalar/ right.Y);
		public static Vector2 operator +(Vector2 left, Vector2 right) => new Vector2(left.X + right.X, left.Y + right.Y);
		public static Vector2 operator +(Vector2 left, float right) => new Vector2(left.X + right, left.Y + right);
		public static Vector2 operator -(Vector2 left, Vector2 right) => new Vector2(left.X - right.X, left.Y - right.Y);
		public static Vector2 operator -(Vector2 left, float right) => new Vector2(left.X - right, left.Y - right);
		public static Vector2 operator -(Vector2 vector) => new Vector2(-vector.X, -vector.Y);
		public override bool Equals(object obj) => obj is Vector2 other && Equals(other);
		public bool Equals(Vector2 right) => X == right.X && Y == right.Y;
		public override int GetHashCode() => (X, Y).GetHashCode();
		public static bool operator ==(Vector2 left, Vector2 right) => left.Equals(right);
		public static bool operator !=(Vector2 left, Vector2 right) => !(left == right);
		
		public override string ToString() => "Vector2[" + X + ", " + Y + "]";
	}
}

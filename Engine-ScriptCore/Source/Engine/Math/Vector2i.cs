using System;
using System.Runtime.InteropServices;

namespace Hazel
{
	[StructLayout(LayoutKind.Sequential)]
	public struct Vector2i
	{
		public static Vector2i Zero = new Vector2i(0, 0);
		public static Vector2i One = new Vector2i(1, 1);
		public static Vector2i Right = new Vector2i(1, 0);
		public static Vector2i Left = new Vector2i(-1, 0);
		public static Vector2i Up = new Vector2i(0, 1);
		public static Vector2i Down = new Vector2i(0, -1);

		public int X;
		public int Y;

		public Vector2i(int scalar) => X = Y = scalar;

		public Vector2i(int x, int y)
		{
			X = x;
			Y = y;
		}

		public int Distance(Vector2i other)
		{
			return (int)Math.Sqrt(Math.Pow(other.X - X, 2) +
									Math.Pow(other.Y - Y, 2));
		}

		public static int Distance(Vector2i p1, Vector2i p2)
		{
			return (int)Math.Sqrt(Math.Pow(p2.X - p1.X, 2) +
									Math.Pow(p2.Y - p1.Y, 2));
		}

		//Lerps from p1 to p2
		public static Vector2i Lerp(Vector2i p1, Vector2i p2, int maxDistanceDelta)
		{
			if (maxDistanceDelta < 0.0f)
				return p1;
			if (maxDistanceDelta > 1.0f)
				return p2;

			return p1 + ((p2 - p1) * maxDistanceDelta);
		}

		/// <summary>
		/// Allows you to pass in a delegate that takes in
		/// a double to process the vector per axis.
		/// i.e. (Math.Cos) or a lambda (v => v * 3)
		/// </summary>
		/// <param name="func">Delegate 'double' method to act as a scalar to process X and Y</param>
		public void Apply(Func<double, double> func)
		{
			X = (int)func(X);
			Y = (int)func(Y);
		}

		/// <summary>
		/// Allows you to pass in a delegate that takes in and returns
		/// a new Vector processed per axis.
		/// i.e. (Math.Cos) or a lambda (v => v * 3)
		/// </summary>
		/// <param name="func">Delegate 'double' method to act as a scalar to process X and Y</param>
		public Vector2i New(Func<double, double> func) => new Vector2i((int)func(X), (int)func(Y));

		public static Vector2i operator *(Vector2i left, int scalar) => new Vector2i(left.X * scalar, left.Y * scalar);
		public static Vector2i operator *(int scalar, Vector2i right) => new Vector2i(scalar * right.X, scalar * right.Y);
		public static Vector2i operator *(Vector2i left, Vector2i right) => new Vector2i(left.X * right.X, left.Y * right.Y);
		public static Vector2i operator /(Vector2i left, Vector2i right) => new Vector2i(left.X / right.X, left.Y / right.Y);
		public static Vector2i operator /(Vector2i left, int scalar) => new Vector2i(left.X / scalar, left.Y / scalar);
		public static Vector2i operator /(int scalar, Vector2i right) => new Vector2i(scalar / right.X, scalar / right.Y);
		public static Vector2i operator +(Vector2i left, Vector2i right) => new Vector2i(left.X + right.X, left.Y + right.Y);
		public static Vector2i operator +(Vector2i left, int right) => new Vector2i(left.X + right, left.Y + right);
		public static Vector2i operator -(Vector2i left, Vector2i right) => new Vector2i(left.X - right.X, left.Y - right.Y);
		public static Vector2i operator -(Vector2i left, int right) => new Vector2i(left.X - right, left.Y - right);
		public static Vector2i operator -(Vector2i vector) => new Vector2i(-vector.X, -vector.Y);
		public override bool Equals(object obj) => obj is Vector2i other && Equals(other);
		public bool Equals(Vector2i right) => X == right.X && Y == right.Y;
		public override int GetHashCode() => (X, Y).GetHashCode();
		public static bool operator ==(Vector2i left, Vector2i right) => left.Equals(right);
		public static bool operator !=(Vector2i left, Vector2i right) => !(left == right);

		public override string ToString() => "Vector2i[" + X + ", " + Y + "]";
	}
}

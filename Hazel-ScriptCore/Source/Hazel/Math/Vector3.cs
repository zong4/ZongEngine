using System;
using System.Runtime.InteropServices;

namespace Hazel
{
	[StructLayout(LayoutKind.Sequential, Pack = 4)]
	public struct Vector3 : IEquatable<Vector3>
	{
		public static Vector3 Zero = new Vector3(0, 0, 0);
		public static Vector3 One = new Vector3(1, 1, 1);
		public static Vector3 Forward = new Vector3(0, 0, -1);
		public static Vector3 Back = new Vector3(0, 0, 1);
		public static Vector3 Right = new Vector3(1, 0, 0);
		public static Vector3 Left = new Vector3(-1, 0, 0);
		public static Vector3 Up = new Vector3(0, 1, 0);
		public static Vector3 Down = new Vector3(0, -1, 0);

		public static Vector3 Inifinity = new Vector3(float.PositiveInfinity);

		public float X;
		public float Y;
		public float Z;

		public Vector3(float scalar) => X = Y = Z = scalar;

		public Vector3(float x, float y, float z)
		{
			X = x;
			Y = y;
			Z = z;
		}

		public Vector3(float x, Vector2 yz)
		{
			X = x;
			Y = yz.X;
			Z = yz.Y;
		}

		public Vector3(Vector2 xy, float z)
		{
			X = xy.X;
			Y = xy.Y;
			Z = z;
		}

		public Vector3(Vector2 vector)
		{
			X = vector.X;
			Y = vector.Y;
			Z = 0.0f;
		}

		public Vector3(Vector4 vector)
		{
			X = vector.X;
			Y = vector.Y;
			Z = vector.Z;
		}

		public void Clamp(Vector3 min, Vector3 max)
		{
			X = Mathf.Clamp(X, min.X, max.X);
			Y = Mathf.Clamp(Y, min.Y, max.Y);
			Z = Mathf.Clamp(Z, min.Z, max.Z);
		}

		public float Length() => (float)Math.Sqrt(X * X + Y * Y + Z * Z);

		public Vector3 Normalized()
		{
			float length = Length();
			return length > 0.0f ? New(v => v / length) : new Vector3(X, Y, Z);
		}

		public void Normalize()
		{
			float length = Length();
			if (length > 0.0f)
				Apply(v => v / length);
		}

		public static Vector3 DirectionFromEuler(Vector3 rotation)
		{
			float pitch = rotation.X;
			float yaw = rotation.Y;
			float roll = rotation.Z;

			return new Vector3(Mathf.Cos(yaw) * Mathf.Cos(pitch),
				Mathf.Sin(yaw) * Mathf.Cos(pitch),
				Mathf.Sin(pitch));
		}

		public static bool EpsilonEquals(Vector3 left, Vector3 right, float epsilon = float.Epsilon)
		{
			bool xEquals = Mathf.Abs(left.X - right.X) < epsilon;
			bool yEquals = Mathf.Abs(left.Y - right.Y) < epsilon;
			bool zEquals = Mathf.Abs(left.Z - right.Z) < epsilon;
			return xEquals && yEquals && zEquals;
		}

		public bool EqualEpsilon(Vector3 other)
		{
			bool xEqual = X >= other.X - float.Epsilon && X <= other.X + float.Epsilon;
			bool yEqual = Y >= other.Y - float.Epsilon && Y <= other.Y + float.Epsilon;
			bool zEqual = Z >= other.Z - float.Epsilon && Z <= other.Z + float.Epsilon;

			return xEqual && yEqual && zEqual;
		}

		public float Distance(Vector3 other)
		{
			return (float)Math.Sqrt(Math.Pow(other.X - X, 2) +
									Math.Pow(other.Y - Y, 2) +
									Math.Pow(other.Z - Z, 2));
		}

		public static float Distance(Vector3 p1, Vector3 p2)
		{
			return (float)Math.Sqrt(Math.Pow(p2.X - p1.X, 2) +
									Math.Pow(p2.Y - p1.Y, 2) +
									Math.Pow(p2.Z - p1.Z, 2));
		}

		//Lerps from p1 to p2
		public static Vector3 Lerp(Vector3 p1, Vector3 p2, float maxDistanceDelta)
		{
			if (maxDistanceDelta < 0.0f)
				return p1;

			if (maxDistanceDelta > 1.0f)
				return p2;

			return p1 + ((p2 - p1) * maxDistanceDelta);
		}

		public const float kEpsilonNormalSqrt = 1e-15F;

		public float sqrMagnitude {get { return X * X + Y * Y + Z * Z; } }

		public static float Dot(Vector3 lhs, Vector3 rhs) { return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z; }

		public static float Angle(Vector3 from, Vector3 to)
		{
			// sqrt(a) * sqrt(b) = sqrt(a * b) -- valid for real numbers
			float denominator = (float)Math.Sqrt(from.sqrMagnitude * to.sqrMagnitude);
			if (denominator < kEpsilonNormalSqrt)
				return 0F;

			float dot = Mathf.Clamp(Dot(from, to) / denominator, -1F, 1F);
			return ((float)Math.Acos(dot)) * Mathf.Rad2Deg;
		}
		
		public static float SignedAngle(Vector3 from, Vector3 to, Vector3 axis)
		{
			float unsignedAngle = Angle(from, to);
			var cross = Cross(from, to);
			float sign = Mathf.Sign(axis.X * cross.X + axis.Y * cross.Y + axis.Z * cross.Z);
			return unsignedAngle * sign;
		}

		/// <summary>
		/// Allows you to pass in a delegate that takes in
		/// a double to process the vector per axis.
		/// i.e. (Math.Cos) or a lambda (v => v * 3)
		/// </summary>
		/// <param name="func">Delegate 'double' method to act as a scalar to process X, Y and Z</param>
		public void Apply(Func<double, double> func)
		{
			X = (float)func(X);
			Y = (float)func(Y);
			Z = (float)func(Z);
		}
		
		/// <summary>
		/// Allows you to pass in a delegate that takes in and returns
		/// a new Vector processed per axis.
		/// i.e. (Math.Cos) or a lambda (v => v * 3)
		/// </summary>
		/// <param name="func">Delegate 'double' method to act as a scalar to process X, Y and Z</param>
		public Vector3 New(Func<double, double> func) => new Vector3((float)func(X), (float)func(Y), (float)func(Z));

		public static Vector3 operator *(Vector3 left, float scalar) => new Vector3(left.X * scalar, left.Y * scalar, left.Z * scalar);
		public static Vector3 operator *(float scalar, Vector3 right) => new Vector3(scalar * right.X, scalar * right.Y, scalar * right.Z);
		public static Vector3 operator *(Vector3 left, Vector3 right) => new Vector3(left.X * right.X, left.Y * right.Y, left.Z * right.Z);
		public static Vector3 operator /(Vector3 left, Vector3 right) => new Vector3(left.X / right.X, left.Y / right.Y, left.Z / right.Z);
		public static Vector3 operator /(Vector3 left, float scalar) => new Vector3(left.X / scalar, left.Y / scalar, left.Z / scalar);
		public static Vector3 operator /(float scalar, Vector3 right) => new Vector3(scalar/ right.X, scalar/ right.Y, scalar/ right.Z);
		public static Vector3 operator +(Vector3 left, Vector3 right) => new Vector3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
		public static Vector3 operator +(Vector3 left, float right) => new Vector3(left.X + right, left.Y + right, left.Z + right);
		public static Vector3 operator -(Vector3 left, Vector3 right) => new Vector3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
		public static Vector3 operator -(Vector3 left, float right) => new Vector3(left.X - right, left.Y - right, left.Z - right);
		public static Vector3 operator -(Vector3 vector) => new Vector3(-vector.X, -vector.Y, -vector.Z);

		public static Vector3 Cross(Vector3 x, Vector3 y)
		{
			return new Vector3(
				x.Y * y.Z - y.Y * x.Z,
				x.Z * y.X - y.Z * x.X,
				x.X * y.Y - y.X * x.Y
			);
		}

		public override bool Equals(object obj) => obj is Vector3 other && Equals(other);
		public bool Equals(Vector3 right) => X == right.X && Y == right.Y && Z == right.Z;

		public override int GetHashCode() => (X, Y, Z).GetHashCode();

		public static bool operator ==(Vector3 left, Vector3 right) => left.Equals(right);
		public static bool operator !=(Vector3 left, Vector3 right) => !(left == right);

		public static Vector3 Cos(Vector3 vector) => vector.New(Math.Cos);
		public static Vector3 Sin(Vector3 vector) => vector.New(Math.Sin);

		public static Vector3 ClampLength(Vector3 vector, float maxLength)
		{
			if (vector.Length() <= maxLength)
				return vector;

			return vector.Normalized() * maxLength;
		}

		public override string ToString() => $"Vector3[{X}, {Y}, {Z}]";

		public Vector2 XY
		{
			get => new Vector2(X, Y);
			set { X = value.X; Y = value.Y; }
		}

		public Vector2 XZ
		{
			get => new Vector2(X, Z);
			set { X = value.X; Z = value.Y; }
		}
		
		public Vector2 YZ
		{
			get => new Vector2(Y, Z);
			set { Y = value.X; Z = value.Y; }
		}
	}
}

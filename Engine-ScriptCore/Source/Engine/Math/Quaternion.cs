using System;
using System.Runtime.InteropServices;

namespace Engine
{
	[StructLayout(LayoutKind.Sequential)]
	public struct Quaternion : IEquatable<Quaternion>
	{
		// These are in X, Y, Z, W order to be compatible with a glm::quat
		public float X;
		public float Y;
		public float Z;
		public float W;

		// Note that this constructor has elements in a different order to
		// the glm::quat constructor which expects w, x, y, z.
		// (even though the storage is x, y, z, w)
		public Quaternion(float x, float y, float z, float w)
		{
			X = x;
			Y = y;
			Z = z;
			W = w;
		}

		public Quaternion(Vector3 xyz, float w)
		{
			X = xyz.X;
			Y = xyz.Y;
			Z = xyz.Z;
			W = w;
		}

		public Quaternion(Vector3 euler)
		{
			Vector3 c = Vector3.Cos(euler * 0.5f);
			Vector3 s = Vector3.Sin(euler * 0.5f);

			X = s.X * c.Y * c.Z - c.X * s.Y * s.Z;
			Y = c.X * s.Y * c.Z + s.X * c.Y * s.Z;
			Z = c.X * c.Y * s.Z - s.X * s.Y * c.Z;
			W = c.X * c.Y * c.Z + s.X * s.Y * s.Z;
		}

		public static Vector3 operator *(Quaternion q, Vector3 v)
		{
			Vector3 qv = new Vector3(q.X, q.Y, q.Z);
			Vector3 uv = Vector3.Cross(qv, v);
			Vector3 uuv = Vector3.Cross(qv, uv);
			return v + ((uv * q.W) + uuv) * 2.0f;
		}

		public static Quaternion operator *(Quaternion a, Quaternion b)
		{
			Quaternion result = new Quaternion();
			result.X = a.W * b.X + a.X * b.W + a.Y * b.Z - a.Z * b.Y;
			result.Y = a.W * b.Y + a.Y * b.W + a.Z * b.X - a.X * b.Z;
			result.Z = a.W * b.Z + a.Z * b.W + a.X * b.Y - a.Y * b.X;
			result.W = a.W * b.W - a.X * b.X - a.Y * b.Y - a.Z * b.Z;
			return result;
		}

		public Vector3 XYZ
		{
			get => new Vector3(X, Y, Z);
			set
			{
				X = value.X;
				Y = value.Y;
				Z = value.Z;
			}
		}

		public override int GetHashCode() => (W, X, Y, Z).GetHashCode();

		public override bool Equals(object obj) => obj is Quaternion other && Equals(other);

		public bool Equals(Quaternion right) => X == right.X && Y == right.Y && Z == right.Z && W == right.W;

		public static bool operator ==(Quaternion left, Quaternion right) => left.Equals(right);
		public static bool operator !=(Quaternion left, Quaternion right) => !(left == right);

		public float Length
		{
			get
			{
				return (float)System.Math.Sqrt(LengthSquared);
			}
		}

		public float LengthSquared
		{
			get
			{
				return X * X + Y * Y + Z * Z + W * W;
			}
		}

		public Vector3 EulerAngles()
		{
			float roll = Mathf.Atan2(2.0f * (X * Y + W * Z), W * W + X * X - Y * Y - Z * Z);
			float y = 2.0f * (Y * Z + W * X);
			float x = W * W - X * X - Y * Y + Z * Z;

			float pitch = 0.0f;

			if (Vector2.EpsilonEquals(new Vector2(x, y), Vector2.Zero))
				pitch = 2.0f * Mathf.Atan2(X, W);
			else
				pitch = Mathf.Atan2(y, x);

			float yaw = Mathf.Asin(Mathf.Clamp(-2.0f * (X * Z - W * Y), -1.0f, 1.0f));

			return new Vector3(pitch, yaw, roll);
		}

		public static Quaternion FromToRotation(Vector3 aFrom, Vector3 aTo)
		{
			Vector3 axis = Vector3.Cross(aFrom, aTo);
			float angle = Vector3.Angle(aFrom, aTo);
			return Quaternion.AngleAxis(angle, axis.Normalized());
		}

		public static Quaternion AngleAxis(float aAngle, Vector3 aAxis)
		{
			aAxis.Normalize();
			float rad = aAngle * Mathf.Deg2Rad * 0.5f;
			aAxis *= Mathf.Sin(rad);
			return new Quaternion(aAxis.X, aAxis.Y, aAxis.Z, Mathf.Cos(rad));
		}

		public static Quaternion QuaternionLookRotation(Vector3 forward, Vector3 up)
		{
			forward.Normalize();

			Vector3 vector = forward.Normalized();
			Vector3 vector2 = Vector3.Cross(up, vector).Normalized();
			Vector3 vector3 = Vector3.Cross(vector, vector2);
			var m00 = vector2.X;
			var m01 = vector2.Y;
			var m02 = vector2.Z;
			var m10 = vector3.X;
			var m11 = vector3.Y;
			var m12 = vector3.Z;
			var m20 = vector.X;
			var m21 = vector.Y;
			var m22 = vector.Z;


			float num8 = (m00 + m11) + m22;
			var quaternion = new Quaternion();
			if (num8 > 0f)
			{
				var num = (float)Math.Sqrt(num8 + 1f);
				quaternion.W = num * 0.5f;
				num = 0.5f / num;
				quaternion.X = (m12 - m21) * num;
				quaternion.Y = (m20 - m02) * num;
				quaternion.Z = (m01 - m10) * num;
				return quaternion;
			}
			if ((m00 >= m11) && (m00 >= m22))
			{
				var num7 = (float)Math.Sqrt(((1f + m00) - m11) - m22);
				var num4 = 0.5f / num7;
				quaternion.X = 0.5f * num7;
				quaternion.Y = (m01 + m10) * num4;
				quaternion.Z = (m02 + m20) * num4;
				quaternion.W = (m12 - m21) * num4;
				return quaternion;
			}
			if (m11 > m22)
			{
				var num6 = (float)Math.Sqrt(((1f + m11) - m00) - m22);
				var num3 = 0.5f / num6;
				quaternion.X = (m10 + m01) * num3;
				quaternion.Y = 0.5f * num6;
				quaternion.Z = (m21 + m12) * num3;
				quaternion.W = (m20 - m02) * num3;
				return quaternion;
			}
			var num5 = (float)Math.Sqrt(((1f + m22) - m00) - m11);
			var num2 = 0.5f / num5;
			quaternion.X = (m20 + m02) * num2;
			quaternion.Y = (m21 + m12) * num2;
			quaternion.Z = 0.5f * num5;
			quaternion.W = (m01 - m10) * num2;
			return quaternion;
		}

		public void Normalize()
		{
			float scale = 1.0f / this.Length;
			XYZ *= scale;
			W *= scale;
		}

		public static Quaternion Slerp(Quaternion a, Quaternion b, float t)
		{
			if (t > 1) t = 1;
			if (t < 0) t = 0;
			return SlerpUnclamped(a, b, t);
		}

		public static Quaternion SlerpUnclamped(Quaternion a, Quaternion b, float t)
		{
			// if either input is zero, return the other.
			if (a.LengthSquared == 0.0f)
			{
				if (b.LengthSquared == 0.0f)
				{
					return new Quaternion(0, 0, 0, 1);
				}
				return b;
			}
			else if (b.LengthSquared == 0.0f)
			{
				return a;
			}


			float cosHalfAngle = a.W * b.W + Vector3.Dot(a.XYZ, b.XYZ);

			if (cosHalfAngle >= 1.0f || cosHalfAngle <= -1.0f)
			{
				// angle = 0.0f, so just return one input.
				return a;
			}
			else if (cosHalfAngle < 0.0f)
			{
				b.XYZ = -b.XYZ;
				b.W = -b.W;
				cosHalfAngle = -cosHalfAngle;
			}

			float blendA;
			float blendB;
			if (cosHalfAngle < 0.99f)
			{
				// do proper slerp for big angles
				float halfAngle = (float)System.Math.Acos(cosHalfAngle);
				float sinHalfAngle = (float)System.Math.Sin(halfAngle);
				float oneOverSinHalfAngle = 1.0f / sinHalfAngle;
				blendA = (float)System.Math.Sin(halfAngle * (1.0f - t)) * oneOverSinHalfAngle;
				blendB = (float)System.Math.Sin(halfAngle * t) * oneOverSinHalfAngle;
			}
			else
			{
				// do lerp if angle is really small.
				blendA = 1.0f - t;
				blendB = t;
			}

			Quaternion result = new Quaternion(blendA * a.XYZ + blendB * b.XYZ, blendA * a.W + blendB * b.W);
			if (result.LengthSquared > 0.0f)
			{
				result.Normalize();
				return result;
			}
			
			return new Quaternion(0, 0, 0, 1);
		}

	}
}

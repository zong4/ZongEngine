using System;

namespace Hazel
{
	public class Prefab : IEquatable<Prefab>
	{
		internal AssetHandle m_Handle;
		public AssetHandle Handle => m_Handle;

		internal Prefab() { m_Handle = AssetHandle.Invalid; }
		internal Prefab(AssetHandle handle) { m_Handle = handle; }

		public static implicit operator bool(Prefab prefab)
		{
			return prefab.m_Handle;
		}

		public override bool Equals(object obj) => obj is Prefab other && Equals(other);

		// NOTE(Peter): Implemented according to Microsofts official documentation:
		// https://docs.microsoft.com/en-us/dotnet/csharp/programming-guide/statements-expressions-operators/how-to-define-value-equality-for-a-type
		public bool Equals(Prefab other)
		{
			if (other is null)
				return false;

			if (ReferenceEquals(this, other))
				return true;

			return m_Handle == other.m_Handle;
		}

		public override int GetHashCode() => m_Handle.GetHashCode();

		public static bool operator ==(Prefab prefabA, Prefab prefabB) => prefabA is null ? prefabB is null : prefabA.Equals(prefabB);
		public static bool operator !=(Prefab prefabA, Prefab prefabB) => !(prefabA == prefabB);

	}
}

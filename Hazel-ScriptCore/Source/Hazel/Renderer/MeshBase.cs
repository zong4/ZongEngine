using System;

namespace Hazel
{
	public class MeshBase : IEquatable<MeshBase>
	{
		internal AssetHandle m_Handle;

		protected MeshBase() { m_Handle = AssetHandle.Invalid; }
		protected MeshBase(AssetHandle handle) { m_Handle = handle;}

		public AssetHandle Handle => m_Handle;
		public Material BaseMaterial => GetMaterial(0);

		public override bool Equals(object obj) => obj is MeshBase other && Equals(other);

		public bool Equals(MeshBase other)
		{
			if (other is null)
				return false;

			if (ReferenceEquals(this, other))
				return true;

			return m_Handle == other.m_Handle;
		}

		public static bool operator==(MeshBase left, MeshBase right) => left is null ? right is null : left.Equals(right);
		public static bool operator!=(MeshBase left, MeshBase right) => !(left == right);

		public virtual Material GetMaterial(int index) { return null; }
		public virtual int GetMaterialCount() { return 0; }
	}
}

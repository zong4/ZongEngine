using System;

namespace Hazel
{
	public class MeshBase : Asset<MeshBase>
	{
		internal MeshBase() : base() { }
		internal MeshBase(ulong handle) : base(handle) { }

		public Material? BaseMaterial => GetMaterial(0);

		public virtual Material? GetMaterial(int index) { return null; }
		public virtual int GetMaterialCount() { return 0; }

		public static bool operator==(MeshBase left, MeshBase right) => left is null ? right is null : left.Equals(right);
		public static bool operator!=(MeshBase left, MeshBase right) => !(left == right);
	}
}

namespace Hazel
{
	public class Mesh : MeshBase
	{
		internal Mesh() : base() {}
		internal Mesh(AssetHandle handle) : base(handle) {}

		public override Material GetMaterial(int index)
		{
			if (!InternalCalls.Mesh_GetMaterialByIndex(ref m_Handle, index, out AssetHandle materialHandle))
				return null;

			return materialHandle.IsValid() ? new Material(m_Handle, materialHandle, null) : null;
		}

		public override int GetMaterialCount() => InternalCalls.Mesh_GetMaterialCount(ref m_Handle);
	}
}

namespace Hazel
{
	public class StaticMesh : MeshBase
	{
		internal StaticMesh() : base() {}
		internal StaticMesh(AssetHandle handle) : base(handle) {}

		public override Material GetMaterial(int index)
		{
			if (!InternalCalls.StaticMesh_GetMaterialByIndex(ref m_Handle, index, out AssetHandle materialHandle))
				return null;

			return new Material(m_Handle, materialHandle, null);
		}

		public override int GetMaterialCount() => InternalCalls.StaticMesh_GetMaterialCount(ref m_Handle);
	}
}

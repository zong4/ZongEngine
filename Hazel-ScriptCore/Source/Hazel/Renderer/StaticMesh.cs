namespace Hazel
{
	[EditorAssignable]
	public class StaticMesh : MeshBase
	{
		internal StaticMesh() : base() { }
		internal StaticMesh(ulong handle) : base(handle) { }

		public override Material? GetMaterial(int index)
		{
			AssetHandle materialHandle = AssetHandle.Invalid;

			unsafe
			{
				if (!InternalCalls.StaticMesh_GetMaterialByIndex(Handle, index, &materialHandle))
					return null;
			}

			return new Material(Handle, materialHandle, null);
		}

		public override int GetMaterialCount()
		{
			unsafe { return InternalCalls.StaticMesh_GetMaterialCount(Handle); }
		}
	}
}

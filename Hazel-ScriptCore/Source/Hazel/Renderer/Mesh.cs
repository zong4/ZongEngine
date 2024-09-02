namespace Hazel
{
	[EditorAssignable]
	public class Mesh : MeshBase
	{
		internal Mesh() : base() { }
		internal Mesh(ulong handle) : base(handle) { }

		public override Material? GetMaterial(int index)
		{
			AssetHandle materialHandle;

			unsafe
			{
				if (!InternalCalls.Mesh_GetMaterialByIndex(Handle, index, &materialHandle))
					return null;
			}

			return materialHandle.IsValid() ? new Material(Handle, materialHandle, null) : null;
		}

		public override int GetMaterialCount()
		{
			unsafe
			{
				return InternalCalls.Mesh_GetMaterialCount(Handle);
			}
		}
	}
}

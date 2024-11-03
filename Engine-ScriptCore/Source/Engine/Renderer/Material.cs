using System;

namespace Engine
{
	public sealed class Material : IEquatable<Material>
	{
		internal AssetHandle m_MeshHandle;
		internal AssetHandle m_Handle;
		internal Component m_MeshComponent;

		private ulong ComponentEntityID => m_MeshComponent != null ? m_MeshComponent.Entity.ID : 0;

		public AssetHandle Handle => m_Handle;

		internal Material()
		{
			m_MeshHandle = AssetHandle.Invalid;
			m_Handle = AssetHandle.Invalid;
		}

		// NOTE(Peter): This represents an actual Material asset!
		internal Material(AssetHandle handle)
		{
			m_Handle = handle;
			m_MeshHandle = AssetHandle.Invalid;
			m_MeshComponent = null;
		}

		// NOTE(Peter): This represents a Material that's attached to a mesh component or mesh asset!
		internal Material(AssetHandle meshHandle, AssetHandle handle, Component meshComponent)
		{
			m_MeshHandle = meshHandle;
			m_Handle = handle;
			m_MeshComponent = meshComponent;
		}

		public Vector3 AlbedoColor
		{
			get
			{
				InternalCalls.Material_GetAlbedoColor(ComponentEntityID, ref m_MeshHandle, ref m_Handle, out Vector3 result);
				return result;
			}

			set => InternalCalls.Material_SetAlbedoColor(ComponentEntityID, ref m_MeshHandle, ref m_Handle, ref value);
		}

		public float Metalness
		{
			get => InternalCalls.Material_GetMetalness(ComponentEntityID, ref m_MeshHandle, ref m_Handle);
			set => InternalCalls.Material_SetMetalness(ComponentEntityID, ref m_MeshHandle, ref m_Handle, value);
		}

		public float Roughness
		{
			get => InternalCalls.Material_GetRoughness(ComponentEntityID, ref m_MeshHandle, ref m_Handle);
			set => InternalCalls.Material_SetRoughness(ComponentEntityID, ref m_MeshHandle, ref m_Handle, value);
		}

		public float Emission
		{
			get => InternalCalls.Material_GetEmission(ComponentEntityID, ref m_MeshHandle, ref m_Handle);
			set => InternalCalls.Material_SetEmission(ComponentEntityID, ref m_MeshHandle, ref m_Handle, value);
		}

		public void Set(string uniform, float value) => InternalCalls.Material_SetFloat(ComponentEntityID, ref m_MeshHandle, ref m_Handle, uniform, value);
		public void Set(string uniform, Texture2D texture) => InternalCalls.Material_SetTexture(ComponentEntityID, ref m_MeshHandle, ref m_Handle, uniform, ref texture.m_Handle);
		public void Set(string uniform, Vector3 value) => InternalCalls.Material_SetVector3(ComponentEntityID, ref m_MeshHandle, ref m_Handle, uniform, ref value);
		public void Set(string uniform, Vector4 value) => InternalCalls.Material_SetVector4(ComponentEntityID, ref m_MeshHandle, ref m_Handle, uniform, ref value);

		public void SetTexture(string uniform, Texture2D texture) => InternalCalls.Material_SetTexture(ComponentEntityID, ref m_MeshHandle, ref m_Handle, uniform, ref texture.m_Handle);

		public override bool Equals(object obj) => obj is Material other && Equals(other);
		public bool Equals(Material right)
		{
			if (right is null)
				return false;

			if (ReferenceEquals(this, right))
				return true;

			return m_Handle == right.m_Handle;
		}
		public static bool operator ==(Material left, Material right) => left is null ? right is null : left.Equals(right);
		public static bool operator !=(Material left, Material right) => !(left == right);
	}
}

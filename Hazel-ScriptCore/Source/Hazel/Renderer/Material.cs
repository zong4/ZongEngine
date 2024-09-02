using System;
using System.Runtime.CompilerServices;

namespace Hazel
{
	[EditorAssignable]
	public sealed class Material : Asset<Material>
	{
		internal AssetHandle m_MeshHandle;
		internal Component? m_MeshComponent;

		private ulong ComponentEntityID => m_MeshComponent != null ? m_MeshComponent.Entity.ID : 0;

		internal Material() : base()
		{
			m_MeshHandle = AssetHandle.Invalid;
		}

		// NOTE(Peter): This represents an actual Material asset!
		internal Material(ulong handle) : base(handle)
		{
			m_MeshHandle = AssetHandle.Invalid;
			m_MeshComponent = null;
		}

		// NOTE(Peter): This represents a Material that's attached to a mesh component or mesh asset!
		internal Material(AssetHandle meshHandle, AssetHandle handle, Component? meshComponent) : base(handle.m_Handle)
		{
			m_MeshHandle = meshHandle;
			m_MeshComponent = meshComponent;
		}

		public Vector3 AlbedoColor
		{
			get
			{
				unsafe
				{
					Vector3 result;
					InternalCalls.Material_GetAlbedoColor(ComponentEntityID, m_MeshHandle, Handle, &result);
					return result;
				}
			}

			set
			{
				unsafe
				{
					InternalCalls.Material_SetAlbedoColor(ComponentEntityID, m_MeshHandle, Handle, &value);
				}
			}
		}

		public float Metalness
		{
			get
			{
				unsafe { return InternalCalls.Material_GetMetalness(ComponentEntityID, m_MeshHandle, Handle); }
			}

			set
			{
				unsafe
				{
					InternalCalls.Material_SetMetalness(ComponentEntityID, m_MeshHandle, Handle, value);;
				}
			}
		}

		public float Roughness
		{
			get
			{
				unsafe
				{
					return InternalCalls.Material_GetRoughness(ComponentEntityID, m_MeshHandle, Handle);
				}
			}

			set
			{
				unsafe
				{
					InternalCalls.Material_SetRoughness(ComponentEntityID, m_MeshHandle, Handle, value);
				}
			}
		}

		public float Emission
		{
			get
			{
				unsafe
				{
					return InternalCalls.Material_GetEmission(ComponentEntityID, m_MeshHandle, Handle);
				}
			}

			set
			{
				unsafe
				{
					InternalCalls.Material_SetEmission(ComponentEntityID, m_MeshHandle, Handle, value);
				}
			}
		}

		public void Set(string uniform, float value)
		{
			unsafe
			{
				InternalCalls.Material_SetFloat(ComponentEntityID, m_MeshHandle, Handle, uniform, value);
			}
		}

		public void Set(string uniform, Texture2D texture)
		{
			unsafe
			{
				InternalCalls.Material_SetTexture(ComponentEntityID, m_MeshHandle, Handle, uniform, texture.Handle);
			}
		}

		public void Set(string uniform, Vector3 value)
		{
			unsafe
			{
				InternalCalls.Material_SetVector3(ComponentEntityID, m_MeshHandle, Handle, uniform, &value);
			}
		}

		public void Set(string uniform, Vector4 value)
		{
			unsafe
			{
				InternalCalls.Material_SetVector4(ComponentEntityID, m_MeshHandle, Handle, uniform, &value);
			}
		}

		public void SetTexture(string uniform, Texture2D texture)
		{
			unsafe
			{
				InternalCalls.Material_SetTexture(ComponentEntityID, m_MeshHandle, Handle, uniform, texture.Handle);
			}
		}

		public static bool operator ==(Material left, Material right) => left is null ? right is null : left.Equals(right);
		public static bool operator !=(Material left, Material right) => !(left == right);
	}
}

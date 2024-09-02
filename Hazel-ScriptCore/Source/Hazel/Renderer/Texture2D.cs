using Coral.Managed.Interop;
using System;

namespace Hazel
{

	public enum TextureWrapMode
	{
		None = 0,
		Clamp,
		Repeat
	}

	public enum TextureFilterMode
	{
		None = 0,
		Linear,
		Nearest,
		Cubic
	}

	[EditorAssignable]
    public class Texture2D : Asset<Texture2D>
    {
		private uint m_Width;
		private uint m_Height;
		public uint Width => m_Width;
		public uint Height => m_Height;

		public Texture2D(ulong handle) : base(handle)
		{
			unsafe
			{
				fixed (uint* width = &m_Width, height = &m_Height)
				{
					InternalCalls.Texture2D_GetSize(width, height);
				}
			}
		}

		public void SetData(Vector4[] data)
		{
			unsafe
			{
				using var arr = new NativeArray<Vector4>(data);
				InternalCalls.Texture2D_SetData(Handle, arr);
			}
		}

		public static Texture2D? Create(uint width, uint height, TextureWrapMode wrapMode = TextureWrapMode.Repeat, TextureFilterMode filterMode = TextureFilterMode.Linear, Vector4[]? data = null)
		{
			if (width == 0)
				throw new ArgumentException("Tried to create a Texture2D with a width of 0.");

			if (height == 0)
				throw new ArgumentException("Tried to create a Texture2D with a height of 0.");

			AssetHandle handle;
			unsafe
			{
				if (!InternalCalls.Texture2D_Create(width, height, wrapMode, filterMode, &handle))
					return null;
			}

			Texture2D texture = new Texture2D(handle.m_Handle) { m_Width = width, m_Height = height };

			if (data != null && data.Length > 0)
				texture.SetData(data);

			return texture;
		}
	}
}

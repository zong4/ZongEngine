using System;

namespace Engine
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

    public class Texture2D
    {
		internal AssetHandle m_Handle;

		public AssetHandle Handle => m_Handle;
		public uint Width { get; private set; }
		public uint Height { get; private set; }

		internal Texture2D() { m_Handle = AssetHandle.Invalid; }

		public Texture2D(AssetHandle handle)
		{
			m_Handle = handle;
			InternalCalls.Texture2D_GetSize(out uint width, out uint height);
			Width = width;
			Height = height;
		}

		public void SetData(Vector4[] data) => InternalCalls.Texture2D_SetData(ref m_Handle, data);
		//public Vector4[] GetData() => InternalCalls.Texture2D_GetData(ref m_Handle);

		public static Texture2D Create(uint width, uint height, TextureWrapMode wrapMode = TextureWrapMode.Repeat, TextureFilterMode filterMode = TextureFilterMode.Linear, Vector4[] data = null)
		{
			if (width == 0)
				throw new ArgumentException("Tried to create a Texture2D with a width of 0.");

			if (height == 0)
				throw new ArgumentException("Tried to create a Texture2D with a height of 0.");

			if (!InternalCalls.Texture2D_Create(width, height, wrapMode, filterMode, out AssetHandle handle))
				return null;

			Texture2D texture = new Texture2D() { m_Handle = handle, Width = width, Height = height };

			if (data != null && data.Length > 0)
				texture.SetData(data);

			return texture;
		}
	}
}

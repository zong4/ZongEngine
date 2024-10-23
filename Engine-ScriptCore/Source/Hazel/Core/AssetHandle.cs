using System.Runtime.InteropServices;

namespace Hazel
{
	[StructLayout(LayoutKind.Sequential)]
	public struct AssetHandle
	{
		public static readonly AssetHandle Invalid = new AssetHandle(0);

		internal ulong m_Handle;

		public AssetHandle(ulong handle) { m_Handle = handle; }

		public bool IsValid() => InternalCalls.AssetHandle_IsValid(ref this);

		public static implicit operator bool(AssetHandle assetHandle)
		{
			return InternalCalls.AssetHandle_IsValid(ref assetHandle);
		}

		public override string ToString() => m_Handle.ToString();
		public override int GetHashCode() => m_Handle.GetHashCode();
	}
}

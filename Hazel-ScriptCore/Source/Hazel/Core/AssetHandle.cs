using System;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;

namespace Hazel
{
	[StructLayout(LayoutKind.Sequential)]
	public struct AssetHandle : IEquatable<AssetHandle>
	{
		public static readonly AssetHandle Invalid = new AssetHandle(0);

		internal readonly ulong m_Handle;

		public AssetHandle(ulong handle) { m_Handle = handle; }

		public bool IsValid()
		{
			unsafe { return InternalCalls.AssetHandle_IsValid(this); }
		}

		public static implicit operator bool(AssetHandle assetHandle)
		{
			unsafe { return InternalCalls.AssetHandle_IsValid(assetHandle); }
		}

		public override string ToString() => m_Handle.ToString();
		public override int GetHashCode() => m_Handle.GetHashCode();

		public bool Equals(AssetHandle other) => m_Handle == other.m_Handle;
		public override bool Equals([NotNullWhen(true)] object? obj) => obj is AssetHandle handle && Equals(handle);

		public static bool operator ==(AssetHandle? handle0, AssetHandle? handle1) => handle0 is null ? handle1 is null : handle0.Equals(handle1);
		public static bool operator !=(AssetHandle? handle0, AssetHandle? handle1) => !(handle0 == handle1);

	}
}

using System;
using System.Collections.Generic;

namespace Hazel
{
	public partial class Asset<T> : IEquatable<T> where T : Asset<T>
	{

		public AssetHandle Handle { get; init; }

		internal Asset()
		{
			Handle = AssetHandle.Invalid;
		}

		internal Asset(ulong handle)
		{
			Handle = new AssetHandle(handle);
		}

		// NOTE(Peter): Implemented according to Microsofts official documentation:
		// https://docs.microsoft.com/en-us/dotnet/csharp/programming-guide/statements-expressions-operators/how-to-define-value-equality-for-a-type
		public override bool Equals(object? obj) => obj is T asset && Equals(asset);

		public bool Equals(T? other)
		{
			if (other is null)
				return false;

			if (ReferenceEquals(this, other))
				return true;

			return Handle == other.Handle;
		}

		public override int GetHashCode() => Handle.GetHashCode();

		public bool IsValid() => Handle.IsValid();

		public static bool IsValid(Asset<T>? asset) => asset != null && asset.IsValid();
		public static implicit operator bool(Asset<T>? asset) => IsValid(asset);
	}
}

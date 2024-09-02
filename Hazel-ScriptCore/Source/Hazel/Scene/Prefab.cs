using System;

namespace Hazel
{
	[EditorAssignable]
	public class Prefab : Asset<Prefab>
	{
		public static bool operator ==(Prefab prefabA, Prefab prefabB) => prefabA is null ? prefabB is null : prefabA.Equals(prefabB);
		public static bool operator !=(Prefab prefabA, Prefab prefabB) => !(prefabA == prefabB);
	}
}

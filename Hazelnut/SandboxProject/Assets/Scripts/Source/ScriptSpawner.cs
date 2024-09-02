using System;
using Hazel;

namespace Sandbox
{
	public class ScriptSpawner : Entity
	{

		public Prefab ScriptPrefab;
		public int Rows = 10;
		public int Columns = 10;

		protected override void OnCreate()
		{
			for (int x = 0; x < Rows; x++)
			{
				for (int z = 0; z < Columns; z++)
				{
					Transform transform = new Transform(new Vector3(x, 0.0f, z), Vector3.Zero, Vector3.One);
					Scene.InstantiatePrefab(ScriptPrefab, transform);
				}
			}
		}

	}
}

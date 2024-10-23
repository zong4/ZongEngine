using System;
using Hazel;

namespace Sandbox.Tests
{
	public class EntityTest : Entity
	{

		public Entity MyEntity;

		protected override void OnCreate()
		{
			Log.Info(this == MyEntity);
		}

	}
}

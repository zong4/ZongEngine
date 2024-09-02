using System;
using Hazel;

namespace Sandbox
{
	public class TeleportTest : Entity
	{

		protected override void OnUpdate(float ts)
		{
			if (Input.IsKeyPressed(KeyCode.T))
			{
				// To "Teleport" an entity, we can simply change its position
				Translation += Vector3.Forward * 5.0f;
			}
		}

	}
}

using System;
using Engine;

namespace Sandbox
{
	public class TeleportTest : Entity
	{

		protected override void OnUpdate(float ts)
		{
			if (Input.IsKeyPressed(KeyCode.T))
			{
				GetComponent<RigidBodyComponent>().Teleport(Translation + Vector3.Forward * 5.0f);
			}
		}

	}
}

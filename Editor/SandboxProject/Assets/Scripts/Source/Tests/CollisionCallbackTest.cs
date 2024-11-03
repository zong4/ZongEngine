using System;
using Engine;

namespace Sandbox.Tests
{
	public class CollisionCallbackTest : Entity
	{

		protected override void OnCreate()
		{
			CollisionBeginEvent += OnCollisionBegin;
			CollisionEndEvent += OnCollisionEnd;
		}

		void OnCollisionBegin(Entity other)
		{
			Log.Info($"Collided with {other.Tag}");
		}

		void OnCollisionEnd(Entity other)
		{
			Log.Info($"Stopped colliding with {other.Tag}");
		}

	}
}

using System;
using Engine;

namespace Sandbox
{
	public class Testing : Entity
	{

		private float m_Value = 0.0f;

		protected override void OnUpdate(float ts)
		{
			m_Value += 1.0f * ts;
		}

		protected override void OnDestroy()
		{
			Log.Info($"Value: {m_Value}");
		}

	}
}

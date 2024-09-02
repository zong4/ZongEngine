using Hazel;
using System;

namespace PhysicsTest
{
	class PhysicsTestTrigger : Entity
	{
		private SkyLightComponent? m_Lights;

		protected override void OnCreate()
		{
			Entity? skylight = Scene.FindEntityByTag("Sky Light");
			m_Lights = skylight?.GetComponent<SkyLightComponent>();

			TriggerBeginEvent += OnTriggerBegin;
			TriggerEndEvent += OnTriggerEnd;
		}

		void OnTriggerBegin(Entity other)
		{
			Log.Trace("Trigger begin");
			if (m_Lights != null)
			{
				m_Lights.Intensity = 0.0f;
			}
		}

		void OnTriggerEnd(Entity other)
		{
			Log.Trace("Trigger end");
			if (m_Lights != null)
			{
				m_Lights.Intensity = 1.0f;
			}
		}
	}
}

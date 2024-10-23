using Hazel;

namespace Tests
{
	public class DOFTest : Entity
	{
		public Entity DOFTarget;

		private float m_Time = 0.0f;

		private bool m_DOFOrigEnabled = false;
		private float m_DOFOrigBlurSize = 0.0f;
		private float m_DOFOrigFocusDistance = 0.0f;

		protected override void OnCreate()
		{
			m_DOFOrigEnabled = SceneRenderer.DepthOfField.IsEnabled;
			m_DOFOrigFocusDistance = SceneRenderer.DepthOfField.FocusDistance;
			m_DOFOrigBlurSize = SceneRenderer.DepthOfField.BlurSize;

			SceneRenderer.DepthOfField.IsEnabled = true;
			SceneRenderer.DepthOfField.BlurSize = 2.0f;
		}

		protected override void OnDestroy()
		{
			// Restore original SceneRenderer DOF settings on exit
			SceneRenderer.DepthOfField.IsEnabled = m_DOFOrigEnabled;
			SceneRenderer.DepthOfField.FocusDistance = m_DOFOrigFocusDistance;
			SceneRenderer.DepthOfField.BlurSize = m_DOFOrigBlurSize;
		}

		protected override void OnUpdate(float ts)
		{
			m_Time += ts;

			if (DOFTarget.ID != 0)
			{
				float z = Mathf.Sin(m_Time * 0.5f) * 3.0f;
				DOFTarget.Translation = new Vector3(DOFTarget.Translation.XY, z);

				float distance = Vector3.Distance(DOFTarget.Transform.WorldTransform.Position, Transform.WorldTransform.Position);
				SceneRenderer.DepthOfField.FocusDistance = distance;
			}

		}

	}
}

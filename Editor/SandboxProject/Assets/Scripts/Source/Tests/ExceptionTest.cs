using Engine;

namespace Sandbox.Tests
{

	public class NullClass
	{
		public float Data;
	}

	public class ExceptionTest : Entity
	{

		private NullClass m_Null = null;

		protected override void OnCreate()
		{
			Log.Info(m_Null.Data);
		}

	}
}

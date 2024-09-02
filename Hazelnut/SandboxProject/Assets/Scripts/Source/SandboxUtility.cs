using Hazel;

namespace Sandbox
{
	public static class SandboxUtility
	{

		public static float GetControllerAxis(int controllerID, int axis, float deadzone = 0.2f)
		{
			float value = Input.GetControllerAxis(controllerID, axis);
			return Mathf.Abs(value) < deadzone ? 0.0f : value;
		}

		public static float GetHorizontalMovement(int controllerID, float controllerDeadzone = 0.2f)
		{
			byte hat = Input.GetControllerHat(controllerID, 0);
			if ((hat & 2) != 0)
				return 1.0f;
			if ((hat & 8) != 0)
				return -1.0f;

			float value = GetControllerAxis(controllerID, 0, controllerDeadzone);

			if (Input.IsKeyDown(KeyCode.A))
				value = -1.0f;
			else if (Input.IsKeyDown(KeyCode.D))
				value = 1.0f;

			return value;
		}

		public static float GetVerticalMovement(int controllerID, float controllerDeadzone = 0.2f)
		{
			byte hat = Input.GetControllerHat(controllerID, 0);
			if ((hat & 4) != 0)
				return 1.0f;
			if ((hat & 1) != 0)
				return -1.0f;

			float value = -GetControllerAxis(controllerID, 1, controllerDeadzone);

			if (Input.IsKeyDown(KeyCode.W))
				value = 1.0f;
			else if (Input.IsKeyDown(KeyCode.S))
				value = -1.0f;

			return value;
		}

		public static float GetGamepadAxisDelta(int controllerID, int axis)
		{
			float value = GetControllerAxis(controllerID, axis);
			value *= -1.0f;
			return (value * value) * Mathf.Sign(value);
		}

	}
}

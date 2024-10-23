namespace Hazel
{
	public enum CursorMode
	{
		Normal = 0,
		Hidden = 1,
		Locked = 2
	}

	public enum MouseButton
	{
		Button0 = 0,
		Button1 = 1,
		Button2 = 2,
		Button3 = 3,
		Button4 = 4,
		Button5 = 5,
		Left = Button0,
		Right = Button1,
		Middle = Button2
	}

	public class Input
	{
		/// <summary>
		/// Returns true the first frame that the key represented by the given KeyCode is pressed down
		/// </summary>
		public static bool IsKeyPressed(KeyCode keycode) => InternalCalls.Input_IsKeyPressed(keycode);

		/// <summary>
		/// Returns true every frame after the key was initially pressed (returns false when <see cref="Input.IsKeyPressed(KeyCode)"/> returns true)
		/// </summary>
		public static bool IsKeyHeld(KeyCode keycode) => InternalCalls.Input_IsKeyHeld(keycode);
		
		/// <summary>
		/// Returns true every frame that the key is down. Equivalent to doing <code>Input.IsKeyPressed(key) || Input.IsKeyHeld(key)</code>
		/// </summary>
		public static bool IsKeyDown(KeyCode keycode) => InternalCalls.Input_IsKeyDown(keycode);

		/// <summary>
		/// Returns true during the frame that the key was released
		/// </summary>
		public static bool IsKeyReleased(KeyCode keycode) => InternalCalls.Input_IsKeyReleased(keycode);

		/// <summary>
		/// Returns true the first frame that the button represented by the given MouseButton is pressed down
		/// </summary>
		public static bool IsMouseButtonPressed(MouseButton button) => InternalCalls.Input_IsMouseButtonPressed(button);

		/// <summary>
		/// Returns true every frame after the button was initially pressed (returns false when <see cref="Input.IsMouseButtonPressed(MouseButton)"/> returns true)
		/// </summary>
		public static bool IsMouseButtonHeld(MouseButton button) => InternalCalls.Input_IsMouseButtonHeld(button);

		/// <summary>
		/// Returns true every frame that the button is down. Equivalent to doing <code>Input.IsMouseButtonPressed(key) || Input.IsMouseButtonHeld(key)</code>
		/// </summary>
		public static bool IsMouseButtonDown(MouseButton button) => InternalCalls.Input_IsMouseButtonDown(button);
		
		/// <summary>
		/// Returns true during the frame that the button was released
		/// </summary>
		public static bool IsMouseButtonReleased(MouseButton button) => InternalCalls.Input_IsMouseButtonReleased(button);

		public static Vector2 GetMousePosition()
		{
			InternalCalls.Input_GetMousePosition(out Vector2 position);
			return position;
		}

		public static bool IsControllerPresent(int id) => InternalCalls.Input_IsControllerPresent(id);
		public static int[] GetConnectedControllerIDs() => InternalCalls.Input_GetConnectedControllerIDs();
		public static string GetControllerName(int id) => InternalCalls.Input_GetControllerName(id);
		
		/// <summary>
		/// Returns true during the frame that the button was released
		/// </summary>
		public static bool IsControllerButtonPressed(int id, GamepadButton button) => InternalCalls.Input_IsControllerButtonPressed(id, (int)button);
		public static bool IsControllerButtonPressed(int id, int button) => InternalCalls.Input_IsControllerButtonPressed(id, button);
		
		/// <summary>
		/// Returns true every frame after the button was initially pressed (returns false when <see cref="Input.IsMouseButtonPressed(MouseButton)"/> returns true)
		/// </summary>
		public static bool IsControllerButtonHeld(int id, GamepadButton button) => InternalCalls.Input_IsControllerButtonHeld(id, (int)button);
		public static bool IsControllerButtonHeld(int id, int button) => InternalCalls.Input_IsControllerButtonHeld(id, button);
		
		/// <summary>
		/// Returns true every frame that the button is down. Equivalent to doing <code>Input.IsMouseButtonPressed(key) || Input.IsMouseButtonHeld(key)</code>
		/// </summary>
		public static bool IsControllerButtonDown(int id, GamepadButton button) => InternalCalls.Input_IsControllerButtonDown(id, (int)button);
		public static bool IsControllerButtonDown(int id, int button) => InternalCalls.Input_IsControllerButtonDown(id, button);
		
		/// <summary>
		/// Returns true during the frame that the button was released
		/// </summary>
		public static bool IsControllerButtonReleased(int id, GamepadButton button) => InternalCalls.Input_IsControllerButtonReleased(id, (int)button);
		public static bool IsControllerButtonReleased(int id, int button) => InternalCalls.Input_IsControllerButtonReleased(id, button);
		public static float GetControllerAxis(int id, int axis) => InternalCalls.Input_GetControllerAxis(id, axis);
		public static byte GetControllerHat(int id, int hat) => InternalCalls.Input_GetControllerHat(id, hat);

		/// <summary>
		/// Getter for the specified controller's axis' deadzone, default value is 0.0f
		/// </summary>
		public static float GetControllerDeadzone(int id, int axis) => InternalCalls.Input_GetControllerDeadzone(id, axis);

		/// <summary>
		/// Setter for the specified controller's axis' deadzone, default value is 0.0f
		/// </summary>
		public static void SetControllerDeadzone(int id, int axis, float deadzone) => InternalCalls.Input_SetControllerDeadzone(id, axis, deadzone);
		public static void SetCursorMode(CursorMode mode) => InternalCalls.Input_SetCursorMode(mode);
		public static CursorMode GetCursorMode() => InternalCalls.Input_GetCursorMode();
	}
}

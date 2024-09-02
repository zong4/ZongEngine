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

	public static class Input
	{
		/// <summary>
		/// Returns true the first frame that the key represented by the given KeyCode is pressed down
		/// </summary>
		public static bool IsKeyPressed(KeyCode keycode)
		{
			unsafe { return InternalCalls.Input_IsKeyPressed(keycode); }
		}

		/// <summary>
		/// Returns true every frame after the key was initially pressed (returns false when <see cref="Input.IsKeyPressed(KeyCode)"/> returns true)
		/// </summary>
		public static bool IsKeyHeld(KeyCode keycode)
		{
			unsafe { return InternalCalls.Input_IsKeyHeld(keycode); }
		}

		/// <summary>
		/// Returns true every frame that the key is down. Equivalent to doing <code>Input.IsKeyPressed(key) || Input.IsKeyHeld(key)</code>
		/// </summary>
		public static bool IsKeyDown(KeyCode keycode)
		{
			unsafe { return InternalCalls.Input_IsKeyDown(keycode); }
		}

		/// <summary>
		/// Returns true during the frame that the key was released
		/// </summary>
		public static bool IsKeyReleased(KeyCode keycode)
		{
			unsafe { return InternalCalls.Input_IsKeyReleased(keycode); }
		}

		/// <summary>
		/// Returns true the first frame that the button represented by the given MouseButton is pressed down
		/// </summary>
		public static bool IsMouseButtonPressed(MouseButton button)
		{
			unsafe { return InternalCalls.Input_IsMouseButtonPressed(button); }
		}

		/// <summary>
		/// Returns true every frame after the button was initially pressed (returns false when <see cref="Input.IsMouseButtonPressed(MouseButton)"/> returns true)
		/// </summary>
		public static bool IsMouseButtonHeld(MouseButton button)
		{
			unsafe { return InternalCalls.Input_IsMouseButtonHeld(button); }
		}

		/// <summary>
		/// Returns true every frame that the button is down. Equivalent to doing <code>Input.IsMouseButtonPressed(key) || Input.IsMouseButtonHeld(key)</code>
		/// </summary>
		public static bool IsMouseButtonDown(MouseButton button)
		{
			unsafe { return InternalCalls.Input_IsMouseButtonDown(button); }
		}

		/// <summary>
		/// Returns true during the frame that the button was released
		/// </summary>
		public static bool IsMouseButtonReleased(MouseButton button)
		{
			unsafe { return InternalCalls.Input_IsMouseButtonReleased(button); }
		}

		public static Vector2 GetMousePosition()
		{
			Vector2 position;
			unsafe { InternalCalls.Input_GetMousePosition(&position); }
			return position;
		}

		public static bool IsControllerPresent(int id)
		{
			unsafe { return InternalCalls.Input_IsControllerPresent(id); }
		}

		public static int[] GetConnectedControllerIDs()
		{
			unsafe
			{
				using var result = InternalCalls.Input_GetConnectedControllerIDs();
				return result;
			}
		}

		public static string GetControllerName(int id)
		{
			unsafe
			{
				using var name = InternalCalls.Input_GetControllerName(id);
				return name;
			}
		}

		/// <summary>
		/// Returns true during the frame that the button was released
		/// </summary>
		public static bool IsControllerButtonPressed(int id, GamepadButton button)
		{
			unsafe { return InternalCalls.Input_IsControllerButtonPressed(id, (int)button); }
		}

		public static bool IsControllerButtonPressed(int id, int button)
		{
			unsafe { return InternalCalls.Input_IsControllerButtonPressed(id, button); }
		}

		/// <summary>
		/// Returns true every frame after the button was initially pressed (returns false when <see cref="Input.IsMouseButtonPressed(MouseButton)"/> returns true)
		/// </summary>
		public static bool IsControllerButtonHeld(int id, GamepadButton button)
		{
			unsafe { return InternalCalls.Input_IsControllerButtonHeld(id, (int)button); }
		}

		public static bool IsControllerButtonHeld(int id, int button)
		{
			unsafe { return InternalCalls.Input_IsControllerButtonHeld(id, button); }
		}

		/// <summary>
		/// Returns true every frame that the button is down. Equivalent to doing <code>Input.IsMouseButtonPressed(key) || Input.IsMouseButtonHeld(key)</code>
		/// </summary>
		public static bool IsControllerButtonDown(int id, GamepadButton button)
		{
			unsafe { return InternalCalls.Input_IsControllerButtonDown(id, (int)button); }
		}

		public static bool IsControllerButtonDown(int id, int button)
		{
			unsafe { return InternalCalls.Input_IsControllerButtonDown(id, button); }
		}

		/// <summary>
		/// Returns true during the frame that the button was released
		/// </summary>
		public static bool IsControllerButtonReleased(int id, GamepadButton button)
		{
			unsafe { return InternalCalls.Input_IsControllerButtonReleased(id, (int)button); }
		}

		public static bool IsControllerButtonReleased(int id, int button)
		{
			unsafe { return InternalCalls.Input_IsControllerButtonReleased(id, button); }
		}

		public static float GetControllerAxis(int id, int axis)
		{
			unsafe { return InternalCalls.Input_GetControllerAxis(id, axis); }
		}

		public static byte GetControllerHat(int id, int hat)
		{
			unsafe { return InternalCalls.Input_GetControllerHat(id, hat); }
		}

		/// <summary>
		/// Getter for the specified controller's axis' deadzone, default value is 0.0f
		/// </summary>
		public static float GetControllerDeadzone(int id, int axis)
		{
			unsafe { return InternalCalls.Input_GetControllerDeadzone(id, axis); }
		}

		/// <summary>
		/// Setter for the specified controller's axis' deadzone, default value is 0.0f
		/// </summary>
		public static void SetControllerDeadzone(int id, int axis, float deadzone)
		{
			unsafe { InternalCalls.Input_SetControllerDeadzone(id, axis, deadzone); }
		}

		public static void SetCursorMode(CursorMode mode)
		{
			unsafe { InternalCalls.Input_SetCursorMode(mode); }
		}

		public static CursorMode GetCursorMode()
		{
			unsafe { return InternalCalls.Input_GetCursorMode(); }
		}
	}
}

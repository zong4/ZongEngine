#include "hzpch.h"
#include "Engine/Core/Input.h"
#include "Window.h"

#include "Engine/Core/Application.h"
#include "Engine/ImGui/ImGui.h"

#include <GLFW/glfw3.h>
#include <imgui/imgui_internal.h>

namespace Hazel {

	void Input::Update()
	{
		// Cleanup disconnected controller
		for (auto it = s_Controllers.begin(); it != s_Controllers.end(); )
		{
			int id = it->first;
			if (glfwJoystickPresent(id) != GLFW_TRUE)
				it = s_Controllers.erase(it);
			else
				it++;
		}

		// Update controllers
		for (int id = GLFW_JOYSTICK_1; id < GLFW_JOYSTICK_LAST; id++)
		{
			if (glfwJoystickPresent(id) == GLFW_TRUE)
			{
				Controller& controller = s_Controllers[id];
				controller.ID = id;
				controller.Name = glfwGetJoystickName(id);

				int buttonCount;
				const unsigned char* buttons = glfwGetJoystickButtons(id, &buttonCount);
				for (int i = 0; i < buttonCount; i++)
				{
					if(buttons[i] == GLFW_PRESS && !controller.ButtonDown[i])
						controller.ButtonStates[i].State = KeyState::Pressed;
					else if(buttons[i] == GLFW_RELEASE && controller.ButtonDown[i])
						controller.ButtonStates[i].State = KeyState::Released;

					controller.ButtonDown[i] = buttons[i] == GLFW_PRESS;
				}

				int axisCount;
				const float* axes = glfwGetJoystickAxes(id, &axisCount);
				for (int i = 0; i < axisCount; i++)
					controller.AxisStates[i] = abs(axes[i]) > controller.DeadZones[i] ? axes[i] : 0.0f;

				int hatCount;
				const unsigned char* hats = glfwGetJoystickHats(id, &hatCount);
				for (int i = 0; i < hatCount; i++)
					controller.HatStates[i] = hats[i];
			}
		}
	}

	bool Input::IsKeyPressed(KeyCode key)
	{
		return s_KeyData.find(key) != s_KeyData.end() && s_KeyData[key].State == KeyState::Pressed;
	}

	bool Input::IsKeyHeld(KeyCode key)
	{
		return s_KeyData.find(key) != s_KeyData.end() && s_KeyData[key].State == KeyState::Held;
	}

	bool Input::IsKeyDown(KeyCode keycode)
	{
		bool enableImGui = Application::Get().GetSpecification().EnableImGui;
		if (!enableImGui)
		{
			auto& window = static_cast<Window&>(Application::Get().GetWindow());
			auto state = glfwGetKey(static_cast<GLFWwindow*>(window.GetNativeWindow()), static_cast<int32_t>(keycode));
			return state == GLFW_PRESS || state == GLFW_REPEAT;
		}
		
		auto& window = static_cast<Window&>(Application::Get().GetWindow());
		GLFWwindow* win = static_cast<GLFWwindow*>(window.GetNativeWindow());
		ImGuiContext* context = ImGui::GetCurrentContext();
		bool pressed = false;
		for (ImGuiViewport* viewport : context->Viewports)
		{
			if (!viewport->PlatformUserData)
				continue;

			GLFWwindow* windowHandle = *(GLFWwindow**)viewport->PlatformUserData; // First member is GLFWwindow
			if (!windowHandle)
				continue;
			auto state = glfwGetKey(windowHandle, static_cast<int32_t>(keycode));
			if (state == GLFW_PRESS || state == GLFW_REPEAT)
			{
				pressed = true;
				break;
			}
		}

		return pressed;
	}

	bool Input::IsKeyReleased(KeyCode key)
	{
		return s_KeyData.find(key) != s_KeyData.end() && s_KeyData[key].State == KeyState::Released;
	}

	bool Input::IsMouseButtonPressed(MouseButton button)
	{
		return s_MouseData.find(button) != s_MouseData.end() && s_MouseData[button].State == KeyState::Pressed;
	}

	bool Input::IsMouseButtonHeld(MouseButton button)
	{
		return s_MouseData.find(button) != s_MouseData.end() && s_MouseData[button].State == KeyState::Held;
	}

	bool Input::IsMouseButtonDown(MouseButton button)
	{
		bool enableImGui = Application::Get().GetSpecification().EnableImGui;
		if (!enableImGui)
		{
			auto& window = static_cast<Window&>(Application::Get().GetWindow());
			auto state = glfwGetMouseButton(static_cast<GLFWwindow*>(window.GetNativeWindow()), static_cast<int32_t>(button));
			return state == GLFW_PRESS;
		}
	
		ImGuiContext* context = ImGui::GetCurrentContext();
		bool pressed = false;
		for (ImGuiViewport* viewport : context->Viewports)
		{
			if (!viewport->PlatformUserData)
				continue;

			GLFWwindow* windowHandle = *(GLFWwindow**)viewport->PlatformUserData; // First member is GLFWwindow
			if (!windowHandle)
				continue;

			auto state = glfwGetMouseButton(static_cast<GLFWwindow*>(windowHandle), static_cast<int32_t>(button));
			if (state == GLFW_PRESS || state == GLFW_REPEAT)
			{
				pressed = true;
				break;
			}
		}
		return pressed;
	}

	bool Input::IsMouseButtonReleased(MouseButton button)
	{
		return s_MouseData.find(button) != s_MouseData.end() && s_MouseData[button].State == KeyState::Released;
	}

	float Input::GetMouseX()
	{
		auto [x, y] = GetMousePosition();
		return (float)x;
	}

	float Input::GetMouseY()
	{
		auto [x, y] = GetMousePosition();
		return (float)y;
	}

	std::pair<float, float> Input::GetMousePosition()
	{
		auto& window = static_cast<Window&>(Application::Get().GetWindow());

		double x, y;
		glfwGetCursorPos(static_cast<GLFWwindow*>(window.GetNativeWindow()), &x, &y);
		return { (float)x, (float)y };
	}

	// TODO: A better way to do this is to handle it internally, and simply move the cursor the opposite side
	//		of the screen when it reaches the edge
	void Input::SetCursorMode(CursorMode mode)
	{
		auto& window = static_cast<Window&>(Application::Get().GetWindow());
		glfwSetInputMode(static_cast<GLFWwindow*>(window.GetNativeWindow()), GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int)mode);

		if (Application::Get().GetSpecification().EnableImGui)
			UI::SetInputEnabled(mode == CursorMode::Normal);
	}

	CursorMode Input::GetCursorMode()
	{
		auto& window = static_cast<Window&>(Application::Get().GetWindow());
		return (CursorMode)(glfwGetInputMode(static_cast<GLFWwindow*>(window.GetNativeWindow()), GLFW_CURSOR) - GLFW_CURSOR_NORMAL);
	}

	bool Input::IsControllerPresent(int id)
	{
		return s_Controllers.find(id) != s_Controllers.end();
	}

	std::vector<int> Input::GetConnectedControllerIDs()
	{
		std::vector<int> ids;
		ids.reserve(s_Controllers.size());
		for (auto [id, controller] : s_Controllers)
			ids.emplace_back(id);

		return ids;
	}

	const Controller* Input::GetController(int id)
	{
		if (!Input::IsControllerPresent(id))
			return nullptr;

		return &s_Controllers.at(id);
	}

	std::string_view Input::GetControllerName(int id)
	{
		if (!Input::IsControllerPresent(id))
			return {};

		return s_Controllers.at(id).Name;
	}

	bool Input::IsControllerButtonPressed(int controllerID, int button)
	{
		if (!Input::IsControllerPresent(controllerID))
			return false;

		auto& contoller = s_Controllers.at(controllerID);
		return contoller.ButtonStates.find(button) != contoller.ButtonStates.end() && contoller.ButtonStates[button].State == KeyState::Pressed;
	}

	bool Input::IsControllerButtonHeld(int controllerID, int button)
	{
		if (!Input::IsControllerPresent(controllerID))
			return false;

		auto& contoller = s_Controllers.at(controllerID);
		return contoller.ButtonStates.find(button) != contoller.ButtonStates.end() && contoller.ButtonStates[button].State == KeyState::Held;
	}

	bool Input::IsControllerButtonDown(int controllerID, int button)
	{
		if (!Input::IsControllerPresent(controllerID))
			return false;

		const Controller& controller = s_Controllers.at(controllerID);
		if (controller.ButtonDown.find(button) == controller.ButtonDown.end())
			return false;

		return controller.ButtonDown.at(button);
	}

	bool Input::IsControllerButtonReleased(int controllerID, int button)
	{
		if (!Input::IsControllerPresent(controllerID))
			return true;

		auto& contoller = s_Controllers.at(controllerID);
		return contoller.ButtonStates.find(button) != contoller.ButtonStates.end() && contoller.ButtonStates[button].State == KeyState::Released;
	}

	float Input::GetControllerAxis(int controllerID, int axis)
	{
		if (!Input::IsControllerPresent(controllerID))
			return 0.0f;

		const Controller& controller = s_Controllers.at(controllerID);
		if (controller.AxisStates.find(axis) == controller.AxisStates.end())
			return 0.0f;

		return controller.AxisStates.at(axis);
	}

	uint8_t Input::GetControllerHat(int controllerID, int hat)
	{
		if (!Input::IsControllerPresent(controllerID))
			return 0;

		const Controller& controller = s_Controllers.at(controllerID);
		if (controller.HatStates.find(hat) == controller.HatStates.end())
			return 0;

		return controller.HatStates.at(hat);
	}

	float Input::GetControllerDeadzone(int controllerID, int axis)
	{
		if (!Input::IsControllerPresent(controllerID))
			return 0.0f;

		const Controller& controller = s_Controllers.at(controllerID);
		return controller.DeadZones.at(axis);
	}

	void Input::SetControllerDeadzone(int controllerID, int axis, float deadzone)
	{
		if (!Input::IsControllerPresent(controllerID))
			return;

		Controller& controller = s_Controllers.at(controllerID);
		controller.DeadZones[axis] = deadzone;
	}

	void Input::TransitionPressedKeys()
	{
		for (const auto& [key, keyData] : s_KeyData)
		{
			if (keyData.State == KeyState::Pressed)
				UpdateKeyState(key, KeyState::Held);
		}

	}

	void Input::TransitionPressedButtons()
	{
		for (const auto& [button, buttonData] : s_MouseData)
		{
			if (buttonData.State == KeyState::Pressed)
				UpdateButtonState(button, KeyState::Held);
		}

		for (const auto& [id, controller] : s_Controllers)
		{
			for (const auto& [button, buttonStates] : controller.ButtonStates)
			{
				if (buttonStates.State == KeyState::Pressed)
					UpdateControllerButtonState(id, button, KeyState::Held);
			}
		}
	}

	void Input::UpdateKeyState(KeyCode key, KeyState newState)
	{
		auto& keyData = s_KeyData[key];
		keyData.Key = key;
		keyData.OldState = keyData.State;
		keyData.State = newState;
	}

	void Input::UpdateButtonState(MouseButton button, KeyState newState)
	{
		auto& mouseData = s_MouseData[button];
		mouseData.Button = button;
		mouseData.OldState = mouseData.State;
		mouseData.State = newState;
	}

	void Input::UpdateControllerButtonState(int controllerID, int button, KeyState newState)
	{
		auto& controllerButtonData = s_Controllers.at(controllerID).ButtonStates.at(button);
		controllerButtonData.Button = button;
		controllerButtonData.OldState = controllerButtonData.State;
		controllerButtonData.State = newState;
	}

	void Input::ClearReleasedKeys()
	{
		for (const auto& [key, keyData] : s_KeyData)
		{
			if (keyData.State == KeyState::Released)
				UpdateKeyState(key, KeyState::None);
		}

		for (const auto& [button, buttonData] : s_MouseData)
		{
			if (buttonData.State == KeyState::Released)
				UpdateButtonState(button, KeyState::None);
		}

		for (const auto& [id, controller] : s_Controllers)
		{
			for (const auto& [button, buttonStates] : controller.ButtonStates)
			{
				if (buttonStates.State == KeyState::Released)
					UpdateControllerButtonState(id, button, KeyState::None);
			}
		}
	}
}

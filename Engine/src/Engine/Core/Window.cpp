#include "pch.h"
#include "Window.h"

#include "Engine/Core/Events/ApplicationEvent.h"
#include "Engine/Core/Events/KeyEvent.h"
#include "Engine/Core/Events/MouseEvent.h"
#include "Engine/Core/Input.h"

#include "Engine/Renderer/RendererAPI.h"

#include "Engine/Platform/Vulkan/VulkanContext.h"
#include "Engine/Platform/Vulkan/VulkanSwapChain.h"

#include <imgui.h>
#include "stb_image.h"

namespace Hazel {

#include "Engine/Embed/HazelIcon.embed"

	static void GLFWErrorCallback(int error, const char* description)
	{
		ZONG_CORE_ERROR_TAG("GLFW", "GLFW Error ({0}): {1}", error, description);
	}

	static bool s_GLFWInitialized = false;

	Window* Window::Create(const WindowSpecification& specification)
	{
		return new Window(specification);
	}

	Window::Window(const WindowSpecification& props)
		: m_Specification(props)
	{
	}

	Window::~Window()
	{
		Shutdown();
	}

	void Window::Init()
	{
		m_Data.Title = m_Specification.Title;
		m_Data.Width = m_Specification.Width;
		m_Data.Height = m_Specification.Height;

		ZONG_CORE_INFO_TAG("GLFW", "Creating window {0} ({1}, {2})", m_Specification.Title, m_Specification.Width, m_Specification.Height);

		if (!s_GLFWInitialized)
		{
			// TODO: glfwTerminate on system shutdown
			int success = glfwInit();
			ZONG_CORE_ASSERT(success, "Could not intialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);

			s_GLFWInitialized = true;
		}

		if (RendererAPI::Current() == RendererAPIType::Vulkan)
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		if (!m_Specification.Decorated)
		{
			// This removes titlebar on all platforms
			// and all of the native window effects on non-Windows platforms
#ifdef ZONG_PLATFORM_WINDOWS
			glfwWindowHint(GLFW_TITLEBAR, false);
#else
			glfwWindowHint(GLFW_DECORATED, false);
#endif
		}

		if (m_Specification.Fullscreen)
		{
			GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

			glfwWindowHint(GLFW_DECORATED, false);
			glfwWindowHint(GLFW_RED_BITS, mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

			m_Window = glfwCreateWindow(mode->width, mode->height, m_Data.Title.c_str(), primaryMonitor, nullptr);
		}
		else
		{
			m_Window = glfwCreateWindow((int)m_Specification.Width, (int)m_Specification.Height, m_Data.Title.c_str(), nullptr, nullptr);
		}

		// Set icon
		{
			GLFWimage icon;
			int channels;
			if (!m_Specification.IconPath.empty())
			{
				std::string iconPathStr = m_Specification.IconPath.string();
				icon.pixels = stbi_load(iconPathStr.c_str(), &icon.width, &icon.height, &channels, 4);
				glfwSetWindowIcon(m_Window, 1, &icon);
				stbi_image_free(icon.pixels);
			}
			else
			{
				// Use embedded Hazel icon
				icon.pixels = stbi_load_from_memory(g_HazelIconPNG, sizeof(g_HazelIconPNG), &icon.width, &icon.height, &channels, 4);
				glfwSetWindowIcon(m_Window, 1, &icon);
				stbi_image_free(icon.pixels);
			}
		}

		// Create Renderer Context
		m_RendererContext = RendererContext::Create();
		m_RendererContext->Init();

		Ref<VulkanContext> context = m_RendererContext.As<VulkanContext>();

		m_SwapChain = hnew VulkanSwapChain();
		m_SwapChain->Init(VulkanContext::GetInstance(), context->GetDevice());
		m_SwapChain->InitSurface(m_Window);

		m_SwapChain->Create(&m_Data.Width, &m_Data.Height, m_Specification.VSync);
		//glfwMaximizeWindow(m_Window);
		glfwSetWindowUserPointer(m_Window, &m_Data);

		bool isRawMouseMotionSupported = glfwRawMouseMotionSupported();
		if (isRawMouseMotionSupported)
			glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		else
			ZONG_CORE_WARN_TAG("Platform", "Raw mouse motion not supported.");

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

			WindowResizeEvent event((uint32_t)width, (uint32_t)height);
			data.EventCallback(event);
			data.Width = width;
			data.Height = height;
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

			WindowCloseEvent event;
			data.EventCallback(event);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

			switch (action)
			{
				case GLFW_PRESS:
				{
					Input::UpdateKeyState((KeyCode)key, KeyState::Pressed);
					KeyPressedEvent event((KeyCode)key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					Input::UpdateKeyState((KeyCode)key, KeyState::Released);
					KeyReleasedEvent event((KeyCode)key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					Input::UpdateKeyState((KeyCode)key, KeyState::Held);
					KeyPressedEvent event((KeyCode)key, 1);
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, uint32_t codepoint)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

			KeyTypedEvent event((KeyCode)codepoint);
			data.EventCallback(event);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

			switch (action)
			{
				case GLFW_PRESS:
				{
					Input::UpdateButtonState((MouseButton)button, KeyState::Pressed);
					MouseButtonPressedEvent event((MouseButton)button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					Input::UpdateButtonState((MouseButton)button, KeyState::Released);
					MouseButtonReleasedEvent event((MouseButton)button);
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double x, double y)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
			MouseMovedEvent event((float)x, (float)y);
			data.EventCallback(event);
		});

		glfwSetTitlebarHitTestCallback(m_Window, [](GLFWwindow* window, int x, int y, int* hit)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
			WindowTitleBarHitTestEvent event(x, y, *hit);
			data.EventCallback(event);
		});

		glfwSetWindowIconifyCallback(m_Window, [](GLFWwindow* window, int iconified)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
			WindowMinimizeEvent event((bool)iconified);
			data.EventCallback(event);
		});

		m_ImGuiMouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
		m_ImGuiMouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
		m_ImGuiMouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);   // FIXME: GLFW doesn't have this.
		m_ImGuiMouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
		m_ImGuiMouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
		m_ImGuiMouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);  // FIXME: GLFW doesn't have this.
		m_ImGuiMouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);  // FIXME: GLFW doesn't have this.
		m_ImGuiMouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

		// Update window size to actual size
		{
			int width, height;
			glfwGetWindowSize(m_Window, &width, &height);
			m_Data.Width = width;
			m_Data.Height = height;
		}

	}

	void Window::Shutdown()
	{
		m_SwapChain->Destroy();
		hdelete m_SwapChain;
		m_RendererContext.As<VulkanContext>()->GetDevice()->Destroy(); // need to destroy the device _before_ windows window destructor destroys the renderer context (because device Destroy() asks for renderer context...)
		glfwTerminate();
		s_GLFWInitialized = false;
	}

	inline std::pair<float, float> Window::GetWindowPos() const
	{
		int x, y;
		glfwGetWindowPos(m_Window, &x, &y);
		return { (float)x, (float)y };
	}

	void Window::ProcessEvents()
	{
		glfwPollEvents();
		Input::Update();
	}

	void Window::SwapBuffers()
	{
		m_SwapChain->Present();
	}

	void Window::SetVSync(bool enabled)
	{
		m_Specification.VSync = enabled;

		Application::Get().QueueEvent([&]()
		{
			m_SwapChain->SetVSync(m_Specification.VSync);
			m_SwapChain->OnResize(m_Specification.Width, m_Specification.Height);
		});
	}

	bool Window::IsVSync() const
	{
		return m_Specification.VSync;
	}

	void Window::SetResizable(bool resizable) const
	{
		glfwSetWindowAttrib(m_Window, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
	}

	void Window::Maximize()
	{
		glfwMaximizeWindow(m_Window);
	}

	void Window::CenterWindow()
	{
		const GLFWvidmode* videmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		int x = (videmode->width / 2) - (m_Data.Width / 2);
		int y = (videmode->height / 2) - (m_Data.Height / 2);
		glfwSetWindowPos(m_Window, x, y);
	}

	void Window::SetTitle(const std::string& title)
	{
		m_Data.Title = title;
		glfwSetWindowTitle(m_Window, m_Data.Title.c_str());
	}

	VulkanSwapChain& Window::GetSwapChain()
	{
		return *m_SwapChain;
	}

}

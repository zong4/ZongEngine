#include "OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

zong::platform::OpenGLContext::OpenGLContext(GLFWwindow* windowHandle) : m_WindowHandle(windowHandle)
{
    // ZONG_CORE_ASSERT(windowHandle, "Window handle is null!")
}

void zong::platform::OpenGLContext::init()
{
    ZONG_PROFILE_FUNCTION();

    glfwMakeContextCurrent(m_WindowHandle);
    int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    // ZONG_CORE_ASSERT(status, "Failed to initialize Glad!")

    // ZONG_CORE_INFO("OpenGL Info:");
    // ZONG_CORE_INFO("  Vendor: {0}", glGetString(GL_VENDOR));
    // ZONG_CORE_INFO("  Renderer: {0}", glGetString(GL_RENDERER));
    // ZONG_CORE_INFO("  Version: {0}", glGetString(GL_VERSION));

    // ZONG_CORE_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), "Hazel requires at least OpenGL
    // version 4.5!")
}

void zong::platform::OpenGLContext::swapBuffers()
{
    ZONG_PROFILE_FUNCTION();

    glfwSwapBuffers(m_WindowHandle);
}

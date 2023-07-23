#include "GLFWWindow.hpp"

platform::GLFWWindow::GLFWWindow(int argc, char** argv) : Window(argc, argv)
{
    ZONG_PROFILE_FUNCTION();

    init(argc, argv);
}

platform::GLFWWindow::~GLFWWindow()
{
    ZONG_PROFILE_FUNCTION();

    exit();
}

void platform::GLFWWindow::init(int argc, char** argv)
{
    ZONG_PROFILE_FUNCTION();
    ZONG_INFO("GLFWWindow::init()");

    createWindow(argc, argv);
    createInstance();
}

void platform::GLFWWindow::run()
{
    ZONG_PROFILE_FUNCTION();
    ZONG_INFO("GLFWWindow::run()");

    while (!glfwWindowShouldClose(_window))
    {
        glfwSwapBuffers(_window);
        glfwPollEvents();
    }
}

void platform::GLFWWindow::exit()
{
    ZONG_PROFILE_FUNCTION();
    ZONG_INFO("GLFWWindow::exit()");

    vkDestroyInstance(_instance, nullptr);

    glfwDestroyWindow(_window);
    glfwTerminate();
}

void platform::GLFWWindow::createWindow(int argc, char** argv)
{
    ZONG_PROFILE_FUNCTION();

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _window = glfwCreateWindow(640, 480, "Engine", nullptr, nullptr);
}

void platform::GLFWWindow::createInstance()
{
    ZONG_PROFILE_FUNCTION();

    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "Zong Engine";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t     glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.enabledExtensionCount   = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount       = 0;

    if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS)
        ZONG_CORE_CRITICAL("Failed to create instance!");
}
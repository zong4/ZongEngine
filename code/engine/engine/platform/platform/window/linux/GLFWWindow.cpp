#include "GLFWWindow.hpp"

platform::GLFWWindow::GLFWWindow(int argc, char** argv) : Window(argc, argv)
{
    // ZONG_PROFILE_FUNCTION();

    init(argc, argv);
}

platform::GLFWWindow::~GLFWWindow()
{
    // ZONG_PROFILE_FUNCTION();

    exit();
}

void platform::GLFWWindow::init(int argc, char** argv)
{
    ZONG_PROFILE_FUNCTION();
    ZONG_INFO("GLFWWindow::init()");

#ifdef DEBUG
    _enableValidationLayers = true;
#endif

    createWindow(argc, argv);
    initVulkan();
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
    _window = glfwCreateWindow(_width, _height, "Engine", nullptr, nullptr);
}

void platform::GLFWWindow::initVulkan()
{
    ZONG_PROFILE_FUNCTION();

    createInstance();
    pickPhysicalDevice();
}

void platform::GLFWWindow::createInstance()
{
    ZONG_PROFILE_FUNCTION();

    if (_enableValidationLayers && !checkValidationLayerSupport())
        ZONG_CORE_CRITICAL("Validation layers requested, but not available!");

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

    auto extensions                    = getRequiredExtensions();
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (_enableValidationLayers)
    {
        createInfo.enabledLayerCount   = static_cast<uint32_t>(_validationLayers.size());
        createInfo.ppEnabledLayerNames = _validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext             = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS)
        ZONG_CORE_CRITICAL("Failed to create instance!");
}

bool platform::GLFWWindow::checkValidationLayerSupport()
{
    ZONG_PROFILE_FUNCTION();

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : _validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
            return false;
    }

    return true;
}

std::vector<const char*> platform::GLFWWindow::getRequiredExtensions()
{
    ZONG_PROFILE_FUNCTION();

    uint32_t     glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (_enableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

void platform::GLFWWindow::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    ZONG_PROFILE_FUNCTION();

    createInfo                 = {};
    createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void platform::GLFWWindow::pickPhysicalDevice()
{
    ZONG_PROFILE_FUNCTION();
}

VKAPI_ATTR VkBool32 VKAPI_CALL platform::GLFWWindow::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                                   VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                                   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                   void*                                       pUserData)
{
    ZONG_PROFILE_FUNCTION();
    ZONG_CORE_ERROR("validation layer: {0}", pCallbackData->pMessage);

    return VK_FALSE;
}
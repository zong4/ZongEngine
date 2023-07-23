#pragma once

#include "../Window.hpp"
#include "platform/pch.hpp"

namespace platform
{

class GLFWWindow : public Window
{
private:
    GLFWwindow*  _window = nullptr;
    VkInstance   _instance;
    VkSurfaceKHR _surface;

    // Validation layers
    bool _enableValidationLayers = false;

    // Device
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice         _device;
    VkQueue          _graphicsQueue;
    VkQueue          _presentQueue;

    // Swap chain
    VkSwapchainKHR           _swapChain;
    std::vector<VkImage>     _swapChainImages;
    VkFormat                 _swapChainImageFormat;
    VkExtent2D               _swapChainExtent;
    std::vector<VkImageView> _swapChainImageViews;

    const std::vector<const char*> _validationLayers = {"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char*> _deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

public:
    GLFWWindow(int argc, char** argv);
    virtual ~GLFWWindow();

    void run() override;

private:
    void init(int argc, char** argv) override;
    void exit() override;

private:
    void createWindow(int argc, char** argv);
    void initVulkan();
    void exitVulkan();
    void createInstance();
    void createSurface();

    // Validation layers
    bool                     checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void                     populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    // Device
    void               pickPhysicalDevice();
    bool               isDeviceSuitable(VkPhysicalDevice device);
    bool               checkDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    void               createLogicalDevice();

    // Swap chain
    void                    createSwapChain();
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR      chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR        chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D              chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void                    createImageViews();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};

} // namespace platform
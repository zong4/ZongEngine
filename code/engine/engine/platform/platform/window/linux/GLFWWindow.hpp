#pragma once

#include "../Window.hpp"
#include "platform/pch.hpp"

namespace platform
{

class GLFWWindow : public Window
{
private:
    GLFWwindow*              _window = nullptr;
    VkInstance               _instance;
    bool                     _enableValidationLayers = false;
    std::vector<const char*> _validationLayers;
    // VkDebugUtilsMessengerEXT _debugMessenger; // not need

public:
    GLFWWindow(int argc, char** argv);
    virtual ~GLFWWindow();

    void run() override;

private:
    void init(int argc, char** argv) override;
    void exit() override;

private:
    void                     createWindow(int argc, char** argv);
    void                     initVulkan();
    void                     createInstance();
    bool                     checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void                     populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void                     pickPhysicalDevice();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};

} // namespace platform
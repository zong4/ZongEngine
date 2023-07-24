#pragma once

#include "../Window.hpp"

namespace zong
{
namespace platform
{

class GLFWWindow : public Window
{
public:
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

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding   = 0;
            bindingDescription.stride    = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

            attributeDescriptions[0].binding  = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset   = offsetof(Vertex, pos);

            attributeDescriptions[1].binding  = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset   = offsetof(Vertex, color);

            attributeDescriptions[2].binding  = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset   = offsetof(Vertex, texCoord);

            return attributeDescriptions;
        }

        bool operator==(const Vertex& other) const { return pos == other.pos && color == other.color && texCoord == other.texCoord; }
    };

    struct UniformBufferObject
    {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

private:
    GLFWwindow*  _window = nullptr;
    VkInstance   _instance;
    VkSurfaceKHR _surface;

    // Validation layers
    bool _enableValidationLayers = false;

    // Device
    VkPhysicalDevice      _physicalDevice = VK_NULL_HANDLE;
    VkDevice              _device;
    VkSampleCountFlagBits _msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkQueue               _graphicsQueue;
    VkQueue               _presentQueue;

    // Swap chain
    VkSwapchainKHR           _swapChain;
    std::vector<VkImage>     _swapChainImages;
    VkFormat                 _swapChainImageFormat;
    VkExtent2D               _swapChainExtent;
    std::vector<VkImageView> _swapChainImageViews;

    // Renderer
    VkRenderPass               _renderPass;
    VkPipelineLayout           _pipelineLayout;
    VkPipeline                 _graphicsPipeline;
    std::vector<VkFramebuffer> _swapChainFramebuffers;

    // Command
    VkCommandPool                _commandPool;
    std::vector<VkCommandBuffer> _commandBuffers;

    // Fence
    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence>     _inFlightFences;
    uint32_t                 _currentFrame = 0;

    // Callback
    bool _framebufferResized = false;

    // Vertex
    VkBuffer       _vertexBuffer;
    VkDeviceMemory _vertexBufferMemory;
    VkBuffer       _indexBuffer;
    VkDeviceMemory _indexBufferMemory;

    // Descriptor
    VkDescriptorSetLayout        _descriptorSetLayout;
    VkDescriptorPool             _descriptorPool;
    std::vector<VkDescriptorSet> _descriptorSets;

    // Uniform
    std::vector<VkBuffer>       _uniformBuffers;
    std::vector<VkDeviceMemory> _uniformBuffersMemory;
    std::vector<void*>          _uniformBuffersMapped;

    // Image
    VkImage        _colorImage;
    VkDeviceMemory _colorImageMemory;
    VkImageView    _colorImageView;
    VkImage        _depthImage;
    VkDeviceMemory _depthImageMemory;
    VkImageView    _depthImageView;

    // Texture
    uint32_t       _mipLevels;
    VkImage        _textureImage;
    VkDeviceMemory _textureImageMemory;
    VkImageView    _textureImageView;
    VkSampler      _textureSampler;

    const std::vector<const char*> _validationLayers    = {"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char*> _deviceExtensions    = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const int                      MAX_FRAMES_IN_FLIGHT = 2;

    std::vector<Vertex>   _vertices;
    std::vector<uint32_t> _indices;

public:
    inline void setFramebufferResized(bool value) { _framebufferResized = value; }

public:
    GLFWWindow(int argc, char** argv);
    virtual ~GLFWWindow();

    void run() override;

private:
    void init(int argc, char** argv) override;
    void drawFrame();
    void exit() override;

private:
    void initWindow(int argc, char** argv);
    void initVulkan();
    void exitVulkan();
    void exitSwapChain();
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
    void                    recreateSwapChain();

    // Renderer
    void           createRenderPass();
    void           createGraphicsPipeline();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void           createFramebuffers();

    // Command
    void            createCommandPool();
    void            createCommandBuffers();
    void            recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    VkCommandBuffer beginSingleTimeCommands();
    void            endSingleTimeCommands(VkCommandBuffer commandBuffer);

    // Fence
    void createSyncObjects();

    // Vertex
    void     createVertexBuffer();
    void     createIndexBuffer();
    void     createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
                          VkDeviceMemory& bufferMemory);
    void     copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // Descriptor
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();

    // Uniform
    void createUniformBuffers();
    void updateUniformBuffer(uint32_t currentImage);

    // Texture
    void                  generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    VkSampleCountFlagBits getMaxUsableSampleCount();
    void        transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
    void        createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
                            VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
                            VkDeviceMemory& imageMemory);
    void        createTextureImage();
    void        createTextureImageView();
    void        createTextureSampler();
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
    void        copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    // Depth
    void     createColorResources();
    void     createDepthResources();
    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    bool     hasStencilComponent(VkFormat format);

    // Model
    void loadModel();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

    static std::vector<char> readFile(const std::string& filename);

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};

} // namespace platform
} // namespace zong

namespace std
{
template <>
struct hash<zong::platform::GLFWWindow::Vertex>
{
    size_t operator()(zong::platform::GLFWWindow::Vertex const& vertex) const
    {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};
} // namespace std
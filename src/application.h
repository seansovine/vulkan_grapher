#pragma once

#include <array>
#include <cstring>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

void framebufferResizeCallback(GLFWwindow *window, int width, int height);

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

class Application {
public:
    Application();

    ~Application();

    void run();

private:
    struct QueueFamilyIndices {
        uint32_t graphicsFamilyIndex;
        uint32_t computeFamilyIndex;
        uint32_t presentFamilyIndex;
    };

    struct SwapchainConfiguration {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> surfaceFormats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    // Top-level internal functions.

    void initWindow();

    void initVulkan();

    void initUI();

    void drawFrame();

    static void drawUI();

    void recordUICommands(uint32_t bufferIdx);

    void recreateSwapchain();

    // Vulkan, glfw, and imgui setup functions.

    friend void framebufferResizeCallback(GLFWwindow *window, int width, int height);

    VkCommandBuffer beginSingleTimeCommands(VkCommandPool cmdPool);

    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool cmdPool);

    bool checkDeviceExtensions(VkPhysicalDevice device);

    bool checkValidationLayerSupport();

    void cleanupSwapchain();

    void cleanupUIResources();

    void createCommandBuffers();

    void createCommandPool();

    void createDescriptorPool();

    void createFramebuffers();

    void createGraphicsPipeline();

    void createImageViews();

    void createInstance();

    void createLogicalDevice();

    void createRenderPass();

    void createUICommandBuffers();

    void createUICommandPool(VkCommandPool *commandPool, VkCommandPoolCreateFlags flags);

    void createUIDescriptorPool();

    void createUIFramebuffers();

    void createUIRenderPass();

    VkShaderModule createShaderModule(const std::vector<char> &shaderCode);

    void createSurface();

    void createSwapchain();

    void createSyncObjects();

    void getDeviceQueueIndices();

    std::vector<const char *> getRequiredExtensions() const;

    bool isDeviceSuitable(VkPhysicalDevice device);

    VkPhysicalDevice pickPhysicalDevice();

    VkExtent2D pickSwapchainExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);

    static VkPresentModeKHR pickSwapchainPresentMode(const std::vector<VkPresentModeKHR> &presentModes);

    static VkSurfaceFormatKHR pickSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);

    SwapchainConfiguration querySwapchainSupport(const VkPhysicalDevice &physicalDevice);

    void setupDebugMessenger();

private:
    const uint32_t WINDOW_WIDTH = 1200;
    const uint32_t WINDOW_HEIGHT = 900;

    bool framebufferResized = false;

    GLFWwindow *window;

    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;

    QueueFamilyIndices queueIndices;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSurfaceKHR surface;

    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkExtent2D swapchainExtent;
    VkFormat swapchainImageFormat;
    std::vector<VkFramebuffer> swapchainFramebuffers;
    std::vector<VkFramebuffer> uiFramebuffers;

    VkRenderPass renderPass;
    VkRenderPass uiRenderPass;

    VkPipeline graphicsPipeline;
    VkPipelineLayout graphicsPipelineLayout;

    VkCommandPool commandPool;
    VkCommandPool uiCommandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkCommandBuffer> uiCommandBuffers;

    uint32_t imageCount = 0;
    uint32_t currentFrame = 0;
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    VkDescriptorPool descriptorPool;
    VkDescriptorPool uiDescriptorPool;

    std::vector<const char *> requiredExtensions;
    const std::array<const char *, 1> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const std::array<const char *, 1> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

    // Debug utilities

    VkDebugUtilsMessengerEXT debugMessenger;
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
};

#ifndef VULKAN_WRAPPER_H_
#define VULKAN_WRAPPER_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <imgui/backends/imgui_impl_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "vertex.h"

#include <cstdint>
#include <functional>
#include <vector>

// A first version of abstracting over the Vulkan interface as a component
// in the larger application. As the name suggests, this version is for
// integration with resources provided by the GLFW framework.

// Data structs.

struct SwapChainInfo {
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkExtent2D swapchainExtent;
    VkFormat swapchainImageFormat;
    std::vector<VkFramebuffer> swapchainFramebuffers;
};

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

struct DebugInfo {
    // Extension information.
    std::vector<const char *> requiredExtensions;
    const std::vector<const char *> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME //
    };
    const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

    // Debug information.
    VkDebugUtilsMessengerEXT debugMessenger;

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
};

// Main interface class.

class GlfwVulkanWrapper {
    DebugInfo debugInfo;

private:
    // Main Vulkan entities.
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;

    QueueFamilyIndices queueIndices;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSurfaceKHR surface;
    SwapChainInfo swapChainInfo;
    VkRenderPass renderPass;

    VkPipeline graphicsPipeline;
    VkPipelineLayout graphicsPipelineLayout;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    uint32_t imageCount = 0;
    uint32_t currentFrame = 0;
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    VkDescriptorPool descriptorPool;

    using DeinitCallback = void(VkDevice);
    using DrawCallback = VkCommandBuffer(uint32_t, const VkExtent2D &);

    // UI callbacks for dependency injection.
    std::function<DeinitCallback> uiDeinitCallback;
    std::function<DrawCallback> uiDrawCallback;

public:
    uint32_t windowWidth;
    uint32_t windowHeight;
    uint32_t vertexDataSize;

public:
    // Initialization functions.
    void init(GLFWwindow *window, uint32_t windowWidth, uint32_t windowHeight, const std::vector<Vertex> &vertexData);

    void setUiDeinitCallback(const std::function<DeinitCallback> &inUiDeinitCallback) {
        uiDeinitCallback = inUiDeinitCallback;
    }
    void setUiDrawCallback(const std::function<DrawCallback> &inUiDrawCallback) {
        uiDrawCallback = inUiDrawCallback;
    }

    // State management functions.
    void recreateSwapchain(GLFWwindow *window);
    void waitForDeviceIdle();
    void deinit();

    // Rendering functions.
    void drawFrame(GLFWwindow *window, bool frameBufferResized);
    void updateMesh(const std::vector<Vertex> &vertexData);

public:
    uint32_t getImageCount() {
        return imageCount;
    }
    VkDevice getLogicalDevice() {
        return logicalDevice;
    }
    const SwapChainInfo &getSwapchainInfo() {
        return swapChainInfo;
    }
    const QueueFamilyIndices &getQueueIndices() {
        return queueIndices;
    }

    ImGui_ImplVulkan_InitInfo imGuiInitInfo(VkDescriptorPool uiDescriptorPool, VkRenderPass uiRenderPass);

private:
    // Misc. helpers used by initialization.
    std::vector<const char *> getRequiredExtensions() const;
    bool isDeviceSuitable(VkPhysicalDevice device);
    SwapchainConfiguration querySwapchainSupport(const VkPhysicalDevice &device);
    VkShaderModule createShaderModule(const std::vector<char> &shaderCode);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // Swap chain creation helpers.
    VkExtent2D pickSwapchainExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);
    static VkPresentModeKHR pickSwapchainPresentMode(const std::vector<VkPresentModeKHR> &presentModes);
    static VkSurfaceFormatKHR pickSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);

    // Methods for Vulkan setup sequence.
    void createInstance();
    void setupDebugMessenger();
    void createSurface(GLFWwindow *window);
    void pickPhysicalDevice();
    void getDeviceQueueIndices();
    void createLogicalDevice();

    // Setup methods that use the logical device.
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();

    void createFramebuffers();
    void createCommandPool();
    void createVertexBuffer(const std::vector<Vertex> &vertexData);
    void createCommandBuffers();
    void createSyncObjects();
    void createDescriptorPool();

    // Cleanup methods.
    void cleanupSwapchain();
};

#endif // VULKAN_WRAPPER_H_

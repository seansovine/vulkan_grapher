#ifndef VULKAN_WRAPPER_H_
#define VULKAN_WRAPPER_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <imgui/backends/imgui_impl_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "app_state.h"
#include "vertex.h"

#include <cstdint>
#include <functional>
#include <vector>

// A first version of abstracting over the Vulkan interface as a component
// in the larger application. As the name suggests, this version includes
// integration with resources provided by the GLFW framework.

// Main interface class.

class GlfwVulkanWrapper {
    DebugInfo debugInfo;

private:
    GLFWwindow *window = nullptr;

    // Main Vulkan entities.
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    QueueFamilyIndices queueIndices;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    SwapChainInfo swapChainInfo;
    VkRenderPass renderPass;

    // Image for multisampling.
    ImageInfo colorImageInfo;

    // Image for depth buffer;
    ImageInfo depthImageInfo;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    // TODO: Make this a vector.
    std::vector<IndexedMesh> currentMeshes;

    uint32_t imageCount = 0;
    uint32_t currentFrame = 0;
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

private:
    using DeinitUICallback = void(VkDevice);
    using DrawUICallback = VkCommandBuffer(uint32_t, uint32_t, const VkExtent2D &);
    using CreateUIFrameBuffersCallback = void(GlfwVulkanWrapper &);
    using DestroyUIFrameBuffersCallback = void(GlfwVulkanWrapper &);

    // UI callbacks for dependency injection.
    std::function<DeinitUICallback> uiDeinitCallback;
    std::function<DrawUICallback> uiDrawCallback;
    std::function<CreateUIFrameBuffersCallback> createUIFrameBuffersCallback;
    std::function<DestroyUIFrameBuffersCallback> destroyUIFrameBuffersCallback;

public:
    uint32_t windowWidth;
    uint32_t windowHeight;

public:
    // Initialization functions.
    void init(GLFWwindow *window, uint32_t windowWidth, uint32_t windowHeight, std::vector<IndexedMesh> &&meshData);

    void setUIDeinitCallback(const std::function<DeinitUICallback> &inUiDeinitCallback) {
        uiDeinitCallback = inUiDeinitCallback;
    }
    void setUIDrawCallback(const std::function<DrawUICallback> &inUiDrawCallback) {
        uiDrawCallback = inUiDrawCallback;
    }
    void setCreateUIFrameBuffersCallback(const std::function<CreateUIFrameBuffersCallback> &inCallback) {
        createUIFrameBuffersCallback = inCallback;
    }
    void setDestroyUIFrameBuffersCallback(const std::function<DestroyUIFrameBuffersCallback> &inCallback) {
        destroyUIFrameBuffersCallback = inCallback;
    }

    // State management functions.
    void recreateSwapchain();
    void waitForDeviceIdle();
    void deinit();

    // Rendering functions.
    void drawFrame(const AppState &appState, bool frameBufferResized);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

public:
    uint32_t getImageCount() {
        return imageCount;
    }
    VkDevice getLogicalDevice() {
        return device;
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
    SwapchainConfig querySwapchainSupport(const VkPhysicalDevice &device);
    VkShaderModule createShaderModule(const std::vector<char> &shaderCode);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkSampleCountFlagBits getMaxUsableSampleCount();

    // Buffer management helpers.
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer,
                      VkDeviceMemory &bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    // Swap chain creation helpers.
    VkExtent2D pickSwapchainExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);
    static VkPresentModeKHR pickSwapchainPresentMode(const std::vector<VkPresentModeKHR> &presentModes);
    static VkSurfaceFormatKHR pickSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);

    // Image creation helpers.
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
                     VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkImage &image, VkDeviceMemory &imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

    VkFormat findDepthFormat();

    // Methods for Vulkan setup sequence.
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
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
    void createColorResources();
    void createDepthResources();

    // Mesh buffer creation helpers.
    void createVertexBuffer(const std::vector<Vertex> &vertexData, VkBuffer &vertexBuffer,
                            VkDeviceMemory &vertexBufferMemory);
    void createIndexBuffer(const std::vector<uint16_t> &indices, VkBuffer &indexBuffer,
                           VkDeviceMemory &indexBufferMemory);
    void createUniformBuffers(UniformInfo &uniformInfo);

    void createDescriptorSetLayout(IndexedMesh &mesh);
    void createDescriptorPool(IndexedMesh &mesh);
    void createDescriptorSets(IndexedMesh &mesh);

    void createCommandBuffers();
    void createSyncObjects();

    // Cleanup methods.
    void cleanupSwapChain();
};

#endif // VULKAN_WRAPPER_H_

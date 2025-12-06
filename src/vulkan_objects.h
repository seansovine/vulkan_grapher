#ifndef VULKAN_OBJECTS_H_
#define VULKAN_OBJECTS_H_

#include <vulkan/vulkan.h>

#include <vector>

// Data structs.

struct SwapchainConfig {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct SwapChainInfo {
    VkSwapchainKHR swapchain;
    VkExtent2D swapChainExtent;
    VkFormat swapChainImageFormat;

    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> swapchainFramebuffers;
};

struct QueueFamilyIndices {
    uint32_t graphicsFamilyIndex;
    uint32_t computeFamilyIndex;
    uint32_t presentFamilyIndex;
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

struct UniformInfo {
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void *> uniformBuffersMapped;

    void destroy(VkDevice device) {
        for (size_t i = 0; i < uniformBuffers.size(); i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }
    }
};

struct ImageInfo {
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;

    void destroy(VkDevice device) {
        vkDestroyImageView(device, imageView, nullptr);
        vkDestroyImage(device, image, nullptr);
        vkFreeMemory(device, imageMemory, nullptr);
    }
};

#endif // VULKAN_OBJECTS_H_

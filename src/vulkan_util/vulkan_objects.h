#ifndef VULKAN_OBJECTS_H_
#define VULKAN_OBJECTS_H_

#include <vulkan/vulkan.h>

#include <cassert>
#include <iostream>
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

struct DescriptorSetLayout {
    DescriptorSetLayout() = default;

    void init(VkDevice inDevice) {
        device = inDevice;
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding            = 0;
        uboLayoutBinding.descriptorCount    = 1;
        uboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings    = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    ~DescriptorSetLayout() {
        if (layout != nullptr) {
            std::cerr << "MeshDescriptorSetLayout must be explicitly destroyed." << std::endl;
        }
        assert(layout == nullptr);
    }

    void destroy() {
        assert(device != nullptr);
        vkDestroyDescriptorSetLayout(device, layout, nullptr);
        layout = nullptr;
    }

    VkDevice device              = nullptr;
    VkDescriptorSetLayout layout = nullptr;
};

#endif // VULKAN_OBJECTS_H_

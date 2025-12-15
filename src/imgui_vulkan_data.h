#ifndef IMGUI_VULKAN_DATA_H_
#define IMGUI_VULKAN_DATA_H_

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

class GlfwVulkanWrapper;

struct ImGuiVulkanData {
    VkCommandPool uiCommandPool;
    std::vector<VkCommandBuffer> uiCommandBuffers;
    std::vector<VkFramebuffer> uiFramebuffers;
    VkRenderPass uiRenderPass;
    VkDescriptorPool uiDescriptorPool;

public:
    VkCommandBuffer recordDrawCommands(uint32_t currentFrame, uint32_t imageIndex, const VkExtent2D &swapchainExtent);

    void init(GlfwVulkanWrapper &vulkan);
    void deinit(VkDevice logicalDevice);

    void createFrameBuffers(GlfwVulkanWrapper &vulkan);
    void destroyFrameBuffers(GlfwVulkanWrapper &vulkan);

private:
    void createDescriptorPool(GlfwVulkanWrapper &vulkan);
    void createRenderPass(GlfwVulkanWrapper &vulkan);
    void createCommandPool(GlfwVulkanWrapper &vulkan);
    void createCommandBuffers(GlfwVulkanWrapper &vulkan);
};

#endif // IMGUI_VULKAN_DATA_H_

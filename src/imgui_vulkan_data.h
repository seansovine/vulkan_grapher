#ifndef IMGUI_VULKAN_DATA_H_
#define IMGUI_VULKAN_DATA_H_

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

struct ImGuiVulkanData {
    VkCommandPool uiCommandPool;
    std::vector<VkCommandBuffer> uiCommandBuffers;
    std::vector<VkFramebuffer> uiFramebuffers;
    VkRenderPass uiRenderPass;
    VkDescriptorPool uiDescriptorPool;

    VkCommandBuffer recordDrawCommands(uint32_t currentFrame, uint32_t imageIndex, const VkExtent2D &swapchainExtent);

    void deinit(VkDevice logicalDevice);
};

#endif // IMGUI_VULKAN_DATA_H_

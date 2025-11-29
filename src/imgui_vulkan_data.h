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

    VkCommandBuffer recordCommands(uint32_t bufferIdx, const VkExtent2D &swapchainExtent);

    void deinit(VkDevice logicalDevice);
};

#endif // IMGUI_VULKAN_DATA_H_

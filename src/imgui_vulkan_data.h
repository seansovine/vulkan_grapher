#ifndef IMGUI_VULKAN_DATA_H_
#define IMGUI_VULKAN_DATA_H_

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

struct ImGuiVulkanData {
    VkCommandPool uiCommandPool;
    std::vector<VkCommandBuffer> uiCommandBuffers;
    std::vector<VkFramebuffer> uiFramebuffers;
    VkRenderPass uiRenderPass;
    VkDescriptorPool uiDescriptorPool;

    void recordCommands(uint32_t bufferIdx, const VkExtent2D &swapchainExtent);
};

#endif // IMGUI_VULKAN_DATA_H_

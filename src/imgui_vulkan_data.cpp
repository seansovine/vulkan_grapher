#include "imgui_vulkan_data.h"

#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <stdexcept>

VkCommandBuffer ImGuiVulkanData::recordDrawCommands(uint32_t currentFrame, uint32_t imageIndex,
                                                    const VkExtent2D &swapchainExtent) {
    VkCommandBufferBeginInfo cmdBufferBegin = {};
    cmdBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBegin.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(uiCommandBuffers[currentFrame], &cmdBufferBegin) != VK_SUCCESS) {
        throw std::runtime_error("Unable to start recording UI command buffer!");
    }

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = uiRenderPass;
    renderPassBeginInfo.framebuffer = uiFramebuffers[imageIndex];
    renderPassBeginInfo.renderArea.extent.width = swapchainExtent.width;
    renderPassBeginInfo.renderArea.extent.height = swapchainExtent.height;
    renderPassBeginInfo.clearValueCount = 1;

    VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    renderPassBeginInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(uiCommandBuffers[currentFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), uiCommandBuffers[currentFrame]); //
    }

    vkCmdEndRenderPass(uiCommandBuffers[currentFrame]);

    if (vkEndCommandBuffer(uiCommandBuffers[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffers!");
    }

    return uiCommandBuffers[currentFrame];
}

void ImGuiVulkanData::deinit(VkDevice logicalDevice) {
    vkDestroyDescriptorPool(logicalDevice, uiDescriptorPool, nullptr);
    vkFreeCommandBuffers(logicalDevice, uiCommandPool, static_cast<uint32_t>(uiCommandBuffers.size()),
                         uiCommandBuffers.data());
    vkDestroyCommandPool(logicalDevice, uiCommandPool, nullptr);
    vkDestroyRenderPass(logicalDevice, uiRenderPass, nullptr);

    for (auto framebuffer : uiFramebuffers) {
        vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
    }
}

#include "imgui_vulkan_data.h"
#include "vulkan_wrapper.h"

#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <stdexcept>

VkCommandBuffer ImGuiVulkanData::recordDrawCommands(uint32_t currentFrame, uint32_t imageIndex,
                                                    const VkExtent2D &swapchainExtent) {
    VkCommandBufferBeginInfo cmdBufferBegin = {};
    cmdBufferBegin.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBegin.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(uiCommandBuffers[currentFrame], &cmdBufferBegin) != VK_SUCCESS) {
        throw std::runtime_error("Unable to start recording UI command buffer!");
    }

    VkRenderPassBeginInfo renderPassBeginInfo    = {};
    renderPassBeginInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass               = uiRenderPass;
    renderPassBeginInfo.framebuffer              = uiFramebuffers[imageIndex];
    renderPassBeginInfo.renderArea.extent.width  = swapchainExtent.width;
    renderPassBeginInfo.renderArea.extent.height = swapchainExtent.height;
    renderPassBeginInfo.clearValueCount          = 1;

    VkClearValue clearColor          = {0.0f, 0.0f, 0.0f, 1.0f};
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

void ImGuiVulkanData::init(GlfwVulkanWrapper &vulkan) {
    createDescriptorPool(vulkan);
    createRenderPass(vulkan);
    createCommandPool(vulkan);
    createCommandBuffers(vulkan);
    createFrameBuffers(vulkan);
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

void ImGuiVulkanData::createDescriptorPool(GlfwVulkanWrapper &vulkan) {
    VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                         {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                         {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets                    = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount              = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes));
    pool_info.pPoolSizes                 = pool_sizes;

    if (vkCreateDescriptorPool(vulkan.getLogicalDevice(), &pool_info, nullptr, &uiDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Cannot allocate UI descriptor pool!");
    }
}

void ImGuiVulkanData::createFrameBuffers(GlfwVulkanWrapper &vulkan) {
    uiFramebuffers.resize(vulkan.getSwapchainInfo().swapchainImages.size());
    VkImageView attachment[1];

    VkFramebufferCreateInfo info = {};
    info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass              = uiRenderPass;
    info.attachmentCount         = 1;
    info.pAttachments            = attachment;
    info.width                   = vulkan.getSwapchainInfo().swapChainExtent.width;
    info.height                  = vulkan.getSwapchainInfo().swapChainExtent.height;
    info.layers                  = 1;

    for (uint32_t i = 0; i < vulkan.getSwapchainInfo().swapchainImages.size(); ++i) {
        attachment[0] = vulkan.getSwapchainInfo().swapchainImageViews[i];
        if (vkCreateFramebuffer(vulkan.getLogicalDevice(), &info, nullptr, &uiFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Unable to create UI framebuffers!");
        }
    }
}

void ImGuiVulkanData::destroyFrameBuffers(GlfwVulkanWrapper &vulkan) {
    for (auto framebuffer : uiFramebuffers) {
        vkDestroyFramebuffer(vulkan.getLogicalDevice(), framebuffer, nullptr);
    }
}

void ImGuiVulkanData::createCommandPool(GlfwVulkanWrapper &vulkan) {
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex        = vulkan.getQueueIndices().graphicsFamilyIndex;
    commandPoolCreateInfo.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(vulkan.getLogicalDevice(), &commandPoolCreateInfo, nullptr, &uiCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Could not create graphics command pool!");
    }
}

void ImGuiVulkanData::createCommandBuffers(GlfwVulkanWrapper &vulkan) {
    uiCommandBuffers.resize(vulkan.getSwapchainInfo().swapchainImageViews.size());

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool                 = uiCommandPool;
    commandBufferAllocateInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount          = static_cast<uint32_t>(uiCommandBuffers.size());

    if (vkAllocateCommandBuffers(vulkan.getLogicalDevice(), &commandBufferAllocateInfo, uiCommandBuffers.data()) !=
        VK_SUCCESS) {
        throw std::runtime_error("Unable to allocate UI command buffers!");
    }
}

void ImGuiVulkanData::createRenderPass(GlfwVulkanWrapper &vulkan) {
    VkAttachmentDescription attachmentDescription = {};
    attachmentDescription.format                  = vulkan.getSwapchainInfo().swapChainImageFormat;
    attachmentDescription.samples                 = VK_SAMPLE_COUNT_1_BIT;
    // UI is drawn on top of existing image.
    attachmentDescription.loadOp        = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // After the UI pass the images should be ready to present.
    attachmentDescription.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachmentDescription.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    VkAttachmentReference attachmentReference = {};
    attachmentReference.attachment            = 0;
    attachmentReference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &attachmentReference;

    // Make the UI render pass dependent on the main renderpass.
    // The docs say about VK_SUBPASS_EXTERNAL:
    //
    //   If srcSubpass is equal to VK_SUBPASS_EXTERNAL, the first synchronization
    //   scope includes commands that occur earlier in submission order than the
    //   vkCmdBeginRenderPass used to begin the render pass instance.
    //
    // https://docs.vulkan.org/spec/latest/chapters/renderpass.html#VkSubpassDependency

    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass          = 0;
    subpassDependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependency.dstStageMask        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount        = 1;
    renderPassInfo.pAttachments           = &attachmentDescription;
    renderPassInfo.subpassCount           = 1;
    renderPassInfo.pSubpasses             = &subpass;
    renderPassInfo.dependencyCount        = 1;
    renderPassInfo.pDependencies          = &subpassDependency;

    if (vkCreateRenderPass(vulkan.getLogicalDevice(), &renderPassInfo, nullptr, &uiRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create UI render pass!");
    }
}

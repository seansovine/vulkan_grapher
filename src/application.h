#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui_vulkan_data.h"
#include "vertex.h"
#include "vulkan_wrapper.h"

#include <cstring>
#include <vector>

class Application {
    friend void framebufferResizeCallback(GLFWwindow *window, int width, int height);

public:
    Application();

    ~Application();

    void run();

private:
    void initWindow();
    void initVulkan();
    void initUI();

    void drawFrame();
    void drawUI();
    void recordUICommands(uint32_t bufferIdx);
    void recreateSwapchain();

    // This is currently toggles between the two example vertex sets updates
    // the vertex buffer with the new set that is chosen after toggling.
    void toggleMesh();

private:
    // ImGui resource management functions.
    void createUICommandBuffers();
    void createUICommandPool(VkCommandPool *commandPool, VkCommandPoolCreateFlags flags);
    void createUIDescriptorPool();
    void createUIFramebuffers();
    void createUIRenderPass();

    void cleanupUIResources();

private:
    const uint32_t WINDOW_WIDTH = 1200;
    const uint32_t WINDOW_HEIGHT = 900;

    bool framebufferResized = false;

    GLFWwindow *window;
    GlfwVulkanWrapper vulkan;
    ImGuiVulkanData imGuiVulkan;

    TestVertexSet currentTestVertexSet;
    std::vector<Vertex> vertexData;
};

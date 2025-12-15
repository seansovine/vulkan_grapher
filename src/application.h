#ifndef APPLICATION_H_
#define APPLICATION_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui_vulkan_data.h"
#include "vulkan_util/app_state.h"
#include "vulkan_wrapper.h"

#include <cstring>

class Application {
    friend void framebufferResizeCallback(GLFWwindow *window, int width, int height);

    AppState appState;

public:
    Application();

    ~Application();

    void run();

private:
    void initWindow();
    void initVulkan();
    void initUI();

    void drawUI();
    void drawFrame();

    // Toggles between the two example vertex sets updates the
    // vertex buffer with data from the new set after toggling.
    void toggleMesh();

private:
    const uint32_t INITIAL_WINDOW_WIDTH  = 1200;
    const uint32_t INITIAL_WINDOW_HEIGHT = 900;

    bool framebufferResized = false;

    GLFWwindow *window;
    GlfwVulkanWrapper vulkan;
    ImGuiVulkanData imGuiVulkan;
};

#endif // APPLICATION_H_

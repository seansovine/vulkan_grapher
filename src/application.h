#ifndef APPLICATION_H_
#define APPLICATION_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui_vulkan_data.h"
#include "vulkan_util/app_state.h"
#include "vulkan_wrapper.h"

#include <cstdint>
#include <cstring>

class Application {
    friend void framebufferResizeCallback(GLFWwindow *window, int width, int height);

    AppState appState;
    std::vector<IndexedMesh> meshesToRender;

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

    void populateFunctionMeshes();

private:
    const uint32_t INITIAL_WINDOW_WIDTH  = 1500;
    const uint32_t INITIAL_WINDOW_HEIGHT = 900;

    bool framebufferResized = false;

    GLFWwindow *window;

    GlfwVulkanWrapper vulkan;
    ImGuiVulkanData imGuiVulkan;
};

#endif // APPLICATION_H_

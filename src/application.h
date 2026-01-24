#ifndef APPLICATION_H_
#define APPLICATION_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "app_state.h"
#include "imgui_vulkan_data.h"
#include "vulkan_wrapper.h"

#include <array>
#include <cstdint>
#include <cstring>

class Application {
    friend void framebufferResizeCallback(GLFWwindow *window, int width, int height);

    AppState appState;
    std::array<IndexedMesh, 2> meshesToRender;

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
    WindowEvents windowEvents;
};

#endif // APPLICATION_H_

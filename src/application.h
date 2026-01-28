#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "app_state.h"
#include "imgui_vulkan_data.h"
#include "user_function.h"
#include "vulkan_wrapper.h"

#include <GLFW/glfw3.h>

#include <array>
#include <cstdint>
#include <cstring>

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

    void drawUI();
    void drawFunctionInput();
    void handleUserInput();

    void drawFrame();
    bool populateFunctionMeshes();

private:
    static constexpr uint32_t INITIAL_WINDOW_WIDTH  = 1500;
    static constexpr uint32_t INITIAL_WINDOW_HEIGHT = 900;

    uint32_t currentWidth  = INITIAL_WINDOW_WIDTH;
    uint32_t currentHeight = INITIAL_WINDOW_HEIGHT;

    bool framebufferResized = false;

    GLFWwindow *window;

    AppState appState;
    std::array<IndexedMesh, 2> meshesToRender;
    std::shared_ptr<UserFunction> userFunction = nullptr;

    GlfwVulkanWrapper vulkan;
    ImGuiVulkanData imGuiVulkan;
    WindowEvents windowEvents;
};

#endif // APPLICATION_H_

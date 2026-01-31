#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "app_state.h"
#include "imgui_vulkan_data.h"
#include "user_function.h"
#include "vulkan_wrapper.h"

#include <GLFW/glfw3.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <optional>
#include <thread>

using FuncXZPtr = double (*)(double, double);

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
    void populateFunctionMeshes();

    void meshBuilderThreadPtr(const FuncXZPtr func);
    void meshBuilderThreadUser(std::shared_ptr<UserFunction> func);

private:
    static constexpr uint32_t INITIAL_WINDOW_WIDTH  = 1500;
    static constexpr uint32_t INITIAL_WINDOW_HEIGHT = 900;

    uint32_t currentWidth  = INITIAL_WINDOW_WIDTH;
    uint32_t currentHeight = INITIAL_WINDOW_HEIGHT;

    bool framebufferResized = false;

    GLFWwindow *window;

    AppState appState;

    GlfwVulkanWrapper vulkan;
    ImGuiVulkanData imGuiVulkan;
    WindowEvents windowEvents;

    std::shared_ptr<UserFunction> userFunction = nullptr;
    // The main thread only touches these while meshReady is true.
    std::array<IndexedMesh, 2> meshesToRender;

    std::optional<std::thread> meshBuilder = std::nullopt;
    // Set by mesh builder thread, cleared by main thread.
    std::atomic_bool meshReady = false;
};

#endif // APPLICATION_H_

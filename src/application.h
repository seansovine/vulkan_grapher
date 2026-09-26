#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "imgui_vulkan_data.h"
#include "vulkan_wrapper.h"

#include <app_state.h>
#include <function_mesh.h>
#include <user_function.h>

#include <GLFW/glfw3.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <optional>
#include <thread>

using FuncXZPtr = double (*)(double, double);

class Application {
    friend void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    friend void windowMovedCallback(GLFWwindow *window, int x, int y);

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
    bool handleUserInput();
    void tryGetUserFunction();
    void handleMeshGeneratorChange();

    void drawFrame();
    void populateFunctionMeshes();
    void populateMeshesBuiltIn();
    void rebuildMeshesBuiltin();
    void populateMeshesExternal();

    void meshBuilderThreadPtr(const FuncXZPtr func, uint32_t meshSize);
    void meshBuilderThreadUser(std::shared_ptr<UserFunction> func, uint32_t meshSize);
    void meshBuilderThreadGeneric(std::function<FuncXZ> func, uint32_t meshSize);
    void meshBuilderThreadExternal(std::string funcExpression);
    bool backgroundInProgress();

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

    // Set by mesh builder thread, cleared by main thread.
    std::atomic_bool backgroundWorkReady = false;
    // While this is present it has exclusive access to meshesToRender and currentFunction.
    std::optional<std::thread> meshBuilder = std::nullopt;

    // The main thread only touches these while backgroundWorkReady is true.
    std::array<IndexedMesh, 2> meshesToRender = {};
    // Stored function object to rebuild mesh on parameter change for built-in.
    std::function<FuncXZ> currentFunction = nullptr;
};

#endif // APPLICATION_H_

#include "application.h"

#include "app_state.h"
#include "function_mesh.h"
#include "mesh.h"
#include "user_function.h"
#include "util.h"
#include "vulkan_wrapper.h"

#include <GLFW/glfw3.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>
#include <spdlog/spdlog.h>
#include <vk_video/vulkan_video_codec_h265std.h>
#include <vulkan/vulkan_core.h>

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

// GLFW callbacks.

void framebufferResizeCallback(GLFWwindow *window, [[maybe_unused]] int width, [[maybe_unused]] int height) {
    auto app                = static_cast<Application *>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
    app->currentWidth       = static_cast<uint32_t>(width);
    app->currentHeight      = static_cast<uint32_t>(height);
}

// Class methods.

Application::Application() {
    initWindow();
    initVulkan();

    // Setup event handling.
    windowEvents.init(window, &appState);
    // Needs done after our glfw callbacks are set.
    initUI();
}

Application::~Application() {
    spdlog::trace("Cleaning up...");

    // Cleanup DearImGui.
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup Vulkan.
    vulkan.deinit();

    // Cleanup GLFW.
    glfwDestroyWindow(window);
    glfwTerminate();

    spdlog::info("Done.");
}

// Initialization methods.

void Application::initUI() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    imGuiVulkan.init(vulkan);
    ImGui_ImplVulkan_InitInfo init_info = vulkan.imGuiInitInfo(imGuiVulkan.uiDescriptorPool, imGuiVulkan.uiRenderPass);

    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_Init(&init_info);

    // Set ui callbacks on Vulkan wrapper.

    vulkan.setUIDeinitCallback( //
        [this](VkDevice logicalDevice) {
            imGuiVulkan.deinit(logicalDevice); //
        });

    vulkan.setUIDrawCallback( //
        [this](uint32_t currentFrame, uint32_t imageIndex, const VkExtent2D &swapchainExtent) -> VkCommandBuffer {
            return imGuiVulkan.recordDrawCommands(currentFrame, imageIndex, swapchainExtent);
        });

    vulkan.setCreateUIFrameBuffersCallback( //
        [this](GlfwVulkanWrapper &inVulkan) {
            this->imGuiVulkan.createFrameBuffers(inVulkan); //
        });

    vulkan.setDestroyUIFrameBuffersCallback( //
        [this](GlfwVulkanWrapper &inVulkan) {
            this->imGuiVulkan.destroyFrameBuffers(inVulkan); //
        });
}

bool Application::populateFunctionMeshes() {
    using namespace math_util;

    auto populateFromPtr = [this](double (*func)(double, double)) {
        FunctionMesh mesh{func};
        spdlog::debug(" - # function mesh vertices: {}", std::to_string(mesh.functionVertices().size()));
        spdlog::debug(" - # function mesh indices:  {}", std::to_string(mesh.meshIndices().size()));

        auto floorMesh = FunctionMesh::simpleFloorMesh();
        meshesToRender = {IndexedMesh{std::move(mesh.functionVertices()), std::move(mesh.meshIndices())},
                          IndexedMesh{std::move(floorMesh.vertices), std::move(floorMesh.indices)}};
    };

    spdlog::debug("Building function meshes.");

    switch (appState.testFunc) {
    case TestFunc::Parabolic: {
        populateFromPtr(TEST_FUNCTION_PARABOLIC);
        break;
    }
    case TestFunc::ShiftedSinc: {
        populateFromPtr(TEST_FUNCTION_SHIFTED_SCALED_SINC_USER);
        break;
    }
    case TestFunc::ExpSine: {
        populateFromPtr(TEST_FUNCTION_SHIFTED_SCALED_EXP_SINE);
        break;
    }
    case TestFunc::UserInput: {
        if (userFunction == nullptr) {
            return false;
        }
        auto wrapper = [func = std::move(userFunction)](double x, double z) -> double {
            return (*func)(x, z);
        };
        FunctionMesh mesh{std::move(wrapper)};
        spdlog::debug(" - # function mesh vertices: {}", std::to_string(mesh.functionVertices().size()));
        spdlog::debug(" - # function mesh indices:  {}", std::to_string(mesh.meshIndices().size()));

        auto floorMesh = FunctionMesh::simpleFloorMesh();
        meshesToRender = {IndexedMesh{std::move(mesh.functionVertices()), std::move(mesh.meshIndices())},
                          IndexedMesh{std::move(floorMesh.vertices), std::move(floorMesh.indices)}};
        // Should have been reset by move; but to make sure.
        userFunction = nullptr;
        break;
    }
    default: {
        throw std::runtime_error("Invalid test function in populateFunctionMeshes.");
    }
    }
    return true;
}

void Application::initVulkan() {
    populateFunctionMeshes();
    vulkan.init(window, INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT);
    vulkan.updateGraphAndFloorMeshes(meshesToRender, funcNames[appState.selectedFuncIndex()]);
    meshesToRender = {};
}

void Application::initWindow() {
    if (!glfwInit()) {
        throw std::runtime_error("Unable to initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT, "Vulkan Grapher", nullptr, nullptr);

    // Have GLFW store a pointer to this class instance for use in callbacks.
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Unable to create window!");
    }
}

// Render loop methods.

void Application::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawUI();

        ImGuiIO &io = ImGui::GetIO();
        windowEvents.setGuiWantsInputs(io.WantCaptureMouse, io.WantCaptureKeyboard);
        handleUserInput();

        drawFrame();

        // Limit frame computation rate, as we target 60 FPS.
        constexpr int64_t timeout = 1000 * 1 / 65.0f;
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
    }

    vulkan.waitForDeviceIdle();
}

void Application::handleUserInput() {
    UserGuiInput userInput = appState.takerUserGuiInput();
    if (appState.testFunc == TestFunc::UserInput && userInput.enterPressed) {
        spdlog::debug("User pressed enter when in user func mode.");
        userFunction = std::make_unique<UserFunction>();
        try {
            userFunction->assign(appState.functionInputBuffer.data());
        } catch (const BadExpression &error) {
            std::cout << "Failed to parse expression." << std::endl;
            userFunction = nullptr;
        }
        if (userFunction != nullptr && populateFunctionMeshes()) {
            // TODO: This should really be done on a background thread.
            spdlog::debug("Updating mesh from user function");
            vulkan.updateGraphAndFloorMeshes(meshesToRender, userFunction->userExpression());
        }
    }
}

void Application::drawFrame() {
    vulkan.drawFrame(appState, framebufferResized);
    framebufferResized = false;
}

void Application::drawUI() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static constexpr ImVec2 WINDOW_SIZE = {380.0f, 360.0f};
    ImGui::SetNextWindowSize(WINDOW_SIZE, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);

    ImGui::Begin("Settings");
    ImGui::Text("Average framerate: %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    // Add some vertical space.
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    if (ImGui::Button("Toggle Object Rotation")) {
        appState.rotating = !appState.rotating;
    }
    if (ImGui::Button("Toggle Wireframe Graph")) {
        appState.wireframe = !appState.wireframe;
    }
    if (ImGui::Button("Toggle PBR in Vertex")) {
        appState.pbrFragPipeline = !appState.pbrFragPipeline;
    }
    if (ImGui::Button("Toggle Draw Floor")) {
        appState.drawFloor = !appState.drawFloor;
    }
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    static int selectedItem = appState.selectedFuncIndex();
    if (ImGui::Combo("Choose function", &selectedItem, funcNames.data(), funcNames.size())) {
        appState.testFunc = static_cast<TestFunc>(selectedItem);
        if (populateFunctionMeshes()) {
            // TODO: This should really be done on a background thread.
            vulkan.updateGraphAndFloorMeshes(meshesToRender, funcNames[selectedItem]);
        }
    }
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    if (ImGui::Button("Reset position")) {
        appState.resetPosition = true;
    }
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    // PBR graph color.
    ImGui::SliderFloat("Graph color R", &appState.graphColor.r, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Graph color G", &appState.graphColor.g, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Graph color B", &appState.graphColor.b, 0.0f, 1.0f, "%.2f");
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    // PBR material parameters.
    ImGui::SliderFloat("Metallic", &appState.metallic, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Roughness", &appState.roughness, 0.0f, 1.0f, "%.2f");

    ImGui::End();

    if (appState.testFunc == TestFunc::UserInput) {
        drawFunctionInput();
    }

    ImGui::Render();
}

void Application::drawFunctionInput() {
    static constexpr ImVec2 WINDOW_SIZE = {400.0f, 130.0f};
    ImGui::SetNextWindowSize(WINDOW_SIZE, ImGuiCond_Appearing);
    ImVec2 windowLocation = {10.0f, currentHeight - WINDOW_SIZE.y - 10};
    ImGui::SetNextWindowPos(windowLocation, ImGuiCond_Appearing);

    ImGui::Begin("Function y = f(x, z).");

    ImGui::Text("Enter f(x, z):");
    ImGui::InputTextMultiline("", appState.functionInputBuffer.data(), //
                              appState.functionInputBuffer.size(), ImVec2(-1.0f, 50.0f));
    ImGui::Text("Press enter to apply");

    ImGui::End();
}

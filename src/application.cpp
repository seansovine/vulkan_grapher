#include "application.h"

#include "app_state.h"
#include "function_mesh.h"
#include "mesh.h"
#include "vulkan_wrapper.h"

#include <GLFW/glfw3.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>
#include <spdlog/spdlog.h>
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

    auto populate = [this](double (*func)(double, double)) {
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
        populate(TEST_FUNCTION_PARABOLIC);
        break;
    }
    case TestFunc::ShiftedSinc: {
        populate(TEST_FUNCTION_SHIFTED_SINC);
        break;
    }
    case TestFunc::ExpSine: {
        populate(TEST_FUNCTION_SHIFTED_SCALED_EXP_SINE_USER);
        break;
    }
    case TestFunc::UserInput: {
        spdlog::error("User-defined function not yet implemented.");
        return false;
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
        windowEvents.setGuiWantsInputs(io.WantCaptureMouse);
        drawFrame();

        // Limit frame computation rate, as we target 60 FPS.
        constexpr int64_t timeout = 1000 * 1 / 65.0f;
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
    }

    vulkan.waitForDeviceIdle();
}

void Application::drawFrame() {
    vulkan.drawFrame(appState, framebufferResized);
    framebufferResized = false;
}

void Application::drawUI() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

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
    ImGui::Render();
}

#include "application.h"

#include "app_state.h"
#include "function_mesh.h"
#include "mesh.h"
#include "vulkan_wrapper.h"

#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>
#include <vulkan/vulkan_core.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

// GLFW callbacks.

void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto app                = static_cast<Application *>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

// Class methods.

Application::Application() {
    initWindow();
    initVulkan();
    initUI();
}

Application::~Application() {
    std::cout << "Cleaning up..." << std::endl;

    // Cleanup DearImGui.
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup Vulkan.
    vulkan.deinit();

    // Cleanup GLFW.
    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Done." << std::endl;
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

void Application::populateFunctionMeshes() {
    static auto TEST_FUNCTION_PARABOLIC = [](double x, double y) -> double {
        return 0.75 - (x - 0.5) * (x - 0.5) - (y - 0.5) * (y - 0.5);
    };

    static auto sinc = [](double x, double y) -> double {
        double scale = 30; // 100
        double mag   = scale * std::sqrt(x * x + y * y);
        return mag == 0.0 ? 1.0 : std::sin(mag) / mag;
    };
    static auto TEST_FUNCTION_SHIFTED_SINC = [](double x, double y) -> double {
        return 0.75 * sinc(x - 0.5, y - 0.5) + 0.25; //
    };

    auto populate = [this](double (*func)(double, double)) {
        FunctionMesh mesh{func};
        std::cout << " - # function mesh vertices: " << std::to_string(mesh.functionVertices().size()) << std::endl;
        std::cout << " - # function mesh indices:  " << std::to_string(mesh.meshIndices().size()) << std::endl;

        meshesToRender = {IndexedMesh{mesh.floorVertices(), mesh.meshIndices()},
                          IndexedMesh{mesh.functionVertices(), mesh.meshIndices()}};
    };

    std::cout << "Building function meshes." << std::endl;

    switch (appState.testFunc) {
    case TestFunc::Parabolic: {
        populate(TEST_FUNCTION_PARABOLIC);
        break;
    }
    case TestFunc::ShiftedSinc: {
        populate(TEST_FUNCTION_SHIFTED_SINC);
        break;
    }
    default: {
        throw std::runtime_error("Invalid test function in populateFunctionMeshes.");
    }
    }
}

void Application::initVulkan() {
    // Set false for very basic mesh debugging.
    static constexpr bool RENDER_FUNCTION = true;

    if (RENDER_FUNCTION) {
        populateFunctionMeshes();

    } else {
        meshesToRender = {IndexedMesh{TEST_VERTICES_1, TEST_INDICES}, IndexedMesh{TEST_VERTICES_2, TEST_INDICES}};
    }

    vulkan.init(window, INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT, std::move(meshesToRender));
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
    glfwSetKeyCallback(window, keyCallback);
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
    if (ImGui::Button("Toggle Test Function")) {
        appState.toggleTestFunc();
        populateFunctionMeshes();
        vulkan.updateMeshes(meshesToRender);
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

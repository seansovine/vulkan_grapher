#include "application.h"

#include "app_state.h"
#include "function_mesh.h"
#include "gmsh_wrapper.h"
#include "mesh.h"
#include "user_function.h"
#include "util.h"
#include "vulkan_wrapper.h"

#include <GLFW/glfw3.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// GLFW callbacks.

void framebufferResizeCallback(GLFWwindow *window, [[maybe_unused]] int width, [[maybe_unused]] int height) {
    auto app                = static_cast<Application *>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
    app->currentWidth       = static_cast<uint32_t>(width);
    app->currentHeight      = static_cast<uint32_t>(height);
}

void windowMovedCallback(GLFWwindow *window, [[maybe_unused]] int x, [[maybe_unused]] int y) {
    [[maybe_unused]] auto app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    // TODO: GLFW passes a mouse down (only) event on title bar click that needs special handling.
}

// Main class startup and shutdown.

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
            imGuiVulkan.deinit(logicalDevice);
        });

    vulkan.setUIDrawCallback( //
        [this](uint32_t currentFrame, uint32_t imageIndex, const VkExtent2D &swapchainExtent) -> VkCommandBuffer {
            return imGuiVulkan.recordDrawCommands(currentFrame, imageIndex, swapchainExtent);
        });

    vulkan.setCreateUIFrameBuffersCallback( //
        [this](GlfwVulkanWrapper &inVulkan) {
            this->imGuiVulkan.createFrameBuffers(inVulkan);
        });

    vulkan.setDestroyUIFrameBuffersCallback( //
        [this](GlfwVulkanWrapper &inVulkan) {
            this->imGuiVulkan.destroyFrameBuffers(inVulkan);
        });
}

void Application::meshBuilderThreadPtr(const FuncXZPtr func) {
    FunctionMesh mesh{func};
    auto floorMesh      = FunctionMesh::simpleFloorMesh();
    meshesToRender      = {IndexedMesh{std::move(mesh.functionVertices()), std::move(mesh.meshIndices())},
                           IndexedMesh{std::move(floorMesh.vertices), std::move(floorMesh.indices)}};
    backgroundWorkReady = true;
}

void Application::meshBuilderThreadUser(std::shared_ptr<UserFunction> func) {
    FunctionMesh mesh{*func};
    auto floorMesh      = FunctionMesh::simpleFloorMesh();
    meshesToRender      = {IndexedMesh{std::move(mesh.functionVertices()), std::move(mesh.meshIndices())},
                           IndexedMesh{std::move(floorMesh.vertices), std::move(floorMesh.indices)}};
    backgroundWorkReady = true;
}

void Application::meshBuilderThreadExternal(std::string funcExpression) {
    // Call into Gmsh wrapper; this version blocks main thread, for initial testing.
    gmsh_wrapper::VertsAndIndices vertsAndInds{};
    try {
        vertsAndInds = gmsh_wrapper::runGmsh(funcExpression);
    } catch (const std::exception &e) {
        spdlog::debug("Gmsh error: {}", e.what());
        appState.functionParseError = true;
        backgroundWorkReady         = true;
        return;
    }
    auto floorMesh      = FunctionMesh::simpleFloorMesh();
    meshesToRender      = {IndexedMesh{std::move(vertsAndInds.vertices), std::move(vertsAndInds.indices)},
                           IndexedMesh{std::move(floorMesh.vertices), std::move(floorMesh.indices)}};
    backgroundWorkReady = true;
}

bool Application::backgroundInProgress() {
    return meshBuilder.has_value();
}

void Application::populateFunctionMeshes() {
    switch (appState.meshGenerator) {
        case MeshGenerator::BuiltIn: {
            populateMeshesBuiltIn();
            break;
        }
#ifdef BUILD_GMSH
        case MeshGenerator::Gmsh: {
            populateMeshesExternal();
            break;
        }
#endif
        default: {
            throw std::runtime_error("Invalid mesh generator setting.");
        }
    }
}

void Application::populateMeshesBuiltIn() {
    using namespace math_util;

    spdlog::debug("Building function meshes.");

    switch (appState.testFunc) {
        case TestFunc::Parabolic: {
            meshBuilder = std::thread(&Application::meshBuilderThreadPtr, this, TEST_FUNCTION_PARABOLIC);
            break;
        }
        case TestFunc::ShiftedSinc: {
            meshBuilder = std::thread(&Application::meshBuilderThreadPtr, this, TEST_FUNCTION_SHIFTED_SCALED_SINC_USER);
            break;
        }
        case TestFunc::ExpSine: {
            meshBuilder = std::thread(&Application::meshBuilderThreadPtr, this, TEST_FUNCTION_SHIFTED_SCALED_EXP_SINE);
            break;
        }
        case TestFunc::UserInput: {
            if (appState.textBufferLen() == 0) {
                userFunction                = nullptr;
                appState.functionParseError = false;
                return;
            }
            if (userFunction == nullptr) {
                return;
            }
            meshBuilder  = std::thread(&Application::meshBuilderThreadUser, this, std::move(userFunction));
            userFunction = nullptr;
            break;
        }
        default: {
            throw std::runtime_error("Invalid test function in populateFunctionMeshes.");
        }
    }
}

void Application::populateMeshesExternal() {
    spdlog::debug("Building function meshes with Gmsh.");

    std::string functionExpression{};
    switch (appState.testFunc) {
        case TestFunc::Parabolic: {
            functionExpression = math_util::gmsh::TEST_FUNCTION_PARABOLIC_EXPR_;
            break;
        }
        case TestFunc::ShiftedSinc: {
            functionExpression = math_util::gmsh::TEST_FUNCTION_SINC_EXPR_;
            break;
        }
        case TestFunc::ExpSine: {
            functionExpression = math_util::gmsh::TEST_FUNCTION_EXP_SINE_EXPR_;
            break;
        }
        case TestFunc::UserInput: {
            if (appState.textBufferLen() != 0) {
                functionExpression = appState.functionInputBuffer.data();
            } else {
                return;
            }
            break;
        }
        default: {
            throw std::runtime_error("Invalid test function in populateFunctionMeshes.");
        }
    }

    meshBuilder = std::thread(&Application::meshBuilderThreadExternal, this, std::move(functionExpression));
}

void Application::initVulkan() {
    populateFunctionMeshes();
    vulkan.init(window, INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT);
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
    glfwSetWindowPosCallback(window, windowMovedCallback);

    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Unable to create window!");
    }
}

// Render loop methods.

void Application::run() {
    while (!glfwWindowShouldClose(window)) {
        auto start = std::chrono::high_resolution_clock::now();
        glfwPollEvents();
        drawUI();

        ImGuiIO &io = ImGui::GetIO();
        windowEvents.setGuiWantsInputs(io.WantCaptureMouse, io.WantCaptureKeyboard);
        handleUserInput();

        drawFrame();
        auto now     = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start);

        // Limit frame rate, target 60 FPS.
        constexpr auto target = std::chrono::nanoseconds(static_cast<long>(1e9 / (double)60.0));
        auto sleepTime        = std::max(std::chrono::nanoseconds(0), target - elapsed);
        std::this_thread::sleep_for(sleepTime);

        if (backgroundWorkReady) {
            if (!appState.functionParseError) {
                auto numVerts = fmt::format(std::locale(), "{:L}", meshesToRender[0].vertices.size());
                auto numInds  = fmt::format(std::locale(), "{:L}", meshesToRender[0].indices.size());
                spdlog::debug(" - # function mesh vertices: {}", numVerts);
                spdlog::debug(" - # function mesh indices:  {}", numInds);

                // Moves out of meshesToRender.
                vulkan.updateGraphAndFloorMeshes(meshesToRender);
            }

            if (meshBuilder.has_value()) {
                meshBuilder->join();
                meshBuilder = std::nullopt;
            }
            meshesToRender      = {};
            backgroundWorkReady = false;
        }
    }

    vulkan.waitForDeviceIdle();
}

void Application::tryGetUserFunction() {
    userFunction = std::make_shared<UserFunction>();
    appState.trimFunctionInput();
    try {
        userFunction->assign(appState.functionInputBuffer.data());
    } catch (const BadExpression &error) {
        userFunction                = nullptr;
        appState.functionParseError = true;
    }
    if (userFunction != nullptr) {
        appState.functionParseError = false;
        populateFunctionMeshes();
    }
}

void Application::handleMeshGeneratorChange() {
    if (meshBuilder.has_value()) {
        spdlog::warn("Mesh generator change while mesh generation is in progress.");
        return;
    }

    switch (appState.meshGenerator) {
        case MeshGenerator::BuiltIn: {
            if (appState.testFunc == TestFunc::UserInput) {
                tryGetUserFunction();
            } else {
                populateFunctionMeshes();
            }
            break;
        }
#ifdef BUILD_GMSH
        case MeshGenerator::Gmsh: {
            populateFunctionMeshes();
            break;
        }
#endif
        default: {
            throw std::runtime_error("Invalid mesh generator in handleMeshGeneratorChange.");
        }
    }
}

void Application::handleUserInput() {
    UserGuiInput userInput = appState.takerUserGuiInput();
    if (appState.testFunc == TestFunc::UserInput && userInput.enterPressed && meshBuilder == std::nullopt) {
        appState.trimFunctionInput();
        appState.functionInputCursorReset = true;
        tryGetUserFunction();
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

    ImGui::BeginDisabled(backgroundInProgress());
    {
#ifdef INCLUDE_EXTERNAL_BACKENDS
        static int selectedMeshGenIndex = appState.meshGeneratorIndex();
        if (ImGui::Combo("Mesh generator", &selectedMeshGenIndex, generatorNames.data(), generatorNames.size())) {
            appState.meshGenerator = static_cast<MeshGenerator>(selectedMeshGenIndex);
            handleMeshGeneratorChange();
        }
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
#endif

        static int selectedFuncIndex = appState.selectedFuncIndex();
        if (ImGui::Combo("Choose function", &selectedFuncIndex, funcNames.data(), funcNames.size())) {
            appState.testFunc = static_cast<TestFunc>(selectedFuncIndex);
            populateFunctionMeshes();
        }
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
    }
    ImGui::EndDisabled();

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

    ImGui::Begin("Function y = f(u, v).");

    auto scrollToEnd = [](ImGuiInputTextCallbackData *data) -> int {
        AppState *pState = (AppState *)data->UserData;
        if (pState->functionInputCursorReset) {
            data->BufTextLen = pState->textBufferLen();
            AppState::trimBuffer(data->Buf, std::strlen(data->Buf));
            data->CursorPos                  = data->BufTextLen;
            data->SelectionStart             = data->BufTextLen;
            data->SelectionEnd               = data->BufTextLen;
            pState->functionInputCursorReset = false;
        }
        return 0;
    };

    ImGui::Text("Enter f(u, v):");
    ImGui::InputTextMultiline("user-func", appState.functionInputBuffer.data(), //
                              appState.functionInputBuffer.size(), ImVec2(-1.0f, 50.0f),
                              ImGuiInputTextFlags_CallbackAlways, scrollToEnd, &appState);

    if (appState.functionParseError) {
        ImGui::Text("Expression is invalid, please update.");
    } else if (backgroundInProgress()) {
        ImGui::Text("Building function mesh...");
    } else {
        ImGui::Text("Press enter to apply.");
    }

    ImGui::End();
}

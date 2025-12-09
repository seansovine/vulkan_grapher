#include "application.h"

#include "function_mesh.h"
#include "vertex.h"
#include "vulkan_wrapper.h"

#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>
#include <vulkan/vulkan_core.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

// GLFW callbacks.

void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto app = static_cast<Application *>(glfwGetWindowUserPointer(window));
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
    std::cout << "Cleaning up" << std::endl;

    // Cleanup DearImGui.
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup Vulkan.
    vulkan.deinit();

    // Cleanup GLFW.
    glfwDestroyWindow(window);
    glfwTerminate();
}

// Initialization methods.

void Application::initUI() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    createUIDescriptorPool();
    createUIRenderPass();
    createUICommandPool(&imGuiVulkan.uiCommandPool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    createUICommandBuffers();
    createUIFramebuffers();

    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = vulkan.imGuiInitInfo(imGuiVulkan.uiDescriptorPool, imGuiVulkan.uiRenderPass);
    ImGui_ImplVulkan_Init(&init_info);

    // Set ui callbacks on Vulkan wrapper.
    vulkan.setUIDeinitCallback([this](VkDevice logicalDevice) {
        imGuiVulkan.deinit(logicalDevice); //
    });
    vulkan.setUIDrawCallback(
        [this](uint32_t currentFrame, uint32_t imageIndex, const VkExtent2D &swapchainExtent) -> VkCommandBuffer {
            return imGuiVulkan.recordDrawCommands(currentFrame, imageIndex, swapchainExtent);
        });
    vulkan.setCreateUIFrameBuffersCallback(
        [this](GlfwVulkanWrapper &inVulkan) { this->imGuiVulkan.createFrameBuffers(inVulkan); });
    vulkan.setDestroyUIFrameBuffersCallback(
        [this](GlfwVulkanWrapper &inVulkan) { this->imGuiVulkan.destroyFrameBuffers(inVulkan); });
}

void Application::initVulkan() {
    static constexpr bool RENDER_FUNCTION = true;

    std::vector<IndexedMesh> meshesToRender;
    if (RENDER_FUNCTION) {
        static auto TEST_FUNCTION_PARABOLIC = [](double x, double y) -> double {
            return 1.0 - (x - 0.5) * (x - 0.5) - (y - 0.5) * (y - 0.5);
        };
        static auto sinc = [](double x, double y) -> double {
            double scale = 50; // 100
            double mag = scale * std::sqrt(x * x + y * y);
            return mag == 0.0 ? 1.0 : std::sin(mag) / mag;
        };
        static auto TEST_FUNCTION_SHIFTED_SINC = [](double x, double y) -> double {
            return 0.75 * sinc(x - 0.5, y - 0.5) + 0.25; //
        };

        FunctionMesh mesh{TEST_FUNCTION_SHIFTED_SINC};
        std::cout << "Building function mesh." << std::endl;
        std::cout << " - # function mesh vertices: " << std::to_string(mesh.functionVertices().size()) << std::endl;
        std::cout << " - # function mesh indices:  " << std::to_string(mesh.meshIndices().size()) << std::endl;
        meshesToRender = {IndexedMesh{mesh.functionVertices(), mesh.meshIndices()},
                          IndexedMesh{mesh.floorVertices(), mesh.meshIndices()}};

    } else {
        meshesToRender = {IndexedMesh{TEST_VERTICES_1, TEST_INDICES}, IndexedMesh{TEST_VERTICES_2, TEST_INDICES}};
    }

    vulkan.init(window, INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT, std::move(meshesToRender));
}

void Application::initWindow() {
    if (!glfwInit()) {
        throw std::runtime_error("Unable to initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT, "Vulkan Grapher", nullptr, nullptr);
    glfwSetKeyCallback(window, keyCallback);

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
        drawFrame();
    }

    vulkan.waitForDeviceIdle();
}

void Application::drawUI() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static float f = 0.0f;

    ImGui::Begin("Settings");
    ImGui::Text("Average framerate: %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);

    // Add some vertical space.
    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    if (ImGui::Button("Toggle Object Rotation")) {
        appState.rotating = !appState.rotating;
    }

    ImGui::End();
    ImGui::Render();
}

void Application::toggleMesh() {
    // TODO
}

void Application::drawFrame() {
    vulkan.drawFrame(appState, framebufferResized);
    framebufferResized = false;
}

// Vulkan and ImGui helpers.

void Application::cleanupUIResources() {
    for (auto framebuffer : imGuiVulkan.uiFramebuffers) {
        vkDestroyFramebuffer(vulkan.getLogicalDevice(), framebuffer, nullptr);
    }

    vkFreeCommandBuffers(vulkan.getLogicalDevice(), imGuiVulkan.uiCommandPool,
                         static_cast<uint32_t>(imGuiVulkan.uiCommandBuffers.size()),
                         imGuiVulkan.uiCommandBuffers.data());
}

void Application::createUICommandBuffers() {
    imGuiVulkan.uiCommandBuffers.resize(vulkan.getSwapchainInfo().swapchainImageViews.size());

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = imGuiVulkan.uiCommandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(imGuiVulkan.uiCommandBuffers.size());

    if (vkAllocateCommandBuffers(vulkan.getLogicalDevice(), &commandBufferAllocateInfo,
                                 imGuiVulkan.uiCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Unable to allocate UI command buffers!");
    }
}

void Application::createUICommandPool(VkCommandPool *cmdPool, VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = vulkan.getQueueIndices().graphicsFamilyIndex;
    commandPoolCreateInfo.flags = flags;

    if (vkCreateCommandPool(vulkan.getLogicalDevice(), &commandPoolCreateInfo, nullptr, cmdPool) != VK_SUCCESS) {
        throw std::runtime_error("Could not create graphics command pool!");
    }
}

void Application::createUIDescriptorPool() {
    VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                         {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                         {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(vulkan.getLogicalDevice(), &pool_info, nullptr, &imGuiVulkan.uiDescriptorPool) !=
        VK_SUCCESS) {
        throw std::runtime_error("Cannot allocate UI descriptor pool!");
    }
}

void Application::createUIFramebuffers() {
    imGuiVulkan.createFrameBuffers(vulkan);
}

void Application::createUIRenderPass() {
    VkAttachmentDescription attachmentDescription = {};
    attachmentDescription.format = vulkan.getSwapchainInfo().swapChainImageFormat;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    // UI is drawn on top of existing image.
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // After the UI pass the images should be ready to present.
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    VkAttachmentReference attachmentReference = {};
    attachmentReference.attachment = 0;
    attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachmentReference;

    // Make the UI render pass dependent on the main renderpass.
    // The docs say about VK_SUBPASS_EXTERNAL:
    //
    //   If srcSubpass is equal to VK_SUBPASS_EXTERNAL, the first synchronization
    //   scope includes commands that occur earlier in submission order than the
    //   vkCmdBeginRenderPass used to begin the render pass instance.
    //
    // https://docs.vulkan.org/spec/latest/chapters/renderpass.html#VkSubpassDependency

    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependency.dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &attachmentDescription;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &subpassDependency;

    if (vkCreateRenderPass(vulkan.getLogicalDevice(), &renderPassInfo, nullptr, &imGuiVulkan.uiRenderPass) !=
        VK_SUCCESS) {
        throw std::runtime_error("Unable to create UI render pass!");
    }
}

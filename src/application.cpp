#include "application.h"

#include "vertex.h"

#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>

#include <iostream>

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

Application::Application()
    : currentTestVertexSet(TestVertexSet::TEST_VERTICES_1),
      vertexData{TEST_VERTICES_1} {
    initWindow();
    initVulkan();
    initUI();
}

Application::~Application() {
    std::cout << "Cleaning up" << std::endl;

    // Cleanup DearImGui
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vulkan.deinit(imGuiVulkan);

    // Cleanup GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
}

// Initialization methods.

void Application::initUI() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // Initialize some DearImgui specific resources
    createUIDescriptorPool();
    createUIRenderPass();
    createUICommandPool(&imGuiVulkan.uiCommandPool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    createUICommandBuffers();
    createUIFramebuffers();

    // Provide bind points from Vulkan API
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = vulkan.imGuiInitInfo(imGuiVulkan);
    ImGui_ImplVulkan_Init(&init_info);
}

void Application::initVulkan() {
    vulkan.init(window, WINDOW_WIDTH, WINDOW_HEIGHT, vertexData);
}

void Application::initWindow() {
    if (!glfwInit()) {
        throw std::runtime_error("Unable to initialize GLFW!");
    }

    // Do not load OpenGL and make window not resizable
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Renderer", nullptr, nullptr);
    glfwSetKeyCallback(window, keyCallback);

    // Add a pointer that allows GLFW to reference our instance
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Unable to create window!");
    }
}

// If swapchain is invalidated, as during window resize, recreate it.
void Application::recreateSwapchain() {
    vulkan.recreateSwapchain(window);
    cleanupUIResources();

    // We also need to take care of the UI.
    ImGui_ImplVulkan_SetMinImageCount(vulkan.getImageCount());
    createUICommandBuffers();
    createUIFramebuffers();
}

// Render loop methods.

void Application::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawUI();
        drawFrame();
    }

    // Wait for unfinished work on GPU before ending application.
    vulkan.waitForDeviceIdle();
}

void Application::drawUI() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static float f = 0.0f;

    ImGui::Begin("Renderer Options");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    // Add some vertical space.
    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    if (ImGui::Button("Toggle Vertex Colors")) {
        toggleMesh();
    }

    ImGui::End();
    ImGui::Render();
}

void Application::toggleMesh() {
    if (currentTestVertexSet == TestVertexSet::TEST_VERTICES_1) {
        currentTestVertexSet = TestVertexSet::TEST_VERTICES_2;
        vertexData = TEST_VERTICES_2;
    } else {
        currentTestVertexSet = TestVertexSet::TEST_VERTICES_1;
        vertexData = TEST_VERTICES_1;
    }
    vulkan.updateMesh(vertexData);
}

void Application::drawFrame() {
    vulkan.drawFrame(window, imGuiVulkan, framebufferResized);
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

void Application::recordUICommands(uint32_t bufferIdx) {
    //
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
    // Create some UI framebuffers. These will be used in the render pass for the UI
    imGuiVulkan.uiFramebuffers.resize(vulkan.getSwapchainInfo().swapchainImages.size());
    VkImageView attachment[1];

    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = imGuiVulkan.uiRenderPass;
    info.attachmentCount = 1;
    info.pAttachments = attachment;
    info.width = vulkan.getSwapchainInfo().swapchainExtent.width;
    info.height = vulkan.getSwapchainInfo().swapchainExtent.height;
    info.layers = 1;

    for (uint32_t i = 0; i < vulkan.getSwapchainInfo().swapchainImages.size(); ++i) {
        attachment[0] = vulkan.getSwapchainInfo().swapchainImageViews[i];
        if (vkCreateFramebuffer(vulkan.getLogicalDevice(), &info, nullptr, &imGuiVulkan.uiFramebuffers[i]) !=
            VK_SUCCESS) {
            throw std::runtime_error("Unable to create UI framebuffers!");
        }
    }
}

void Application::createUIRenderPass() {
    // Create an attachment description for the render pass
    VkAttachmentDescription attachmentDescription = {};
    attachmentDescription.format = vulkan.getSwapchainInfo().swapchainImageFormat;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Need UI to be drawn on top of main
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Last pass so we want to present after
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // Create a color attachment reference
    VkAttachmentReference attachmentReference = {};
    attachmentReference.attachment = 0;
    attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Create a subpass
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachmentReference;

    // Create a subpass dependency to synchronize our main and UI render passes
    // We want to render the UI after the geometry has been written to the framebuffer
    // so we need to configure a subpass dependency as such
    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Create external dependency
    subpassDependency.dstSubpass = 0;                   // The geometry subpass comes first
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Wait on writes
    subpassDependency.dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Finally create the UI render pass
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

#ifndef VERTEX_H_
#define VERTEX_H_

#include "app_state.h"
#include "uniforms.h"
#include "vulkan_objects.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;

    glm::vec3 tangent;
    glm::vec3 bitangent;
    glm::vec3 normal;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding   = 0;
        bindingDescription.stride    = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};

        attributeDescriptions[0].binding  = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset   = offsetof(Vertex, pos);

        attributeDescriptions[1].binding  = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset   = offsetof(Vertex, color);

        attributeDescriptions[2].binding  = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset   = offsetof(Vertex, tangent);

        attributeDescriptions[3].binding  = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset   = offsetof(Vertex, bitangent);

        attributeDescriptions[4].binding  = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[4].offset   = offsetof(Vertex, normal);

        return attributeDescriptions;
    }
};

struct MeshDescriptorSetLayout {
    MeshDescriptorSetLayout() = default;

    void init(VkDevice inDevice) {
        device = inDevice;
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding            = 0;
        uboLayoutBinding.descriptorCount    = 1;
        uboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings    = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    ~MeshDescriptorSetLayout() {
        if (layout != nullptr) {
            std::cerr << "MeshDescriptorSetLayout must be explicitly destroyed." << std::endl;
        }
        assert(layout == nullptr);
    }

    void destroy() {
        assert(device != nullptr);
        vkDestroyDescriptorSetLayout(device, layout, nullptr);
        layout = nullptr;
    }

    VkDevice device              = nullptr;
    VkDescriptorSetLayout layout = nullptr;
};

struct IndexedMesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    uint32_t numIndices;

    UniformInfo uniformInfo;

    VkDescriptorPool descriptorPool;
    MeshDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;

    void createDescriptorSetLayout(VkDevice device) {
        descriptorSetLayout.init(device);
    }

    void createDescriptorPool(VkDevice device, uint32_t numDescriptorSets) {
        VkDescriptorPoolSize poolSize = {};
        poolSize.type                 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount      = numDescriptorSets;

        VkDescriptorPoolCreateInfo createInfo = {};
        createInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.maxSets                    = numDescriptorSets;
        createInfo.poolSizeCount              = 1;
        createInfo.pPoolSizes                 = &poolSize;

        if (vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Unable to create descriptor pool!");
        }
    }

    void createDescriptorSets(VkDevice device, uint32_t numDescriptorSets) {
        std::vector<VkDescriptorSetLayout> layouts(numDescriptorSets, descriptorSetLayout.layout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = descriptorPool;
        allocInfo.descriptorSetCount = numDescriptorSets;
        allocInfo.pSetLayouts        = layouts.data();

        descriptorSets.resize(numDescriptorSets);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < numDescriptorSets; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformInfo.uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range  = sizeof(TransformsUniform);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet          = descriptorSets[i];
            descriptorWrite.dstBinding      = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo     = &bufferInfo;

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }
    }

    void updateUniformBuffer(uint32_t currentImage, const AppState &appState, float aspectRatio, glm::vec3 color) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        static float lastTime = 0.0f;

        float time;
        if (appState.rotating) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            time     = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
            lastTime = time;
        } else {
            time = lastTime;
        }

        static constexpr float ROT_RADS_PER_SEC = 22.5f;
        static constexpr float DIST_COMP        = 1.5f;
        static constexpr glm::vec3 VIEWER_POS   = glm::vec3(0.0f, DIST_COMP, DIST_COMP);

        TransformsUniform ubo{};

        // Update MVP matrices.
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(ROT_RADS_PER_SEC), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.model = glm::translate(ubo.model, glm::vec3{-0.5f, -0.25f, -0.5f});
        ubo.view  = glm::lookAt(VIEWER_POS, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.proj  = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);

        // Correct orientation.
        ubo.proj[1][1] *= -1;

        // Update mesh color.
        ubo.meshColor = color;
        // Set viewer position; constant for now.
        ubo.viewerPos = VIEWER_POS;

        ubo.metallic  = appState.metallic;
        ubo.roughness = appState.roughness;

        memcpy(uniformInfo.uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    void destroyBuffers(VkDevice device) {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);

        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);
    }

    void destroyResources(VkDevice device) {
        uniformInfo.destroy(device);
        destroyBuffers(device);

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorSetLayout.destroy();
    }
};

// Test data for development.

enum class TestVertexSet {
    TEST_VERTICES_1,
    TEST_VERTICES_2,
};

// clang-format off
static const std::vector<Vertex> TEST_VERTICES_1 = {
    Vertex{{ 0.0f, -0.5f, -0.25f}, {1.0f, 0.0f, 0.0f}}, //
    Vertex{{ 0.5f,  0.5f, -0.25f}, {0.0f, 1.0f, 0.0f}}, //
    Vertex{{-0.5f,  0.5f, -0.25f}, {0.0f, 0.0f, 1.0f}}  //
};

static const std::vector<Vertex> TEST_VERTICES_2 = {
    Vertex{{ 0.0f, -0.5f, 0.25f}, {1.0f, 1.0f, 1.0f}}, //
    Vertex{{ 0.5f,  0.5f, 0.25f}, {0.0f, 1.0f, 0.0f}}, //
    Vertex{{-0.5f,  0.5f, 0.25f}, {0.0f, 0.0f, 1.0f}}  //
};
// clang-format on

static const std::vector<uint32_t> TEST_INDICES = {
    0, 1, 2 //
};

#endif

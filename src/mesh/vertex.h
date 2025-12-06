#ifndef VERTEX_H_
#define VERTEX_H_

#include "uniforms.h"
#include "vulkan_objects.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <vector>
#include <vulkan/vulkan_core.h>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

struct IndexedMesh {
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    uint32_t numIndices;

    UniformInfo uniformInfo;

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;

    void updateUniformBuffer(uint32_t currentImage, bool rotating, float aspectRatio) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        static float lastTime = 0.0f;

        float time;
        if (rotating) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
            lastTime = time;
        } else {
            time = lastTime;
        }

        static constexpr float ROTATION_RADS_PER_SEC = 22.5f;
        static constexpr float DIST_COMP = 1.5f;

        TransformsUniform ubo{};
        ubo.model =
            glm::rotate(glm::mat4(1.0f), time * glm::radians(ROTATION_RADS_PER_SEC), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.model = glm::translate(ubo.model, glm::vec3{-0.5f, -0.5f, -0.5f});
        ubo.view = glm::lookAt(glm::vec3(DIST_COMP, DIST_COMP, DIST_COMP), glm::vec3(0.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        memcpy(uniformInfo.uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    void destroyResources(VkDevice device) {
        uniformInfo.destroy(device);

        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);

        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
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

static const std::vector<uint16_t> TEST_INDICES = {
    0, 1, 2 //
};

#endif

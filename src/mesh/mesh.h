#ifndef VERTEX_H_
#define VERTEX_H_

#include "app_state.h"
#include "uniforms.h"
#include "vulkan_objects.h"

#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <glm/trigonometric.hpp>
#include <numbers>
#include <vector>

#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;

    glm::vec3 tangent   = {};
    glm::vec3 bitangent = {};
    glm::vec3 normal    = {};

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

class MeshController {
    static constexpr glm::vec3 DEFAULT_MESH_POSITION = {-0.5f, -0.25f, -0.5f};
    static constexpr double ROT_RADS_PER_SEC         = std::numbers::pi / 8.0;
    static constexpr double USER_ROT_SPEED           = 0.0125;
    static constexpr double USER_TRANS_SPEED         = 0.01;

    ModelUniform ubo{};
    glm::vec3 meshPosition = {0.0f, 0.0f, 0.0f};

    bool rotationPaused = true;
    // TODO: To use this it would have to be per image as in SceneInfo.
    [[maybe_unused]] bool needsUniformWrite = true;

    using time_point          = decltype(std::chrono::high_resolution_clock::now());
    time_point lastUpdateTime = std::chrono::high_resolution_clock::now();

public:
    double xRotRad = 0.0;
    double yRotRad = 0.0;

public:
    MeshController() = default;

    ModelUniform &getUbo() {
        return ubo;
    }

    void setPauseRotation(bool pause) {
        if (rotationPaused && !pause) {
            lastUpdateTime = std::chrono::high_resolution_clock::now();
        }
        rotationPaused = pause;
    }

    void updateFromAppState(const AppState &appState) {
        ubo.metallic  = appState.metallic;
        ubo.roughness = appState.roughness;
    }

    void updateColor(glm::vec3 color) {
        ubo.meshColor = color;
    }

    void restartRotation() {
        lastUpdateTime = std::chrono::high_resolution_clock::now();
    }

    void syncYRotation(double inYRotRad) {
        yRotRad = inYRotRad;
    }

    void reset() {
        xRotRad      = 0;
        yRotRad      = 0;
        meshPosition = {0.0f, 0.0f, 0.0f};
        updateMatrix();
    }

    void updateMatrix() {
        // Update model matrix.
        ubo.model = glm::mat4(1.0f);
        ubo.model = glm::translate(ubo.model, meshPosition);
        ubo.model = glm::rotate(ubo.model, static_cast<float>(yRotRad), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.model = glm::rotate(ubo.model, static_cast<float>(-xRotRad), glm::vec3(1.0f, 0.0f, 0.0f));
        ubo.model = glm::translate(ubo.model, DEFAULT_MESH_POSITION);
        // auto inversePosition  = glm::vec3(glm::inverse(ubo.model) * glm::vec4(meshPosition, 0));
    }

    void applyUserRotation(const std::pair<double, double> userRot) {
        if (userRot.first == 0.0 && userRot.second == 0.0) {
            return;
        }
        yRotRad = yRotRad + userRot.first * USER_ROT_SPEED;
        xRotRad = xRotRad + userRot.second * USER_ROT_SPEED;
        updateMatrix();
    }

    void applyUserTranslation(double dx, double dy) {
        if (dx == 0 && dy == 0) {
            return;
        }
        meshPosition.x += dx * USER_TRANS_SPEED;
        meshPosition.y -= dy * USER_TRANS_SPEED;
        updateMatrix();
    }

    void applyTimedRotation() {
        if (rotationPaused) {
            return;
        }

        // Increment rotation angle proportional to time change.
        time_point currentTime = std::chrono::high_resolution_clock::now();
        double time = std::chrono::duration<double, std::chrono::seconds::period>(currentTime - lastUpdateTime).count();
        yRotRad     = yRotRad + time * ROT_RADS_PER_SEC;
        lastUpdateTime = currentTime;
        updateMatrix();
    }
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
    MeshController controller;

    VkDescriptorPool descriptorPool;
    DescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;

public:
    IndexedMesh() = default;

    IndexedMesh(std::vector<Vertex> &&inVertices, std::vector<uint32_t> &&inIndices)
        : vertices{std::forward<std::vector<Vertex>>(inVertices)},
          indices{std::forward<std::vector<uint32_t>>(inIndices)} {
        controller.updateMatrix();
    }

    glm::vec3 &getVertColor() {
        assert(!vertices.empty());
        return vertices[0].color;
    }

public:
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
            bufferInfo.range  = sizeof(ModelUniform);

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

    bool needsUniformBufferWrite() {
        return true;
    }

    void updateUniformBuffer(uint32_t currentImage) {
        memcpy(uniformInfo.uniformBuffersMapped[currentImage], &controller.getUbo(), sizeof(ModelUniform));
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

#endif

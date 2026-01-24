#ifndef UNIFORMS_H_
#define UNIFORMS_H_

#include "vulkan_objects.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cstring>

struct ModelUniform {
    glm::mat4 model;
    // Color of graph surface in PBR pipeline.
    glm::vec3 meshColor = {0.0f, 0.0f, 0.0f};
    // PBR parameters.
    glm::float32 roughness = 0.0;
    glm::float32 metallic  = 0.0;
};

static constexpr float DIST_COMP              = 1.5f;
static constexpr glm::vec3 DEFAULT_VIEWER_POS = glm::vec3(0.0f, DIST_COMP, DIST_COMP);

struct CameraUniform {
    glm::mat4 view;
    glm::mat4 proj;
    // Position of viewer in world coords.
    glm::vec3 viewerPos = {0.0f, 0.0f, 0.0f};
    // To complete alignment of vec3 to 16 bytes.
    glm::uint32_t _paddingBytes1;
};

struct SceneInfo {
    UniformInfo uniformInfo;
    CameraUniform ubo;

    // THis is currently not changed after first write.
    std::vector<bool> needsBufferWrite;

    VkDescriptorPool descriptorPool;
    DescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;

public:
    SceneInfo() {
        // Set viewer position; constant for now.
        ubo.viewerPos = DEFAULT_VIEWER_POS;
    }

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

        // A bit awkward here but it works.
        needsBufferWrite = std::vector<bool>(numDescriptorSets, true);
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
            bufferInfo.range  = sizeof(CameraUniform);

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

    bool needsUniformBufferWrite(uint32_t currentImage) {
        return needsBufferWrite[currentImage];
    }

    void updateUniformBuffer(uint32_t currentImage, float aspectRatio) {
        ubo.view = glm::lookAt(DEFAULT_VIEWER_POS, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);
        // Correct orientation.
        ubo.proj[1][1] *= -1;

        memcpy(uniformInfo.uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
        needsBufferWrite[currentImage] = false;
    }

    void destroyResources(VkDevice device) {
        uniformInfo.destroy(device);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorSetLayout.destroy();
    }
};

#endif // UNIFORMS_H_

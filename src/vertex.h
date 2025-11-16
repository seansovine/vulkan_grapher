#ifndef VERTEX_H_
#define VERTEX_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <array>
#include <vector>

struct Vertex {
    glm::vec2 pos;
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
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

// clang-format off
static const std::vector<Vertex> TEST_VERTICES_1 = {
    {{ 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, //
    {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},  //
    {{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}  //
};

static const std::vector<Vertex> TEST_VERTICES_2 = {
    {{ 0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}}, //
    {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},  //
    {{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}  //
};
// clang-format on

#endif

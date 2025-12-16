#ifndef UNIFORMS_H_
#define UNIFORMS_H_

#include <glm/fwd.hpp>
#include <glm/glm.hpp>

struct TransformsUniform {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;

    // Position of viewer in world coords.
    glm::vec3 viewerPos = {0.0f, 0.0f, 0.0f};
    // To complete alignment of vec3 to 16 bytes.
    glm::uint32_t _paddingBytes1;

    // Color of graph surface in PBR pipeline.
    glm::vec3 meshColor = {0.0f, 0.0f, 0.0f};

    // PBR parameters.
    glm::float32 roughness = 0.0;
    glm::float32 metallic  = 0.0;
};

#endif // UNIFORMS_H_

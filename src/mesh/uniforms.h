#ifndef UNIFORMS_H_
#define UNIFORMS_H_

#include <glm/fwd.hpp>
#include <glm/glm.hpp>

struct TransformsUniform {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;

    // Color of graph surface in PBR pipeline.
    glm::vec3 meshColor = {0.0f, 0.0f, 0.0f};
};

#endif // UNIFORMS_H_

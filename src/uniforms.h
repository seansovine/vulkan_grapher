#ifndef UNIFORMS_H_
#define UNIFORMS_H_

#include <glm/glm.hpp>

struct TransformsUniform {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

#endif // UNIFORMS_H_

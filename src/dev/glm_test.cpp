#include <format>
#include <iostream>

#include <glm/fwd.hpp>
#include <glm/geometric.hpp>

static std::string debugGlmVec(glm::dvec3 vec) {
    return std::format("({:.6f}, {:.6f}, {:.6f})", vec.x, vec.y, vec.z);
};

int main() {
    glm::dvec3 x(1.0, 0.0, 0.0);
    glm::dvec3 y(1.0, 1.0, 0.0);

    glm::dvec3 cp = glm::cross(x, y);
    std::cout << std::format("Result: {}\n", debugGlmVec(cp));

    // TODO: Use this to try out improved normal computation.
}

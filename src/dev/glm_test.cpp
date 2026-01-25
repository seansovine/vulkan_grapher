#include <format>
#include <iostream>

#include <glm/fwd.hpp>
#include <glm/geometric.hpp>

[[maybe_unused]]
static std::string debugGlmVecTrunc(glm::dvec3 vec) {
    return std::format("({:.6f}, {:.6f}, {:.6f})", vec.x, vec.y, vec.z);
};

static std::string debugGlmVec(glm::dvec3 vec) {
    return std::format("({}, {}, {})", vec.x, vec.y, vec.z);
};

int main() {
    glm::dvec3 tx(0.3152908477409399, 0.9489950902564246, 0);
    std::cout << std::format("Tx length: {}\n", glm::length(tx));

    glm::dvec3 tz(0, -0.961188655455929, 0.2758919509931805);
    std::cout << std::format("Tz length: {}\n\n", glm::length(tz));

    glm::dvec3 normal = glm::cross(tz, tx);
    std::cout << std::format("Computed normal: {}\n", debugGlmVec(normal));

    double normalLen = glm::length(normal);
    std::cout << std::format("Normal length: {}\n\n", normalLen);

    // Sanity check.

    double cx = tx.z * tz.y - tx.y * tz.z;
    double cy = tx.x * tz.z - tx.z * tz.x;
    double cz = tx.y * tz.x - tx.x * tz.y;

    std::cout << std::format("Manually computed normal: {}\n\n", debugGlmVec(glm::dvec3(cx, cy, cz)));

    double cx1 = tx.z * tz.y;
    double cx2 = tx.y * tz.z;
    double cy1 = tx.x * tz.z;
    double cy2 = tx.z * tz.x;
    double cz1 = tx.y * tz.x;
    double cz2 = tx.x * tz.y;

    cx = cx1 - cx2;
    cy = cy1 - cy2;
    cz = cz1 - cz2;

    std::cout << "Computation is:\n";
    std::cout << std::format("- cx: {} - {}\n", cx1, cx2);
    std::cout << std::format("- cy: {} - {}\n", cy1, cy2);
    std::cout << std::format("- cz: {} - {}\n", cz1, cz2);
    std::cout << std::format("Normal computed in steps: {}\n\n", debugGlmVec(glm::dvec3(cx, cy, cz)));
}

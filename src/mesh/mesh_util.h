#ifndef MESH_UTIL_H_
#define MESH_UTIL_H_

#include "mesh.h"
#include "util.h"

#include <spdlog/spdlog.h>

#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>

struct Triangle {
    uint32_t vert1Idx = UINT32_MAX;
    uint32_t vert2Idx = UINT32_MAX;
    uint32_t vert3Idx = UINT32_MAX;

    // Normal vector in world coordinates.
    glm::dvec3 normal = {};
    double area       = 0.0;
};

namespace mesh_util {

inline void assignTriangleNormalArea(Triangle &tri, const std::vector<Vertex> &verts) {
    glm::dvec3 vert1 = verts[tri.vert1Idx].pos;
    glm::dvec3 vert2 = verts[tri.vert2Idx].pos;
    glm::dvec3 vert3 = verts[tri.vert3Idx].pos;
    tri.normal       = glm::normalize(glm::cross(vert2 - vert1, vert3 - vert1));
    assert(std::isnormal(tri.normal.x) && std::isnormal(tri.normal.y) && std::isnormal(tri.normal.z));

    double len1 = glm::length(vert1 - vert2);
    double len2 = glm::length(vert2 - vert3);
    double len3 = glm::length(vert3 - vert1);
    tri.area    = math_util::triangleArea(len1, len2, len3);
}

inline constexpr double H       = 10e-6;
inline constexpr bool DEBUG_TBN = false;

template <typename... Args>
void maybeDebug(spdlog::format_string_t<Args...> fmt, Args &&...args) {
    if constexpr (DEBUG_TBN) {
        spdlog::debug(fmt, std::forward<Args>(args)...);
    }
}

// Cross products are computed and basis vectors ordered relative to the Gmsh coord system.
inline void assignVertTBNGmsh(Vertex &vert, const std::vector<uint32_t> &vertTriInds,
                              const std::vector<Triangle> &tris) {
    constexpr glm::dvec3 xDir = {1.0f, 0.0f, 0.0f};
    constexpr glm::dvec3 zDir = {0.0f, 0.0f, 1.0f};

    maybeDebug("Computing vertex normal:");
    glm::dvec3 normal = {0.0f, 0.0f, 0.0f};
    for (uint32_t vertTriIdx : vertTriInds) {
        const Triangle &vertTri = tris[vertTriIdx];
        glm::dvec3 triDVec      = glm::dvec3(vertTri.normal);
        maybeDebug(" - Averaging tri normal: {}", math_util::debugGlmVecTrunc(triDVec));
        normal += vertTri.area * triDVec;
    }
    normal = -1.0 * glm::normalize(normal);

    // Use Gram-Schmidt to get ONB. Ordered to make Gmsh-derived right-handed.
    glm::dvec3 tangent   = glm::normalize(xDir - glm::dot(xDir, normal) * normal);
    glm::dvec3 bitangent = glm::normalize(zDir - glm::dot(zDir, normal) * normal - glm::dot(zDir, tangent) * tangent);

    maybeDebug(" :: Computed normal: {}", math_util::debugGlmVecTrunc(normal));
    maybeDebug(" :: Computed tangent: {}", math_util::debugGlmVecTrunc(tangent));
    maybeDebug(" :: Computed bitangent: {}", math_util::debugGlmVecTrunc(bitangent));

    // Verify orientation and orthonormality.
    constexpr float TRIPLE_ERROR_TOLERANCE = 0.01f;
    float scalarTriple                     = glm::dot(normal, glm::cross(bitangent, tangent));
    if (std::abs(scalarTriple - 1.0f) >= TRIPLE_ERROR_TOLERANCE) {
        std::cout << "TBN scalar triple product: " << std::to_string(scalarTriple) << std::endl;
    }
    assert(std::abs(scalarTriple - 1.0f) < TRIPLE_ERROR_TOLERANCE);

    vert.tangent   = glm::vec3(tangent);
    vert.bitangent = glm::vec3(bitangent);
    vert.normal    = glm::vec3(normal);
}

} // namespace mesh_util

#endif // MESH_UTIL_H_

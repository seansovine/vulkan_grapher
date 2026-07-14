#ifndef MESH_UTIL_H_
#define MESH_UTIL_H_

#include "mesh.h"
#include "util.h"

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

    double len1 = glm::length(vert1 - vert2);
    double len2 = glm::length(vert2 - vert3);
    double len3 = glm::length(vert3 - vert1);
    tri.area    = math_util::triangleArea(len1, len2, len3);
}

} // namespace mesh_util

#endif // MESH_UTIL_H_

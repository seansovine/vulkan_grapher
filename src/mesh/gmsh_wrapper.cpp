#include "gmsh_wrapper.h"

#ifdef BUILD_GMSH

#include "mesh.h"
#include "mesh_util.h"

#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <gmsh.h>
#include <spdlog/spdlog.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <gmsh.h>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gmsh_wrapper {

struct MeshDomain2D {
    double u_min = 0.0;
    double u_max = 1.0;
    double v_min = 0.0;
    double v_max = 1.0;
};

static constexpr auto DEFAULT_DOMAIN = MeshDomain2D{};

void buildMesh(const std::string &functionDef, MeshDomain2D domain, double meshSize = 0.2, bool collapseZ = false) {
    gmsh::model::add("parametric_surface");

    double u_min = domain.u_min;
    double u_max = domain.u_max;
    double v_min = domain.v_min;
    double v_max = domain.v_max;

    auto gTag = gmsh::model::geo::addGeometry("ParametricSurface", {}, {"u", "v", functionDef});

    int p1 = gmsh::model::geo::addPointOnGeometry(gTag, u_min, v_min, 0.0, meshSize, 1);
    int p2 = gmsh::model::geo::addPointOnGeometry(gTag, u_max, v_min, 0.0, meshSize, 2);
    int p3 = gmsh::model::geo::addPointOnGeometry(gTag, u_max, v_max, 0.0, meshSize, 3);
    int p4 = gmsh::model::geo::addPointOnGeometry(gTag, u_min, v_max, 0.0, meshSize, 4);

    int l1 = gmsh::model::geo::addLine(p1, p2);
    int l2 = gmsh::model::geo::addLine(p2, p3);
    int l3 = gmsh::model::geo::addLine(p3, p4);
    int l4 = gmsh::model::geo::addLine(p4, p1);

    int loop                      = gmsh::model::geo::addCurveLoop({l1, l2, l3, l4});
    [[maybe_unused]] int _surface = gmsh::model::geo::addPlaneSurface({loop});

    gmsh::model::geo::synchronize();
    gmsh::model::mesh::generate(2);

    if (collapseZ) {
        std::vector<std::size_t> nodeTags;
        std::vector<double> coords;
        std::vector<double> parametricCoords;

        gmsh::model::mesh::getNodes(nodeTags, coords, parametricCoords);

        spdlog::trace("Num node tags:    {}", std::size(nodeTags));
        spdlog::trace("Num coords:       {}", std::size(coords));
        spdlog::trace("Num param.coords: {}", std::size(parametricCoords));

        for (std::size_t node_i = 0; node_i < std::size(nodeTags); ++node_i) {
            gmsh::model::mesh::setNode(nodeTags[node_i], {coords[3 * node_i], coords[3 * node_i + 1], 0.0}, {});
        }
    }

    if constexpr (false) {
        gmsh::write("parametric_surface.msh");
    }
}

VertsAndIndices gmshToIndexed() {
    std::vector<std::size_t> nodeTags;
    std::vector<double> coords;
    std::vector<double> parametricCoords;

    gmsh::model::mesh::getNodes(nodeTags, coords, parametricCoords);

    spdlog::trace("Num node tags:             {}", std::size(nodeTags));
    spdlog::trace("Num coords:                {}", std::size(coords));

    VertsAndIndices indexedMesh;
    indexedMesh.vertices.reserve(nodeTags.size());
    std::unordered_map<std::size_t, uint32_t> tagToIndex;

    static constexpr glm::vec3 FUNCT_COLOR = {0.070f, 0.336f, 0.594f};

    for (size_t i = 0; i < nodeTags.size(); ++i) {
        tagToIndex[nodeTags[i]] = i;

        Vertex v;
        v.pos.x = coords[3 * i + 0];
        v.pos.y = coords[3 * i + 2];
        v.pos.z = coords[3 * i + 1];
        // We use "OpenGL coordinates" where y is up.

        v.color = FUNCT_COLOR;
        indexedMesh.vertices.push_back(v);
    }

    std::vector<int> elementTypes;
    std::vector<std::vector<std::size_t>> elementTags;
    std::vector<std::vector<std::size_t>> elementNodeTags;

    gmsh::model::mesh::getElements(elementTypes, elementTags, elementNodeTags);

    // Maps vertex to set of triangles it is incident to.
    std::unordered_map<uint32_t, std::vector<uint32_t>> vertIndexToTriIndices{};
    vertIndexToTriIndices.reserve(std::size(tagToIndex));
    std::vector<Triangle> triangles{};

    for (std::size_t typeIdx = 0; typeIdx < elementTypes.size(); ++typeIdx) {
        if (elementTypes[typeIdx] != 2) {
            continue;
        }
        const auto &triElementNodeTags = elementNodeTags[typeIdx];
        assert(std::size(triElementNodeTags) % 3 == 0);
        uint32_t numTris = std::size(triElementNodeTags) / 3;
        triangles.reserve(numTris);

        spdlog::trace("Num tri element tags:      {}", std::size(elementTags[typeIdx]));
        spdlog::trace("Num tri element node tags: {}", std::size(triElementNodeTags));
        for (uint32_t triIndex = 0; triIndex < numTris; ++triIndex) {
            uint32_t vert1Index = tagToIndex[triElementNodeTags[3 * triIndex + 0]];
            uint32_t vert2Index = tagToIndex[triElementNodeTags[3 * triIndex + 1]];
            uint32_t vert3Index = tagToIndex[triElementNodeTags[3 * triIndex + 2]];
            vertIndexToTriIndices[vert1Index].push_back(triIndex);
            vertIndexToTriIndices[vert2Index].push_back(triIndex);
            vertIndexToTriIndices[vert3Index].push_back(triIndex);
            triangles.push_back({vert1Index, vert2Index, vert3Index});
        }
    }
    spdlog::trace("Num triangles:             {}", std::size(triangles));

    // Assign normal and area to triangles and add indices.
    indexedMesh.indices.reserve(3 * std::size(triangles));
    for (auto &tri : triangles) {
        mesh_util::assignTriangleNormalArea(tri, indexedMesh.vertices);

        indexedMesh.indices.push_back(tri.vert1Idx);
        indexedMesh.indices.push_back(tri.vert2Idx);
        indexedMesh.indices.push_back(tri.vert3Idx);
    }

    // Assign TBN vectors to vertices.
    for (uint32_t vert_i = 0; vert_i < std::size(indexedMesh.vertices); ++vert_i) {
        Vertex &vert = indexedMesh.vertices[vert_i];
        mesh_util::assignVertTBNGmsh(vert, vertIndexToTriIndices[vert_i], triangles);
    }
    return indexedMesh;
}

VertsAndIndices runGmsh(const std::string &functionExpr) {
    spdlog::debug("Running Gmsh.");

    gmsh::initialize(0, nullptr, false);

    static constexpr double MESH_SIZE = 0.015;
    static constexpr bool COLLAPSE_Z  = false;

    buildMesh(functionExpr, DEFAULT_DOMAIN, MESH_SIZE, COLLAPSE_Z);

    VertsAndIndices indexedMesh = gmshToIndexed();

    gmsh::finalize();
    return indexedMesh;
}

} // namespace gmsh_wrapper

#else

namespace gmsh_wrapper {

VertsAndIndices runGmsh(const std::string &_) {
    return {};
}

} // namespace gmsh_wrapper

#endif

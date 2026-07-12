#include "gmsh_wrapper.h"

#include "mesh.h"

#include <gmsh.h>
#include <spdlog/spdlog.h>

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

IndexedMesh gmshToIndexed() {
    std::vector<std::size_t> nodeTags;
    std::vector<double> coords;
    std::vector<double> parametricCoords;

    gmsh::model::mesh::getNodes(nodeTags, coords, parametricCoords);

    IndexedMesh indexedMesh;
    indexedMesh.vertices.reserve(nodeTags.size());
    std::unordered_map<std::size_t, uint32_t> tagToIndex;

    for (size_t i = 0; i < nodeTags.size(); ++i) {
        tagToIndex[nodeTags[i]] = i;

        Vertex v;
        v.pos.x = coords[3 * i + 0];
        v.pos.y = coords[3 * i + 1];
        v.pos.z = coords[3 * i + 2];

        indexedMesh.vertices.push_back(v);
    }

    std::vector<int> elementTypes;
    std::vector<std::vector<std::size_t>> elementTags;
    std::vector<std::vector<std::size_t>> elementNodeTags;

    gmsh::model::mesh::getElements(elementTypes, elementTags, elementNodeTags);

    for (std::size_t typeIdx = 0; typeIdx < elementTypes.size(); ++typeIdx) {
        if (elementTypes[typeIdx] != 2) {
            continue;
        }
        for (std::size_t tag : elementNodeTags[typeIdx]) {
            indexedMesh.indices.push_back(tagToIndex[tag]);
        }
    }

    // TODO: We have enough information here to compute TBN vectors for vertices.

    return indexedMesh;
}

void runGmsh(const std::string &functionExpr) {
    spdlog::debug("Running Gmsh.");

    gmsh::initialize(0, nullptr, false);

    static constexpr double MESH_SIZE = 0.02;
    static constexpr bool COLLAPSE_Z  = false;

    buildMesh(functionExpr, DEFAULT_DOMAIN, MESH_SIZE, COLLAPSE_Z);

    IndexedMesh _ = gmshToIndexed();
    // TODO: Convert Gmsh mesh to fully-defined indexed mesh return.

    gmsh::finalize();
}

} // namespace gmsh_wrapper

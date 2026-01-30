#ifndef FUNCTION_MESH_H_
#define FUNCTION_MESH_H_

#include "mesh.h"
#include "util.h"

#include <cstddef>
#include <glm/fwd.hpp>
#include <spdlog/spdlog.h>

#include <cassert>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

// ------------------
// Geometric helpers.

struct Square;

using SharedSquare = std::shared_ptr<Square>;

struct Square {
    float mTopLeft[2];
    float mBtmRight[2];

    // Refinement level of this square.
    uint32_t depth = 0;

    // Neighbors in same level of grid.
    // For sharing vertices via indices.
    SharedSquare northNeighbor = nullptr;
    SharedSquare southNeighbor = nullptr;
    SharedSquare westNeighbor  = nullptr;
    SharedSquare eastNeighbor  = nullptr;

    // Parent square, if this is a refinement.
    SharedSquare parent = nullptr;

    // Vertex indeices of corners.
    // UINT32_MAX means unassigned.
    uint32_t topLeftIdx     = UINT32_MAX;
    uint32_t topRightIdx    = UINT32_MAX;
    uint32_t bottomRightIdx = UINT32_MAX;
    uint32_t bottomLeftIdx  = UINT32_MAX;
    uint32_t centerIdx      = UINT32_MAX;

    // Child squares if this has been refined.
    // Order is: top-left, top-right, bottom-left, bottom-right.
    std::vector<SharedSquare> children = {};

    // Indices of triangles for this square.
    std::vector<uint32_t> triangleIndices = {};

    bool hasChildren() const {
        return !children.empty();
    }

    struct EdgeRefinements {
        std::vector<uint32_t> north = {};
        std::vector<uint32_t> west  = {};
        std::vector<uint32_t> south = {};
        std::vector<uint32_t> east  = {};
    };
    EdgeRefinements edgeRefinements = {};

    const EdgeRefinements &populateRefinements();
};

struct Triangle {
    uint32_t vert1Idx = UINT32_MAX;
    uint32_t vert2Idx = UINT32_MAX;
    uint32_t vert3Idx = UINT32_MAX;

    // Normal vector in world coordinates.
    glm::vec3 normal = {};
    double area      = 0.0;
};

// --------------------
// Function mesh class.

// Builds a mesh for graphing a function z = f(x, y).
// There is some redundancy among the vertices that we
// will eliminate using indexing in a future version.

using FuncXZ = double(double, double);

class FunctionMesh {

    static constexpr bool USE_NEW_MESH     = true;
    static constexpr bool SHOW_REFINEMENT  = true;
    static constexpr bool DEBUG_REFINEMENT = false;
    static constexpr bool DIRECT_NORMALS   = false;

    // Number of subdivisions of x,y axes when creating cells.
    static constexpr int NUM_CELLS = 400;

    // Currently valid values are 0 and 1; we may
    // add code to support deeper refinement later.
    static constexpr uint8_t MAX_REFINEMENT_DEPTH = 1;

    static constexpr double REFINEMENT_THRESHOLD_VARIATION = 0.25;
    static constexpr double REFINEMENT_THRESHOLD_2ND_DERIV = 20.0;

    // Increment for derivative estimates.
    static constexpr double H = 10e-6;

    // For interpolating between normal computation methods.
    static constexpr double SECOND_DERIV_CUTOFF       = 40.0;
    static constexpr double SECOND_DERIV_CUTOFF_WIDTH = 10.0;

    const math_util::LogisticCutoff mSecondDerivCutoff = {SECOND_DERIV_CUTOFF, SECOND_DERIV_CUTOFF_WIDTH};

public:
    FunctionMesh(std::function<FuncXZ> &&func)
        : mFunc(std::forward<std::function<FuncXZ>>(func)) {
        init();
    }

    void init() {
        auto start = std::chrono::high_resolution_clock::now();
        generateMesh();
        auto stop = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        spdlog::debug("Mesh generation time: {} ms.", duration.count());
    }

    void generateMesh() {
        buildFloorMesh();
        spdlog::trace("Finished building floor mesh.");

        if constexpr (USE_NEW_MESH) {
            computeVerticesAndIndices();
        } else {
            computeFloorMeshVertices();
            computeFunctionMeshVertices();
            buildIndicesList();
        }
    }

    std::vector<SharedSquare> &tessellationSquare() {
        return mFloorMeshSquares;
    }

    std::vector<Vertex> &floorVertices() {
        return mFloorMeshVertices;
    }
    std::vector<Vertex> &functionVertices() {
        return mFunctionMeshVertices;
    }

    std::vector<uint32_t> &meshIndices() {
        return mMeshIndices;
    }

    struct VerticesAndIndices {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    static VerticesAndIndices simpleFloorMesh() {
        return VerticesAndIndices{
            .vertices =
                {//
                 Vertex{
                     .pos       = glm::vec3(0.0, 0.0, 0.0),
                     .color     = FLOOR_COLOR,
                     .tangent   = glm::vec3(1.0, 0.0, 0.0),
                     .bitangent = glm::vec3(0.0, 0.0, 1.0),
                     .normal    = glm::vec3(0.0, 1.0, 0.0),
                 },
                 Vertex{
                     .pos       = glm::vec3(1.0, 0.0, 0.0),
                     .color     = FLOOR_COLOR,
                     .tangent   = glm::vec3(1.0, 0.0, 0.0),
                     .bitangent = glm::vec3(0.0, 0.0, 1.0),
                     .normal    = glm::vec3(0.0, 1.0, 0.0),
                 },
                 Vertex{
                     .pos       = glm::vec3(1.0, 0.0, 1.0),
                     .color     = FLOOR_COLOR,
                     .tangent   = glm::vec3(1.0, 0.0, 0.0),
                     .bitangent = glm::vec3(0.0, 0.0, 1.0),
                     .normal    = glm::vec3(0.0, 1.0, 0.0),
                 },
                 Vertex{
                     .pos       = glm::vec3(0.0, 0.0, 1.0),
                     .color     = FLOOR_COLOR,
                     .tangent   = glm::vec3(1.0, 0.0, 0.0),
                     .bitangent = glm::vec3(0.0, 0.0, 1.0),
                     .normal    = glm::vec3(0.0, 1.0, 0.0),
                 }},

            .indices = {0, 2, 1, //
                        0, 3, 2} //
        };
    }

public:
    // Mesh debugging methods.

    std::basic_ostream<char> &debugVertex(std::basic_ostream<char> &debugStrm, uint32_t vertex_i) {
        const Vertex &vertex = mFloorMeshVertices[vertex_i];
        debugStrm << "(" << vertex.pos.x << ", " << vertex.pos.z << ")";
        return debugStrm;
    }

    std::basic_ostream<char> &debugRefinements(std::basic_ostream<char> &debugStrm, const Square &square) {
        std::string indent(square.depth * 4, ' ');
        debugStrm << indent << " = North refinements:";
        for (uint32_t refinmnt : square.edgeRefinements.north) {
            debugStrm << " ";
            debugVertex(debugStrm, refinmnt);
        }
        debugStrm << std::endl;
        debugStrm << indent << " = West refinements:";
        for (uint32_t refinmnt : square.edgeRefinements.west) {
            debugStrm << " ";
            debugVertex(debugStrm, refinmnt);
        }
        debugStrm << std::endl;
        debugStrm << indent << " = South refinements:";
        for (uint32_t refinmnt : square.edgeRefinements.south) {
            debugStrm << " ";
            debugVertex(debugStrm, refinmnt);
        }
        debugStrm << std::endl;
        debugStrm << indent << " = East refinements:";
        for (uint32_t refinmnt : square.edgeRefinements.east) {
            debugStrm << " ";
            debugVertex(debugStrm, refinmnt);
        }

        return debugStrm;
    }

    std::string debugSquareCell(const Square &square, uint32_t &square_i, bool recurse = true) {
        std::stringstream debugStrm;
        std::string indent(square.depth * 4, ' ');

        debugStrm << std::noskipws << indent << "Square " << square_i << std::endl;
        debugStrm << indent << " - depth: " << std::to_string(square.depth) << std::endl;
        debugStrm << indent << " - top left: ";
        debugVertex(debugStrm, square.topLeftIdx) << std::endl;
        debugStrm << indent << " - top right: ";
        debugVertex(debugStrm, square.topRightIdx) << std::endl;
        debugStrm << indent << " - bottom right: ";
        debugVertex(debugStrm, square.bottomRightIdx) << std::endl;
        debugStrm << indent << " - bottom left: ";
        debugVertex(debugStrm, square.bottomLeftIdx) << std::endl;
        debugStrm << indent << " - center: ";
        debugVertex(debugStrm, square.centerIdx) << std::endl;

        if (recurse && square.hasChildren()) {
            debugStrm << indent << " + Children:" << std::endl;
            for (const SharedSquare &child : square.children) {
                debugStrm << debugSquareCell(*child, square_i);
            }
        }
        debugRefinements(debugStrm, square) << std::endl;

        square_i++;
        return debugStrm.str();
    }

    std::string debugMesh() {
        std::stringstream debugStrm;
        uint32_t square_i = 0;
        for (const SharedSquare &square : mFloorMeshSquares) {
            debugStrm << debugSquareCell(*square, square_i);
        }
        return debugStrm.str();
    }

    void debugTriangle(const Triangle &tri) {
        std::cout << "Tri indices: " << std::endl; //
        std::cout << "   " << std::setw(6) << std::to_string(tri.vert1Idx) << " ";
        debugVertex(std::cout, tri.vert1Idx) << std::endl;
        std::cout << "   " << std::setw(6) << std::to_string(tri.vert2Idx) << " ";
        debugVertex(std::cout, tri.vert2Idx) << std::endl;
        std::cout << "   " << std::setw(6) << std::to_string(tri.vert3Idx) << " ";
        debugVertex(std::cout, tri.vert3Idx) << std::endl;
    }

    void logIndices(const Square &square) {
        auto logRefinements = [](const std::vector<uint32_t> &refinements) {
            for (uint32_t i = 0; i < refinements.size() - 1; i++) {
                std::cout << std::to_string(refinements[i]) << ", ";
            }
            std::cout << std::to_string(refinements.back()) << std::endl;
        };

        std::cout << "Square indices:" << std::endl;
        std::cout << " --- Center:  " << std::to_string(square.centerIdx) << std::endl;
        std::cout << " - Top left:  " << std::to_string(square.topLeftIdx) << std::endl;
        std::cout << " - Top right: " << std::to_string(square.topRightIdx) << std::endl;
        std::cout << " - Btm left:  " << std::to_string(square.bottomLeftIdx) << std::endl;
        std::cout << " - Btm right: " << std::to_string(square.bottomRightIdx) << std::endl;

        std::cout << " >> north refinements: ";
        logRefinements(square.edgeRefinements.north);
        std::cout << " >> west refinements:  ";
        logRefinements(square.edgeRefinements.west);
        std::cout << " >> south refinements: ";
        logRefinements(square.edgeRefinements.south);
        std::cout << " >> east refinements:  ";
        logRefinements(square.edgeRefinements.east);
    }

private:
    void computeVerticesAndIndices();

    void syncEdgeRefinements(SharedSquare &square);

    void syncRefmtsHoriz(std::vector<uint32_t> &to, std::vector<uint32_t> &from);

    void syncRefmtsVert(std::vector<uint32_t> &to, std::vector<uint32_t> &from);

    void buildFloorMesh();

    void addFloorMeshVertex(float x, float z);

    void setFuncVertTBNs();
    void setFuncVertTBNsDirect();
    glm::dvec3 normalAtPoint(const glm::vec3 &pos);

    double funcMeshY(uint32_t index) {
        return mFunctionMeshVertices[index].pos.y;
    }

    struct SquareFuncEval {
        double topLeftVal;
        double topRightVal;
        double btmRightVal;
        double btmLeftVal;
        double centerVal;
    };

    SquareFuncEval evalFuncSquare(const Square &square) {
        return {
            .topLeftVal  = funcMeshY(square.topLeftIdx),
            .topRightVal = funcMeshY(square.topRightIdx),
            .btmRightVal = funcMeshY(square.bottomRightIdx),
            .btmLeftVal  = funcMeshY(square.bottomLeftIdx),
            .centerVal   = funcMeshY(square.centerIdx),
        };
    }

    struct XZCoord {
        float x;
        float z;
    };

    XZCoord meshXZ(uint32_t index) {
        return {mFloorMeshVertices[index].pos.x, mFloorMeshVertices[index].pos.z};
    }

    double secondDerivEst(const Square &square);
    double secondDerivEstMax(const glm::vec3 &pos);

    // Precondition: Square vertex indices are valid for function mesh.
    bool shouldRefine(Square &square);

    void refine(SharedSquare square);

    void addSquareTris(const SharedSquare &square);

    void addTriIndices(const Triangle &tri);

    // DEPRECATED: Old method of mesh construction.

    void computeFloorMeshVertices() {
        auto vertices = std::vector<Vertex>{};
        vertices.reserve(mFloorMeshSquares.size() * 6);

        for (const auto &square : mFloorMeshSquares) {
            // First triangle.
            vertices.push_back({glm::vec3{square->mTopLeft[0], 0.0, square->mTopLeft[1]}, FLOOR_COLOR});
            vertices.push_back({glm::vec3{square->mTopLeft[0], 0.0, square->mBtmRight[1]}, FLOOR_COLOR});
            vertices.push_back({glm::vec3{square->mBtmRight[0], 0.0, square->mTopLeft[1]}, FLOOR_COLOR});

            // Second triangle.
            vertices.push_back({glm::vec3{square->mBtmRight[0], 0.0, square->mBtmRight[1]}, FLOOR_COLOR});
            vertices.push_back({glm::vec3{square->mBtmRight[0], 0.0, square->mTopLeft[1]}, FLOOR_COLOR});
            vertices.push_back({glm::vec3{square->mTopLeft[0], 0.0, square->mBtmRight[1]}, FLOOR_COLOR});
        }

        mFloorMeshVertices = std::move(vertices);
    }

    void computeFunctionMeshVertices() {
        auto vertices = std::vector<Vertex>{};
        vertices.reserve(mFloorMeshVertices.size());

        // Now update y-coordinates w/ function values.
        for (std::size_t i = 0; i < mFloorMeshVertices.size(); i++) {
            vertices.push_back({mFloorMeshVertices[i].pos, FUNCT_COLOR});
            Vertex &newVertex = vertices.back();
            newVertex.pos.y   = static_cast<float>(mFunc(newVertex.pos.x, newVertex.pos.z));
        }

        mFunctionMeshVertices = std::move(vertices);
    }

    void buildIndicesList() {
        assert(mFloorMeshVertices.size() % 3 == 0);
        assert(mFloorMeshVertices.size() == mFunctionMeshVertices.size());
        mMeshIndices.reserve(mFloorMeshVertices.size());

        for (uint32_t i = 0; i < mFunctionMeshVertices.size(); i++) {
            mMeshIndices.push_back(i);
        }
    }

private:
    // The function z = mF(x, y) that we will graph.
    std::function<FuncXZ> mFunc = nullptr;
    // Tried to keep the abstraction low, but std::function is just too convenient.

    // Default RGB colors for floor and function meshes.
    static constexpr glm::vec3 FLOOR_COLOR         = {0.556f, 0.367f, 0.076f};
    static constexpr glm::vec3 FUNCT_COLOR         = {0.070f, 0.336f, 0.594f};
    static constexpr glm::vec3 REFINE_DEBUG_COLOR1 = {0.0f, 1.0f, 0.0f};
    static constexpr glm::vec3 REFINE_DEBUG_COLOR2 = {1.0f, 0.5f, 0.0f};

    // Ensure we don't overlow our index type: This check is
    // necessary, but not sufficient, because of mesh refinement.
    static_assert(static_cast<uint64_t>(NUM_CELLS) * static_cast<uint64_t>(NUM_CELLS) <
                  static_cast<uint64_t>(UINT32_MAX));

    // = 1.0 / mNumCells.
    static constexpr double mCellWidth = 1.0 / NUM_CELLS;

    // Squares that make up x,y-plane mesh.
    std::vector<SharedSquare> mFloorMeshSquares = {};
    // IMPORTANT: Pointers to these elements are stored in various places.
    //            So once assigned, it must never reallocate!
    //
    // This means the top-level grid shape is fixed, and that grid squares
    // are the owners of their child squares added during refinement.

    // Vertices of triangular tessellation built from squares.
    std::vector<Vertex> mFloorMeshVertices = {};
    // Tessellation vertices with heights from function values.
    std::vector<Vertex> mFunctionMeshVertices = {};

    // Triangles in the function mesh; also used for floor mesh.
    std::vector<Triangle> mFunctionMeshTriangles = {};
    // Indices of triangles this vertex is incident to; for normal calculations.
    //  -- This is parallel to mFunctionMeshVertices.
    //  -- It contains indexes into mFunctionMeshTriangles.
    std::vector<std::set<uint32_t>> mVertexTriangles = {};

    // For now we assume a simple relationship between floor and function meshes.
    std::vector<uint32_t> mMeshIndices = {};
};

#endif // FUNCTION_MESH_H_

#ifndef FUNCTION_MESH_H_
#define FUNCTION_MESH_H_

#include "vertex.h"

#include <glm/fwd.hpp>

#include <cstdint>
#include <set>

// ------------------
// Geometric helpers.

struct Square {
    float mTopLeft[2];
    float mBtmRight[2];

    // Refinement level of this square.
    uint32_t depth = 0;

    // Neighbors in same level of grid.
    // For sharing vertices via indices.
    Square *northNeighbor = nullptr;
    Square *westNeighbor  = nullptr;

    // TODO: We need to add additional edge vertices/
    // triangles when neighbor squares get refined.

    // Vertex indeices of corners.
    // UINT16_MAX means unassigned.
    uint16_t topLeftIdx     = UINT16_MAX;
    uint16_t topRightIdx    = UINT16_MAX;
    uint16_t bottomRightIdx = UINT16_MAX;
    uint16_t bottomLeftIdx  = UINT16_MAX;
    uint16_t centerIdx      = UINT16_MAX;

    // Child squares if this has been refined.
    std::vector<Square> children;

    // Indices of triangles for this square.
    std::vector<uint16_t> triangleIndices;

    bool hasChildren() const {
        return !children.empty();
    }
};

struct Triangle {
    uint16_t vert1Idx = UINT16_MAX;
    uint16_t vert2Idx = UINT16_MAX;
    uint16_t vert3Idx = UINT16_MAX;

    // Normal vector in world coordinates.
    glm::vec3 normal;
};

// --------------------
// Function mesh class.

// Builds a mesh for graphing a function z = f(x, y).
// There is some redundancy among the vertices that we
// will eliminate using indexing in a future version.

class FunctionMesh {
    using F = double (*)(double, double);

    static constexpr bool USE_NEW_MESH     = true;
    static constexpr bool DEBUG_REFINEMENT = false;

    static constexpr uint8_t MAX_REFINEMENT_DEPTH          = 0;
    static constexpr double REFINEMENT_THRESHOLD_VARIATION = 0.25;
    static constexpr double REFINEMENT_THRESHOLD_2ND_DERIV = 25.0;

public:
    explicit FunctionMesh(const F func)
        : mFunc(func) {
        generateMesh();
    }

    void generateMesh() {
        buildFloorMesh();

        if constexpr (USE_NEW_MESH) {
            computeVerticesAndIndices();
        } else {
            computeFloorMeshVertices();
            computeFunctionMeshVertices();
            buildIndicesList();
        }
    }

    std::vector<Square> &tessellationSquare() {
        return mFloorMeshSquares;
    }

    std::vector<Vertex> &floorVertices() {
        return mFloorMeshVertices;
    }
    std::vector<Vertex> &functionVertices() {
        return mFunctionMeshVertices;
    }

    std::vector<uint16_t> &meshIndices() {
        return mMeshIndices;
    }

    std::basic_ostream<char> &debugVertex(std::basic_ostream<char> &debugStrm, uint32_t vertex_i) {
        const Vertex &vertex = mFloorMeshVertices[vertex_i];
        debugStrm << "(" << vertex.pos.x << ", " << vertex.pos.z << ")";
        return debugStrm;
    }

    std::string debugSquareCell(const Square &square, uint32_t &square_i) {
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
        square_i++;

        if (square.hasChildren()) {
            debugStrm << indent << " + Children:" << std::endl;
            for (const Square &child : square.children) {
                debugStrm << debugSquareCell(child, square_i);
            }
        }

        return debugStrm.str();
    }

    std::string debugMesh() {
        std::stringstream debugStrm;
        uint32_t square_i = 0;
        for (const Square &square : mFloorMeshSquares) {
            debugStrm << debugSquareCell(square, square_i);
        }
        return debugStrm.str();
    }

private:
    void computeVerticesAndIndices();

    void buildFloorMesh();

    void addFloorMeshVertex(float x, float z);

    void setFuncVertTBNs();

    double funcMeshY(uint16_t index) {
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

    XZCoord meshXZ(uint16_t index) {
        return {mFloorMeshVertices[index].pos.x, mFloorMeshVertices[index].pos.z};
    }

    double secondDerivEst(const Square &square, const SquareFuncEval &funcVals);

    // Precondition: Square vertex indices are valid for function mesh.
    bool shouldRefine(Square &square);

    void refine(Square &square);

    void addSquareTris(const Square &square);

    void addTriIndices(const Triangle &tri);

    // DEPRECATED: Old method of mesh construction.

    void computeFloorMeshVertices() {
        auto vertices = std::vector<Vertex>{};
        vertices.reserve(mFloorMeshSquares.size() * 6);

        for (const auto &square : mFloorMeshSquares) {
            // First triangle.
            vertices.push_back({glm::vec3{square.mTopLeft[0], 0.0, square.mTopLeft[1]}, FLOOR_COLOR});
            vertices.push_back({glm::vec3{square.mTopLeft[0], 0.0, square.mBtmRight[1]}, FLOOR_COLOR});
            vertices.push_back({glm::vec3{square.mBtmRight[0], 0.0, square.mTopLeft[1]}, FLOOR_COLOR});

            // Second triangle.
            vertices.push_back({glm::vec3{square.mBtmRight[0], 0.0, square.mBtmRight[1]}, FLOOR_COLOR});
            vertices.push_back({glm::vec3{square.mBtmRight[0], 0.0, square.mTopLeft[1]}, FLOOR_COLOR});
            vertices.push_back({glm::vec3{square.mTopLeft[0], 0.0, square.mBtmRight[1]}, FLOOR_COLOR});
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

        for (uint16_t i = 0; i < mFunctionMeshVertices.size(); i++) {
            mMeshIndices.push_back(i);
        }
    }

private:
    // The function z = mF(x, y) that we will graph.
    F mFunc;

    // Default RGB colors for floor and function meshes.
    static constexpr glm::vec3 FLOOR_COLOR        = {0.556f, 0.367f, 0.076f};
    static constexpr glm::vec3 FUNCT_COLOR        = {0.070f, 0.336f, 0.594f};
    static constexpr glm::vec3 REFINE_DEBUG_COLOR = {0.0f, 1.0f, 0.0f};

    // Number of subdivisions of x,y axes when creating cells.
    static constexpr int mNumCells = 50;

    // = 1.0 / mNumCells.
    static constexpr double mCellWidth = 1.0 / mNumCells;

    // Squares that make up x,y-plane mesh.
    std::vector<Square> mFloorMeshSquares = {};
    // Vertices of triangular tessellation built from squares.
    std::vector<Vertex> mFloorMeshVertices = {};
    // Tessellation vertices with heights from function values.
    std::vector<Vertex> mFunctionMeshVertices = {};

    // Triangles in the function mesh; also used for floor mesh.
    std::vector<Triangle> mFunctionMeshTriangles = {};
    // Indices of triangles this vertex is incident to; for normal calculations.
    std::vector<std::set<uint16_t>> mVertexTriangles = {};

    // For now we assume a simple relationship between floor and function meshes.
    std::vector<uint16_t> mMeshIndices = {};
};

#endif // FUNCTION_MESH_H_

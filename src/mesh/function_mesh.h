#ifndef FUNCTION_MESH_H_
#define FUNCTION_MESH_H_

#include "vertex.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <glm/fwd.hpp>
#include <vector>

// ------------------
// Geometric helpers.

struct Square {
    float mTopLeft[2];
    float mBtmRight[2];
};

// --------------------
// Function mesh class.

// Builds a mesh for graphing a function z = f(x, y).
// There is some redundancy among the vertices that we
// will eliminate using indexing in a future version.

class FunctionMesh {
    using F = double (*)(double, double);

public:
    explicit FunctionMesh(const F func)
        : mFunc(func) {
        generateMesh();
    }

    void generateMesh() {
        buildFloorMesh();
        computeFloorMeshVertices();
        computeFunctionMeshVertices();
        buildIndicesList();
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

private:
    void buildFloorMesh() {
        mFloorMeshSquares.reserve(mNumCells * mNumCells);
        const double width = 1.0 / mNumCells;

        for (int i = 1; i <= mNumCells; i++) {
            for (int j = 1; j <= mNumCells; j++) {
                // clang-format off
                Square square{
                  .mTopLeft ={
                    static_cast<float>((i - 1) * width),
                    static_cast<float>((j - 1) * width),
                  },
                  .mBtmRight = {
                    static_cast<float>(i * width),
                    static_cast<float>(j * width),
                  }
                };
                // clang-format on
                mFloorMeshSquares.push_back(square);
            }
        }
    }

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
            newVertex.pos.y = static_cast<float>(mFunc(newVertex.pos.x, newVertex.pos.z));
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
    static constexpr glm::vec3 FLOOR_COLOR = {0.556f, 0.367f, 0.076f};
    static constexpr glm::vec3 FUNCT_COLOR = {1.0f, 0.0f, 0.0f};

    // Number of subdivisions of x,y axes when creating cells.
    static constexpr int mNumCells = 100;

    // Squares that make up x,y-plane mesh.
    std::vector<Square> mFloorMeshSquares = {};
    // Vertices of triangular tessellation built from squares.
    std::vector<Vertex> mFloorMeshVertices = {};
    // Tessellation vertices with heights from function values.
    std::vector<Vertex> mFunctionMeshVertices = {};

    // For now we assume a simple relationship between floor and function meshes.
    std::vector<uint16_t> mMeshIndices = {};
};

#endif // FUNCTION_MESH_H_

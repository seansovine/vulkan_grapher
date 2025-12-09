#ifndef FUNCTION_MESH_H_
#define FUNCTION_MESH_H_

#include "vertex.h"

#include <glm/fwd.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <sstream>
#include <vector>

// ------------------
// Geometric helpers.

struct Square {
    float mTopLeft[2];
    float mBtmRight[2];

    // Neighbors in same level of grid.
    // For sharing vertices via indices.
    Square *northNeighbor = nullptr;
    Square *westNeighbor = nullptr;

    // Vertex indeices of corners.
    // UINT16_MAX means unassigned.
    uint16_t topLeftIdx = UINT16_MAX;
    uint16_t topRightIdx = UINT16_MAX;
    uint16_t bottomRightIdx = UINT16_MAX;
    uint16_t bottomLeftIdx = UINT16_MAX;
    uint16_t centerIdx = UINT16_MAX;

    std::vector<Square> children;

    bool hasChildren() {
        return !children.empty();
    }
};

// --------------------
// Function mesh class.

// Builds a mesh for graphing a function z = f(x, y).
// There is some redundancy among the vertices that we
// will eliminate using indexing in a future version.

class FunctionMesh {
    using F = double (*)(double, double);

    static constexpr bool USE_NEW_MESH = true;

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

    std::stringstream &debugVertex(std::stringstream &debugStrm, uint32_t vertex_i) {
        const Vertex &vertex = mFloorMeshVertices[vertex_i];
        debugStrm << "(" << vertex.pos.x << ", " << vertex.pos.z << ")";
        return debugStrm;
    }

    std::string debugMesh() {
        std::stringstream debugStrm;
        uint32_t square_i = 0;
        for (auto &square : mFloorMeshSquares) {
            debugStrm << "Square " << square_i << std::endl;
            debugStrm << " - top left: ";
            debugVertex(debugStrm, square.topLeftIdx) << std::endl;
            debugStrm << " - top right: ";
            debugVertex(debugStrm, square.topRightIdx) << std::endl;
            debugStrm << " - bottom right: ";
            debugVertex(debugStrm, square.bottomRightIdx) << std::endl;
            debugStrm << " - bottom left: ";
            debugVertex(debugStrm, square.bottomLeftIdx) << std::endl;
            debugStrm << " - center: ";
            debugVertex(debugStrm, square.centerIdx) << std::endl;
            square_i++;
        }
        return debugStrm.str();
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
                    static_cast<float>((j - 1) * width), //
                    static_cast<float>((i - 1) * width), //
                  },
                  .mBtmRight = {
                    static_cast<float>(j * width), //
                    static_cast<float>(i * width), //
                  }
                };
                // clang-format on

                if (j >= 2) {
                    square.westNeighbor = &mFloorMeshSquares.back();
                }
                if (i >= 2) {
                    Square &northNeighbor = mFloorMeshSquares[(i - 2) * mNumCells + (j - 1)];
                    square.northNeighbor = &northNeighbor;
                }

                mFloorMeshSquares.push_back(square);
            }
        }
    }

    // New method. Once complete will replace old methods.
    void computeVerticesAndIndices() {
        mFloorMeshVertices.clear();
        mFloorMeshVertices.reserve((mNumCells + 1) * (mNumCells + 1) + mNumCells * mNumCells);

        for (auto &square : mFloorMeshSquares) {
            float centerX = 0.5 * (square.mTopLeft[0] + square.mBtmRight[0]);
            float centerZ = 0.5 * (square.mTopLeft[1] + square.mBtmRight[1]);

            // Add vertex indices from neighbors if available.
            if (square.northNeighbor != nullptr) {
                square.topLeftIdx = square.northNeighbor->bottomLeftIdx;
                square.topRightIdx = square.northNeighbor->bottomRightIdx;
            }
            if (square.westNeighbor != nullptr) {
                square.topLeftIdx = square.westNeighbor->topRightIdx;
                square.bottomLeftIdx = square.westNeighbor->bottomRightIdx;
            }

            // Add remaining unassigned vertices and indices.
            if (square.topLeftIdx == UINT16_MAX) {
                mFloorMeshVertices.push_back(Vertex{
                    .pos = glm::vec3{square.mTopLeft[0], 0.0, square.mTopLeft[1]}, //
                    .color = FLOOR_COLOR                                           //
                });
                square.topLeftIdx = mFloorMeshVertices.size() - 1;
            }
            if (square.topRightIdx == UINT16_MAX) {
                mFloorMeshVertices.push_back(Vertex{
                    .pos = glm::vec3{square.mBtmRight[0], 0.0, square.mTopLeft[1]}, //
                    .color = FLOOR_COLOR                                            //
                });
                square.topRightIdx = mFloorMeshVertices.size() - 1;
            }
            if (square.bottomRightIdx == UINT16_MAX) {
                mFloorMeshVertices.push_back(Vertex{
                    .pos = glm::vec3{square.mBtmRight[0], 0.0, square.mBtmRight[1]}, //
                    .color = FLOOR_COLOR                                             //
                });
                square.bottomRightIdx = mFloorMeshVertices.size() - 1;
            }
            if (square.bottomLeftIdx == UINT16_MAX) {
                mFloorMeshVertices.push_back(Vertex{
                    .pos = glm::vec3{square.mTopLeft[0], 0.0, square.mBtmRight[1]}, //
                    .color = FLOOR_COLOR                                            //
                });
                square.bottomLeftIdx = mFloorMeshVertices.size() - 1;
            }

            // Add center vertex and index.
            mFloorMeshVertices.push_back(Vertex{
                .pos = glm::vec3{centerX, 0.0, centerZ}, //
                .color = FLOOR_COLOR                     //
            });
            square.centerIdx = mFloorMeshVertices.size() - 1;
        }

        // Copy vertex data
        mFunctionMeshVertices = mFloorMeshVertices;
        for (auto &vertex : mFunctionMeshVertices) {
            vertex.color = FUNCT_COLOR;
            vertex.pos.y = static_cast<float>(mFunc(vertex.pos.x, vertex.pos.z));
        }

        // Create index list for all triangles.
        mMeshIndices.clear();
        mMeshIndices.reserve(mFloorMeshSquares.size() * 12);
        for (auto &square : mFloorMeshSquares) {
            // Top triangle.
            mMeshIndices.push_back(square.centerIdx);
            mMeshIndices.push_back(square.topRightIdx);
            mMeshIndices.push_back(square.topLeftIdx);
            // Left triangle.
            mMeshIndices.push_back(square.centerIdx);
            mMeshIndices.push_back(square.topLeftIdx);
            mMeshIndices.push_back(square.bottomLeftIdx);
            // Bottom triangle.
            mMeshIndices.push_back(square.centerIdx);
            mMeshIndices.push_back(square.bottomLeftIdx);
            mMeshIndices.push_back(square.bottomRightIdx);
            // Right triangle.
            mMeshIndices.push_back(square.centerIdx);
            mMeshIndices.push_back(square.bottomRightIdx);
            mMeshIndices.push_back(square.topRightIdx);
        }
    }

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
    static constexpr int mNumCells = 50;
    // TODO: Switch to larger index type so we can increase this back to 100.

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

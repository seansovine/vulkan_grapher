#ifndef FUNCTION_MESH_H_
#define FUNCTION_MESH_H_

#include "vertex.h"

#include <glm/fwd.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

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

    // Vertex indeices of corners.
    // UINT16_MAX means unassigned.
    uint16_t topLeftIdx     = UINT16_MAX;
    uint16_t topRightIdx    = UINT16_MAX;
    uint16_t bottomRightIdx = UINT16_MAX;
    uint16_t bottomLeftIdx  = UINT16_MAX;
    uint16_t centerIdx      = UINT16_MAX;

    std::vector<Square> children;

    // TODO: For use in automated mesh refinement.
    bool hasChildren() const {
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

    static constexpr bool USE_NEW_MESH     = true;
    static constexpr bool DEBUG_REFINEMENT = true;

    static constexpr uint8_t MAX_REFINEMENT_DEPTH = 1;
    static constexpr double REFINEMENT_THRESHOLD  = 0.25;

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
                    square.northNeighbor  = &northNeighbor;
                }

                mFloorMeshSquares.push_back(square);
            }
        }
    }

    double funcMeshY(uint16_t index) {
        return mFunctionMeshVertices[index].pos.y;
    }

    struct XZCoord {
        float x;
        float z;
    };

    XZCoord meshXZ(uint16_t index) {
        return {mFloorMeshVertices[index].pos.x, mFloorMeshVertices[index].pos.z};
    }

    // Precondition: Square vertex indices are valid for function mesh.
    bool shouldRefine(Square &square) {
        if constexpr (DEBUG_REFINEMENT) {
            std::cout << "Refinement check for square w/ top left corner: ";
            debugVertex(std::cout, square.topLeftIdx) << std::endl;
        }

        if (square.depth >= MAX_REFINEMENT_DEPTH) {
            return false;
        }

        double maxF = std::numeric_limits<double>::lowest();
        double minF = std::numeric_limits<double>::max();

        maxF = std::max(maxF, funcMeshY(square.topLeftIdx));
        minF = std::min(minF, funcMeshY(square.topLeftIdx));

        maxF = std::max(maxF, funcMeshY(square.bottomLeftIdx));
        minF = std::min(minF, funcMeshY(square.bottomLeftIdx));

        maxF = std::max(maxF, funcMeshY(square.bottomRightIdx));
        minF = std::min(minF, funcMeshY(square.bottomRightIdx));

        maxF = std::max(maxF, funcMeshY(square.topRightIdx));
        minF = std::min(minF, funcMeshY(square.topRightIdx));

        maxF = std::max(maxF, funcMeshY(square.centerIdx));
        minF = std::min(minF, funcMeshY(square.centerIdx));

        double valueRange = maxF - minF;
        bool shouldRefine = valueRange > REFINEMENT_THRESHOLD;

        // TODO: Add second derivative or other further condition.

        if constexpr (DEBUG_REFINEMENT) {
            std::cout << " - value range: " << std::to_string(valueRange) << std::endl;
            if (shouldRefine) {
                std::cout << " - Refinement should be done." << std::endl;
            }
        }

        return shouldRefine;
    }

    void refine(Square &square) {
        if constexpr (DEBUG_REFINEMENT) {
            mFunctionMeshVertices[square.topLeftIdx].color     = REFINE_DEBUG_COLOR;
            mFunctionMeshVertices[square.topRightIdx].color    = REFINE_DEBUG_COLOR;
            mFunctionMeshVertices[square.bottomRightIdx].color = REFINE_DEBUG_COLOR;
            mFunctionMeshVertices[square.bottomLeftIdx].color  = REFINE_DEBUG_COLOR;
            mFunctionMeshVertices[square.centerIdx].color      = REFINE_DEBUG_COLOR;
        }

        float center[2] = {0.5f * (square.mTopLeft[0] + square.mBtmRight[0]),
                           0.5f * (square.mTopLeft[1] + square.mBtmRight[1])};

        float topMiddle[2]   = {0.5f * (square.mTopLeft[0] + square.mBtmRight[0]), square.mTopLeft[1]};
        float btmMiddle[2]   = {0.5f * (square.mTopLeft[0] + square.mBtmRight[0]), square.mBtmRight[1]};
        float leftMiddle[2]  = {square.mTopLeft[0], 0.5f * (square.mTopLeft[1] + square.mBtmRight[1])};
        float rightMiddle[2] = {square.mBtmRight[0], 0.5f * (square.mTopLeft[1] + square.mBtmRight[1])};

        glm::vec3 funcColor = FUNCT_COLOR;
        if (DEBUG_REFINEMENT) {
            funcColor = REFINE_DEBUG_COLOR;
        }

        auto addVert = [this, funcColor](float coords[2]) -> uint16_t {
            mFloorMeshVertices.push_back(Vertex{
                .pos   = {coords[0], 0.0f, coords[1]},
                .color = FLOOR_COLOR,
            });
            mFunctionMeshVertices.push_back(Vertex{
                .pos   = {coords[0], mFunc(coords[0], coords[1]), coords[1]},
                .color = funcColor,
            });
            return mFloorMeshVertices.size() - 1;
        };

        uint16_t topMidIdx   = addVert(topMiddle);
        uint16_t rightMidIdx = addVert(rightMiddle);
        uint16_t btmMidIdx   = addVert(btmMiddle);
        uint16_t leftMidIdx  = addVert(leftMiddle);

        auto makeCenter = [](float topLeft[2], float btmRight[2]) -> XZCoord {
            return {0.5f * (topLeft[0] + btmRight[0]), 0.5f * (topLeft[1] + btmRight[1])};
        };

        // Add four children; recurse on children as needed.
        // Update mesh vertices and indices as needed.

        // Add top left child.

        uint32_t childDepth = square.depth + 1;

        XZCoord newCenter        = makeCenter(square.mTopLeft, center);
        float newCenterCoords[3] = {newCenter.x, newCenter.z};
        uint16_t newCenterIdx    = addVert(newCenterCoords);

        square.children.push_back(Square{
            .mTopLeft  = {square.mTopLeft[0], square.mTopLeft[1]},
            .mBtmRight = {center[0], center[1]},

            .depth = childDepth,

            .topLeftIdx     = square.topLeftIdx,
            .topRightIdx    = topMidIdx,
            .bottomRightIdx = square.centerIdx,
            .bottomLeftIdx  = leftMidIdx,
            .centerIdx      = newCenterIdx,
        });

        // Add top right child.

        XZCoord newCenter2        = makeCenter(topMiddle, rightMiddle);
        float newCenterCoords2[3] = {newCenter2.x, newCenter.z};
        uint16_t newCenterIdx2    = addVert(newCenterCoords2);

        square.children.push_back(Square{
            .mTopLeft  = {topMiddle[0], topMiddle[1]},
            .mBtmRight = {rightMiddle[0], rightMiddle[1]},

            .depth = childDepth,

            .westNeighbor = &square.children.back(),

            .topLeftIdx     = topMidIdx,
            .topRightIdx    = square.topRightIdx,
            .bottomRightIdx = rightMidIdx,
            .bottomLeftIdx  = square.centerIdx,
            .centerIdx      = newCenterIdx2,
        });

        // Add bottom left child.

        XZCoord newCenter3        = makeCenter(leftMiddle, btmMiddle);
        float newCenterCoords3[3] = {newCenter3.x, newCenter3.z};
        uint16_t newCenterIdx3    = addVert(newCenterCoords3);

        square.children.push_back(Square{
            .mTopLeft  = {leftMiddle[0], leftMiddle[1]},
            .mBtmRight = {btmMiddle[0], btmMiddle[1]},

            .depth = childDepth,

            .northNeighbor = &square.children[square.children.size() - 2],

            .topLeftIdx     = leftMidIdx,
            .topRightIdx    = square.centerIdx,
            .bottomRightIdx = btmMidIdx,
            .bottomLeftIdx  = square.bottomLeftIdx,
            .centerIdx      = newCenterIdx3,
        });

        // Add bottom right child.

        XZCoord newCenter4        = makeCenter(center, square.mBtmRight);
        float newCenterCoords4[3] = {newCenter4.x, newCenter4.z};
        uint16_t newCenterIdx4    = addVert(newCenterCoords4);

        square.children.push_back(Square{
            .mTopLeft  = {center[0], center[1]},
            .mBtmRight = {square.mBtmRight[0], square.mBtmRight[1]},

            .depth = childDepth,

            .northNeighbor = &square.children[square.children.size() - 2],
            .westNeighbor  = &square.children.back(),

            .topLeftIdx     = square.centerIdx,
            .topRightIdx    = rightMidIdx,
            .bottomRightIdx = square.bottomRightIdx,
            .bottomLeftIdx  = btmMidIdx,
            .centerIdx      = newCenterIdx4,
        });

        // TODO: Recurse if necessary.
    }

    void addTriIndices(const Square &square) {
        // If square has children, instead recurse into them.
        if (square.hasChildren()) {
            for (const Square &child : square.children) {
                addTriIndices(child);
            }
        }

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

    // New method. Once complete will replace old methods.
    void computeVerticesAndIndices() {
        mFloorMeshVertices.clear();
        mFloorMeshVertices.reserve((mNumCells + 1) * (mNumCells + 1) + mNumCells * mNumCells);

        for (auto &square : mFloorMeshSquares) {
            float centerX = 0.5 * (square.mTopLeft[0] + square.mBtmRight[0]);
            float centerZ = 0.5 * (square.mTopLeft[1] + square.mBtmRight[1]);

            // Add vertex indices from neighbors if available.
            if (square.northNeighbor != nullptr) {
                square.topLeftIdx  = square.northNeighbor->bottomLeftIdx;
                square.topRightIdx = square.northNeighbor->bottomRightIdx;
            }
            if (square.westNeighbor != nullptr) {
                square.topLeftIdx    = square.westNeighbor->topRightIdx;
                square.bottomLeftIdx = square.westNeighbor->bottomRightIdx;
            }

            // Add remaining unassigned vertices and indices.
            if (square.topLeftIdx == UINT16_MAX) {
                mFloorMeshVertices.push_back(Vertex{
                    .pos   = glm::vec3{square.mTopLeft[0], 0.0, square.mTopLeft[1]}, //
                    .color = FLOOR_COLOR                                             //
                });
                square.topLeftIdx = mFloorMeshVertices.size() - 1;
            }
            if (square.topRightIdx == UINT16_MAX) {
                mFloorMeshVertices.push_back(Vertex{
                    .pos   = glm::vec3{square.mBtmRight[0], 0.0, square.mTopLeft[1]}, //
                    .color = FLOOR_COLOR                                              //
                });
                square.topRightIdx = mFloorMeshVertices.size() - 1;
            }
            if (square.bottomRightIdx == UINT16_MAX) {
                mFloorMeshVertices.push_back(Vertex{
                    .pos   = glm::vec3{square.mBtmRight[0], 0.0, square.mBtmRight[1]}, //
                    .color = FLOOR_COLOR                                               //
                });
                square.bottomRightIdx = mFloorMeshVertices.size() - 1;
            }
            if (square.bottomLeftIdx == UINT16_MAX) {
                mFloorMeshVertices.push_back(Vertex{
                    .pos   = glm::vec3{square.mTopLeft[0], 0.0, square.mBtmRight[1]}, //
                    .color = FLOOR_COLOR                                              //
                });
                square.bottomLeftIdx = mFloorMeshVertices.size() - 1;
            }

            // Add center vertex and index.
            mFloorMeshVertices.push_back(Vertex{
                .pos   = glm::vec3{centerX, 0.0, centerZ}, //
                .color = FLOOR_COLOR                       //
            });
            square.centerIdx = mFloorMeshVertices.size() - 1;
        }

        // Copy vertex data
        mFunctionMeshVertices = mFloorMeshVertices;
        for (auto &vertex : mFunctionMeshVertices) {
            vertex.color = FUNCT_COLOR;
            vertex.pos.y = static_cast<float>(mFunc(vertex.pos.x, vertex.pos.z));
        }

        for (auto &square : mFloorMeshSquares) {
            if (shouldRefine(square)) {
                refine(square);
            }
        }

        // Create index list for all triangles in squares.
        mMeshIndices.clear();
        mMeshIndices.reserve(mFloorMeshSquares.size() * 12);
        for (auto &square : mFloorMeshSquares) {
            addTriIndices(square);
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

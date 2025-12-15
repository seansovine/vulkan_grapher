#include "function_mesh.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

void FunctionMesh::buildFloorMesh() {
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

double FunctionMesh::secondDerivEst(const Square &square, const SquareFuncEval &funcVals) {
    float center[2]      = {0.5f * (square.mTopLeft[0] + square.mBtmRight[0]),
                            0.5f * (square.mTopLeft[1] + square.mBtmRight[1])};
    float topMiddle[2]   = {0.5f * (square.mTopLeft[0] + square.mBtmRight[0]), square.mTopLeft[1]};
    float btmMiddle[2]   = {0.5f * (square.mTopLeft[0] + square.mBtmRight[0]), square.mBtmRight[1]};
    float leftMiddle[2]  = {square.mTopLeft[0], 0.5f * (square.mTopLeft[1] + square.mBtmRight[1])};
    float rightMiddle[2] = {square.mBtmRight[0], 0.5f * (square.mTopLeft[1] + square.mBtmRight[1])};

    double centerY      = mFunc(center[0], center[1]);
    double topMiddleY   = mFunc(topMiddle[0], topMiddle[1]);
    double btmMiddleY   = mFunc(btmMiddle[0], btmMiddle[1]);
    double leftMiddleY  = mFunc(leftMiddle[0], leftMiddle[1]);
    double rightMiddleY = mFunc(rightMiddle[0], rightMiddle[1]);

    double topLeftY  = funcMeshY(square.topLeftIdx);
    double btmLeftY  = funcMeshY(square.bottomLeftIdx);
    double topRightY = funcMeshY(square.topRightIdx);
    double btmRightY = funcMeshY(square.bottomRightIdx);

    // See: https://en.wikipedia.org/wiki/Finite_difference#Multivariate_finite_differences
    double fxx = 4.0 * (rightMiddleY + leftMiddleY - 2.0 * centerY) / (mCellWidth * mCellWidth);
    double fyy = 4.0 * (topMiddleY + btmMiddleY - 2.0 * centerY) / (mCellWidth * mCellWidth);
    double fxy = (topRightY - btmRightY - topLeftY + btmLeftY) / (mCellWidth * mCellWidth);

    return std::sqrt(fxx * fxx + fyy * fyy + fxy * fxy) / 3.0;
}

// Precondition: Square vertex indices are valid for function mesh.
bool FunctionMesh::shouldRefine(Square &square) {
    if constexpr (DEBUG_REFINEMENT) {
        std::cout << "Refinement check for square w/ top left corner: ";
        debugVertex(std::cout, square.topLeftIdx) << std::endl;
    }

    if (square.depth >= MAX_REFINEMENT_DEPTH) {
        return false;
    }

    double maxF = std::numeric_limits<double>::lowest();
    double minF = std::numeric_limits<double>::max();

    SquareFuncEval funcVals = evalFuncSquare(square);

    maxF = std::max(maxF, funcVals.topLeftVal);
    minF = std::min(minF, funcVals.topLeftVal);
    maxF = std::max(maxF, funcVals.topRightVal);
    minF = std::min(minF, funcVals.topRightVal);
    maxF = std::max(maxF, funcVals.btmRightVal);
    minF = std::min(minF, funcVals.btmRightVal);
    maxF = std::max(maxF, funcVals.btmLeftVal);
    minF = std::min(minF, funcVals.btmLeftVal);
    maxF = std::max(maxF, funcVals.centerVal);
    minF = std::min(minF, funcVals.centerVal);

    double valueRange     = maxF - minF;
    double secondDerivMag = secondDerivEst(square, funcVals);

    bool shouldRefine = valueRange > REFINEMENT_THRESHOLD_VARIATION || secondDerivMag > REFINEMENT_THRESHOLD_2ND_DERIV;

    if constexpr (DEBUG_REFINEMENT) {
        std::cout << " - value range: " << std::to_string(valueRange) << std::endl;
        std::cout << " - second deriv. magnitude: " << std::to_string(secondDerivMag) << std::endl;
        if (shouldRefine) {
            std::cout << " - Refinement should be done." << std::endl;
        }
    }

    return shouldRefine;
}

void FunctionMesh::refine(Square &square) {
    if constexpr (DEBUG_REFINEMENT) {
        mFunctionMeshVertices[square.topLeftIdx].color     = REFINE_DEBUG_COLOR;
        mFunctionMeshVertices[square.topRightIdx].color    = REFINE_DEBUG_COLOR;
        mFunctionMeshVertices[square.bottomRightIdx].color = REFINE_DEBUG_COLOR;
        mFunctionMeshVertices[square.bottomLeftIdx].color  = REFINE_DEBUG_COLOR;
        mFunctionMeshVertices[square.centerIdx].color      = REFINE_DEBUG_COLOR;
    }

    float center[2]      = {0.5f * (square.mTopLeft[0] + square.mBtmRight[0]),
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

void FunctionMesh::addTriIndices(const Square &square) {
    // If square has children, instead recurse into them.
    if (square.hasChildren()) {
        for (const Square &child : square.children) {
            addTriIndices(child);
        }
        return;
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
void FunctionMesh::computeVerticesAndIndices() {
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

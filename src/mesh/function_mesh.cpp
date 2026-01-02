#include "function_mesh.h"

#include "mesh.h"

#include <algorithm>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

// Square implementations.

// Postcondition: Edge refinments vectors will be populated in appropriate
//                order for the orientation of each edge.
const Square::EdgeRefinements &Square::populateRefinements() {
    if (!hasChildren()) {
        edgeRefinements = {
            .north = {topLeftIdx, topRightIdx},
            .west  = {topLeftIdx, bottomLeftIdx},
            .south = {bottomLeftIdx, bottomRightIdx},
            .east  = {topRightIdx, bottomRightIdx},
        };
        return edgeRefinements;
    }
    // Invariant: A square has 0 or 4 children.
    assert(children.size() == 4);

    // Absorb refinements from children.

    Square &topLeftChild                    = children[0];
    const EdgeRefinements *childRefinements = &topLeftChild.populateRefinements();
    for (uint32_t i = 0; i < childRefinements->north.size() - 1; i++) {
        edgeRefinements.north.push_back(childRefinements->north[i]);
    }
    for (uint32_t i = 0; i < childRefinements->west.size() - 1; i++) {
        edgeRefinements.west.push_back(childRefinements->west[i]);
    }
    // Omit last to avoid duplicates.

    Square &topRightChild = children[1];
    childRefinements      = &topRightChild.populateRefinements();
    for (uint32_t i = 0; i < childRefinements->north.size(); i++) {
        edgeRefinements.north.push_back(childRefinements->north[i]);
    }
    for (uint32_t i = 0; i < childRefinements->east.size() - 1; i++) {
        edgeRefinements.east.push_back(childRefinements->east[i]);
    }

    Square &bottomLeftChild = children[2];
    childRefinements        = &bottomLeftChild.populateRefinements();
    for (uint32_t i = 0; i < childRefinements->south.size() - 1; i++) {
        edgeRefinements.south.push_back(childRefinements->south[i]);
    }
    for (uint32_t i = 0; i < childRefinements->west.size(); i++) {
        edgeRefinements.west.push_back(childRefinements->west[i]);
    }

    Square &bottomRightChild = children[3];
    childRefinements         = &bottomRightChild.populateRefinements();
    for (uint32_t i = 0; i < childRefinements->south.size(); i++) {
        edgeRefinements.south.push_back(childRefinements->south[i]);
    }
    for (uint32_t i = 0; i < childRefinements->east.size(); i++) {
        edgeRefinements.east.push_back(childRefinements->east[i]);
    }

    return edgeRefinements;
}

// FunctionMesh implementations.

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

            // The ONLY place where we insert into this vector.
            mFloorMeshSquares.push_back(square);
            Square &newSquare = mFloorMeshSquares.back();

            // Assign neighbors in top-level grid.
            if (j >= 2) {
                Square &westNeighbor      = mFloorMeshSquares.at(mFloorMeshSquares.size() - 2);
                newSquare.westNeighbor    = &westNeighbor;
                westNeighbor.eastNeighbor = &newSquare;
            }
            if (i >= 2) {
                Square &northNeighbor       = mFloorMeshSquares.at((i - 2) * mNumCells + (j - 1));
                newSquare.northNeighbor     = &northNeighbor;
                northNeighbor.southNeighbor = &newSquare;
            }
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

void FunctionMesh::addFloorMeshVertex(float x, float z) {
    mFloorMeshVertices.push_back(Vertex{
        .pos       = {x, 0.0f, z},
        .color     = FLOOR_COLOR,
        .tangent   = {1.0f, 0.0f, 0.0f},
        .bitangent = {0.0f, 0.0f, 1.0f},
        .normal    = {0.0f, 1.0f, 0.0f},
    });
}

void FunctionMesh::refine(Square &square) {
    glm::vec3 funcColor = FUNCT_COLOR;

    if constexpr (SHOW_REFINEMENT) {

        switch (square.depth) {
        case 0: {
            funcColor = REFINE_DEBUG_COLOR1;
            break;
        }
        default: {
            funcColor = REFINE_DEBUG_COLOR2;
            break;
        }
        }

        mFunctionMeshVertices[square.topLeftIdx].color     = funcColor;
        mFunctionMeshVertices[square.topRightIdx].color    = funcColor;
        mFunctionMeshVertices[square.bottomRightIdx].color = funcColor;
        mFunctionMeshVertices[square.bottomLeftIdx].color  = funcColor;
        mFunctionMeshVertices[square.centerIdx].color      = funcColor;
    }

    float center[2]      = {0.5f * (square.mTopLeft[0] + square.mBtmRight[0]),
                            0.5f * (square.mTopLeft[1] + square.mBtmRight[1])};
    float topMiddle[2]   = {0.5f * (square.mTopLeft[0] + square.mBtmRight[0]), square.mTopLeft[1]};
    float btmMiddle[2]   = {0.5f * (square.mTopLeft[0] + square.mBtmRight[0]), square.mBtmRight[1]};
    float leftMiddle[2]  = {square.mTopLeft[0], 0.5f * (square.mTopLeft[1] + square.mBtmRight[1])};
    float rightMiddle[2] = {square.mBtmRight[0], 0.5f * (square.mTopLeft[1] + square.mBtmRight[1])};

    auto addVert = [this, funcColor](float coords[2]) -> uint16_t {
        addFloorMeshVertex(coords[0], coords[1]);
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

        .parent = &square,

        .topLeftIdx     = square.topLeftIdx,
        .topRightIdx    = topMidIdx,
        .bottomRightIdx = square.centerIdx,
        .bottomLeftIdx  = leftMidIdx,
        .centerIdx      = newCenterIdx,
    });
    Square &topLeftChild = square.children.back();

    // Add top right child.

    XZCoord newCenter2        = makeCenter(topMiddle, rightMiddle);
    float newCenterCoords2[3] = {newCenter2.x, newCenter.z};
    uint16_t newCenterIdx2    = addVert(newCenterCoords2);

    square.children.push_back(Square{
        .mTopLeft  = {topMiddle[0], topMiddle[1]},
        .mBtmRight = {rightMiddle[0], rightMiddle[1]},

        .depth = childDepth,

        .westNeighbor = &topLeftChild,
        .parent       = &square,

        .topLeftIdx     = topMidIdx,
        .topRightIdx    = square.topRightIdx,
        .bottomRightIdx = rightMidIdx,
        .bottomLeftIdx  = square.centerIdx,
        .centerIdx      = newCenterIdx2,
    });
    Square &topRightChild     = square.children.back();
    topLeftChild.eastNeighbor = &topRightChild;

    // Add bottom left child.

    XZCoord newCenter3        = makeCenter(leftMiddle, btmMiddle);
    float newCenterCoords3[3] = {newCenter3.x, newCenter3.z};
    uint16_t newCenterIdx3    = addVert(newCenterCoords3);

    square.children.push_back(Square{
        .mTopLeft  = {leftMiddle[0], leftMiddle[1]},
        .mBtmRight = {btmMiddle[0], btmMiddle[1]},

        .depth = childDepth,

        .northNeighbor = &topLeftChild,
        .parent        = &square,

        .topLeftIdx     = leftMidIdx,
        .topRightIdx    = square.centerIdx,
        .bottomRightIdx = btmMidIdx,
        .bottomLeftIdx  = square.bottomLeftIdx,
        .centerIdx      = newCenterIdx3,
    });
    Square &bottomLeftChild    = square.children.back();
    topLeftChild.southNeighbor = &bottomLeftChild;

    // Add bottom right child.

    XZCoord newCenter4        = makeCenter(center, square.mBtmRight);
    float newCenterCoords4[3] = {newCenter4.x, newCenter4.z};
    uint16_t newCenterIdx4    = addVert(newCenterCoords4);

    square.children.push_back(Square{
        .mTopLeft  = {center[0], center[1]},
        .mBtmRight = {square.mBtmRight[0], square.mBtmRight[1]},

        .depth = childDepth,

        .northNeighbor = &topRightChild,
        .westNeighbor  = &bottomLeftChild,
        .parent        = &square,

        .topLeftIdx     = square.centerIdx,
        .topRightIdx    = rightMidIdx,
        .bottomRightIdx = square.bottomRightIdx,
        .bottomLeftIdx  = btmMidIdx,
        .centerIdx      = newCenterIdx4,
    });
    Square &bottomRightChild     = square.children.back();
    topRightChild.southNeighbor  = &bottomRightChild;
    bottomLeftChild.eastNeighbor = &bottomRightChild;

    // Recurse if necessary.
    for (auto &child : square.children) {
        if (shouldRefine(child)) {
            refine(child);
        }
    }
}

void FunctionMesh::addSquareTris(const Square &square) {
    // If square has children, instead recurse into them.
    if (square.hasChildren()) {
        for (const Square &child : square.children) {
            addSquareTris(child);
        }
        return;
    }

    auto addTri = [this](uint16_t idx1, uint16_t idx2, uint16_t idx3) {
        Triangle newTri = {.vert1Idx = idx1, .vert2Idx = idx2, .vert3Idx = idx3};
        mFunctionMeshTriangles.push_back(newTri);
        size_t newTriIdx = mFunctionMeshTriangles.size() - 1;
        mVertexTriangles[idx1].insert(newTriIdx);
        mVertexTriangles[idx2].insert(newTriIdx);
        mVertexTriangles[idx3].insert(newTriIdx);
    };

    // NOTE: This handles squares that don't have children. Effectively
    //       we currently only support refining meshes to a single level.
    //       We'll have to think of something more sophisticated to allow
    //       further levels of refinement.

    // Top triangles.
    const auto &northRefinements = square.edgeRefinements.north;
    for (uint32_t i = 0; i < northRefinements.size() - 1; i++) {
        addTri(square.centerIdx, northRefinements[i + 1], northRefinements[i]);
    }

    // Left triangles.
    const auto &westRefinements = square.edgeRefinements.west;
    for (uint32_t i = 0; i < westRefinements.size() - 1; i++) {
        addTri(square.centerIdx, westRefinements[i], westRefinements[i + 1]);
    }

    // Bottom triangles.
    const auto &southRefinements = square.edgeRefinements.south;
    for (uint32_t i = 0; i < southRefinements.size() - 1; i++) {
        addTri(square.centerIdx, southRefinements[i], southRefinements[i + 1]);
    }

    // Right triangles.
    const auto &eastRefinements = square.edgeRefinements.east;
    for (uint32_t i = 0; i < eastRefinements.size() - 1; i++) {
        addTri(square.centerIdx, eastRefinements[i + 1], eastRefinements[i]);
    }
}

void FunctionMesh::addTriIndices(const Triangle &tri) {
    mMeshIndices.push_back(tri.vert1Idx);
    mMeshIndices.push_back(tri.vert2Idx);
    mMeshIndices.push_back(tri.vert3Idx);
}

void FunctionMesh::setFuncVertTBNs() {
    // Assign normal to each triangle.
    for (Triangle &tri : mFunctionMeshTriangles) {
        glm::vec3 &vert1 = mFunctionMeshVertices[tri.vert1Idx].pos;
        glm::vec3 &vert2 = mFunctionMeshVertices[tri.vert2Idx].pos;
        glm::vec3 &vert3 = mFunctionMeshVertices[tri.vert3Idx].pos;
        // TODO: Make sure we have the correct orientation.
        tri.normal = glm::normalize(glm::cross(vert2 - vert1, vert3 - vert1));
    }

    constexpr glm::vec3 xDir = {1.0f, 0.0f, 0.0f};
    constexpr glm::vec3 zDir = {0.0f, 0.0f, 1.0f};

    // Now compute TBN basis for each vertex by averaging tri normals.
    for (uint16_t i = 0; i < mFloorMeshVertices.size(); i++) {
        Vertex &funcVert = mFunctionMeshVertices[i];

        glm::vec3 avgNormal = {0.0f, 0.0f, 0.0f};
        for (uint16_t vertTriIdx : mVertexTriangles[i]) {
            const Triangle &vertTri = mFunctionMeshTriangles[vertTriIdx];
            avgNormal += vertTri.normal * (1.0f / mVertexTriangles[i].size());
        }
        avgNormal       = glm::normalize(avgNormal);
        funcVert.normal = avgNormal;

        // Use Gram-Schmidt to get ONB.
        glm::vec3 tangent = glm::normalize(xDir - glm::dot(xDir, avgNormal) * avgNormal);
        glm::vec3 bitangent =
            glm::normalize(zDir - glm::dot(zDir, avgNormal) * avgNormal - glm::dot(zDir, tangent) * tangent);
        funcVert.tangent   = tangent;
        funcVert.bitangent = bitangent;

        // Verify orientation and orthonormality.
        constexpr float TRIPLE_ERROR_TOLERANCE = 0.01f;
        float scalarTriple                     = glm::dot(avgNormal, glm::cross(funcVert.bitangent, funcVert.tangent));
        if (std::abs(scalarTriple - 1.0f) >= TRIPLE_ERROR_TOLERANCE) {
            std::cout << "TBN scalar triple product: " << std::to_string(scalarTriple) << std::endl;
        }
        assert(std::abs(scalarTriple - 1.0f) < TRIPLE_ERROR_TOLERANCE);
    }
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
            addFloorMeshVertex(square.mTopLeft[0], square.mTopLeft[1]);
            square.topLeftIdx = mFloorMeshVertices.size() - 1;
        }
        if (square.topRightIdx == UINT16_MAX) {
            addFloorMeshVertex(square.mBtmRight[0], square.mTopLeft[1]);
            square.topRightIdx = mFloorMeshVertices.size() - 1;
        }
        if (square.bottomRightIdx == UINT16_MAX) {
            addFloorMeshVertex(square.mBtmRight[0], square.mBtmRight[1]);
            square.bottomRightIdx = mFloorMeshVertices.size() - 1;
        }
        if (square.bottomLeftIdx == UINT16_MAX) {
            addFloorMeshVertex(square.mTopLeft[0], square.mBtmRight[1]);
            square.bottomLeftIdx = mFloorMeshVertices.size() - 1;
        }

        // Add center vertex and index.
        addFloorMeshVertex(centerX, centerZ);
        square.centerIdx = mFloorMeshVertices.size() - 1;
    }

    // Copy vertex data
    mFunctionMeshVertices = mFloorMeshVertices;
    for (auto &vertex : mFunctionMeshVertices) {
        vertex.color = FUNCT_COLOR;
        vertex.pos.y = static_cast<float>(mFunc(vertex.pos.x, vertex.pos.z));
    }

    // Refine squares and populate initial edge refinements.
    for (auto &square : mFloorMeshSquares) {
        if (shouldRefine(square)) {
            refine(square);
        }
        auto &_ = square.populateRefinements();
    }

    // Update edge refinements from neighbors to make mesh water tight.
    for (auto &square : mFloorMeshSquares) {
        syncEdgeRefinements(square);
    }

    mMeshIndices.clear();
    // NOTE: This will not be accurate if refinement happens.
    mMeshIndices.reserve(mFloorMeshSquares.size() * 12);
    // These must have the same size as # mesh vertices.
    mFunctionMeshTriangles.resize(mFloorMeshVertices.size());
    mVertexTriangles.resize(mFloorMeshVertices.size());

    // Create triangles for squares.
    for (auto &square : mFloorMeshSquares) {
        addSquareTris(square);
    }

    // Create indices for triangles.
    for (const Triangle &tri : mFunctionMeshTriangles) {
        addTriIndices(tri);
    }

    setFuncVertTBNs();
}

static constexpr bool DEV_REF_DEBUG = false;

static std::vector<uint16_t> *getNorthNbRefinements(Square *square) {
    while (square != nullptr) {
        if (square->northNeighbor != nullptr) {
            return &square->northNeighbor->edgeRefinements.south;
        } else {
            square = square->parent;
        }
    }
    return nullptr;
}
static std::vector<uint16_t> *getSouthNbRefinements(Square *square) {
    while (square != nullptr) {
        if (square->southNeighbor != nullptr) {
            return &square->southNeighbor->edgeRefinements.north;
        } else {
            square = square->parent;
        }
    }
    return nullptr;
}
static std::vector<uint16_t> *getEastNbRefinements(Square *square) {
    while (square != nullptr) {
        if (square->eastNeighbor != nullptr) {
            return &square->eastNeighbor->edgeRefinements.west;
        } else {
            square = square->parent;
        }
    }
    return nullptr;
}
static std::vector<uint16_t> *getWestNbRefinements(Square *square) {
    while (square != nullptr) {
        if (square->westNeighbor != nullptr) {
            return &square->westNeighbor->edgeRefinements.east;
        } else {
            square = square->parent;
        }
    }
    return nullptr;
}

// Precondition: to and from are sorted left-to-right.
void FunctionMesh::syncRefmtsHoriz(std::vector<uint16_t> &to, std::vector<uint16_t> &from) {
    assert(!to.empty());

    auto getX = [this](size_t index) -> float {
        return mFloorMeshVertices[index].pos.x; //
    };

    float leftLim  = getX(to.front());
    float rightLim = getX(to.back());

    for (uint16_t fromIdx : from) {
        float fromX = getX(fromIdx);
        if (leftLim < fromX && fromX < rightLim) {
            if constexpr (DEV_REF_DEBUG) {
                std::cout << " --- Syncing refinement." << std::endl;
            }
            to.push_back(fromIdx);
        }
    }

    // Now re-sort by x-coord and remove duplicates.
    std::sort(to.begin(), to.end(), [getX](uint16_t a, uint16_t b) {
        return getX(a) < getX(b); //
    });
    auto last = std::unique(to.begin(), to.end(), [getX](uint16_t a, uint16_t b) {
        return getX(a) == getX(b); //
    });
    to.erase(last, to.end());
}

// Precondition: to and from are sorted by increasing z.
void FunctionMesh::syncRefmtsVert(std::vector<uint16_t> &to, std::vector<uint16_t> &from) { // TODO: Update to vertical.
    assert(!to.empty());

    auto getZ = [this](size_t index) -> float {
        return mFloorMeshVertices[index].pos.z; //
    };

    float leftLim  = getZ(to.front());
    float rightLim = getZ(to.back());

    for (uint16_t fromIdx : from) {
        float fromX = getZ(fromIdx);
        if (leftLim < fromX && fromX < rightLim) {
            if constexpr (DEV_REF_DEBUG) {
                std::cout << " --- Syncing refinement." << std::endl;
            }
            to.push_back(fromIdx);
        }
    }

    // Now re-sort by x-coord and remove duplicates.
    std::sort(to.begin(), to.end(), [getZ](uint16_t a, uint16_t b) {
        return getZ(a) < getZ(b); //
    });
    auto last = std::unique(to.begin(), to.end(), [getZ](uint16_t a, uint16_t b) {
        return getZ(a) == getZ(b); //
    });
    to.erase(last, to.end());
}

// Precondition: All edge refinments have been populated.
void FunctionMesh::syncEdgeRefinements(Square &square) {
    if (square.hasChildren()) {
        for (auto &child : square.children) {
            syncEdgeRefinements(child);
        }
        // This only needs done for leaf cells, which are rendered.
        return;
    }

    auto northNbRefmnts = getNorthNbRefinements(&square);
    if (northNbRefmnts != nullptr && northNbRefmnts->size() > 2) {
        if constexpr (DEV_REF_DEBUG) {
            std::cout << ">>> Before north refinement sync: " << std::endl;
            debugRefinements(std::cout, square) << std::endl;
        }
        syncRefmtsHoriz(square.edgeRefinements.north, *northNbRefmnts);
        if constexpr (DEV_REF_DEBUG) {
            std::cout << ">>> After north refinement sync: " << std::endl;
            debugRefinements(std::cout, square) << std::endl;
        }
    }
    auto southNbRefmnts = getSouthNbRefinements(&square);
    if (southNbRefmnts != nullptr && southNbRefmnts->size() > 2) {
        if constexpr (DEV_REF_DEBUG) {
            std::cout << "Doing a south neighbor refinement sync. # refinements: "
                      << std::to_string(southNbRefmnts->size()) << std::endl;
            uint32_t square_i = 0;
            std::cout << debugSquareCell(square, square_i) << std::endl;
            std::cout << debugSquareCell(*square.southNeighbor, square_i) << std::endl;
        }
        syncRefmtsHoriz(square.edgeRefinements.south, *southNbRefmnts);
    }
    auto eastNbRefmnts = getEastNbRefinements(&square);
    if (eastNbRefmnts != nullptr && eastNbRefmnts->size() > 2) {
        syncRefmtsVert(square.edgeRefinements.east, *eastNbRefmnts);
    }
    auto westNbRefmnts = getWestNbRefinements(&square);
    if (westNbRefmnts != nullptr && westNbRefmnts->size() > 2) {
        syncRefmtsVert(square.edgeRefinements.west, *westNbRefmnts);
    }
}

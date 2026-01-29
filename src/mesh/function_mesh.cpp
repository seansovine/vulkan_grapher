#include "function_mesh.h"

#include "mesh.h"
#include "util.h"

#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cmath>
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
    mFloorMeshSquares.reserve(NUM_CELLS * NUM_CELLS);
    const double width = 1.0 / NUM_CELLS;

    for (int i = 1; i <= NUM_CELLS; i++) {
        for (int j = 1; j <= NUM_CELLS; j++) {
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
                Square &northNeighbor       = mFloorMeshSquares.at((i - 2) * NUM_CELLS + (j - 1));
                newSquare.northNeighbor     = &northNeighbor;
                northNeighbor.southNeighbor = &newSquare;
            }
        }
    }
}

double FunctionMesh::secondDerivEst(const Square &square) {
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

double FunctionMesh::secondDerivEstMax(const glm::vec3 &pos) {
    double x = pos.x;
    double z = pos.z;

    double fxx = (mFunc(x + H, z) - 2.0 * mFunc(x, z) + mFunc(x - H, z)) / (H * H);
    double fzz = (mFunc(x, z + H) - 2.0 * mFunc(x, z) + mFunc(x, z - H)) / (H * H);

    double fxz = (mFunc(x + H, z + H) - mFunc(x + H, z) - mFunc(x, z + H)    //
                  + 2.0 * mFunc(x, z)                                        //
                  - mFunc(x - H, z) - mFunc(x, z - H) + mFunc(x - H, z - H)) //
                 / (2.0 * H * H);

    return std::max({std::abs(fxx), std::abs(fzz), std::abs(fxz)});
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
    double secondDerivMag = secondDerivEst(square);

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

    auto addVert = [this, funcColor](float coords[2]) -> uint32_t {
        addFloorMeshVertex(coords[0], coords[1]);
        mFunctionMeshVertices.push_back(Vertex{
            .pos   = {coords[0], mFunc(coords[0], coords[1]), coords[1]},
            .color = funcColor,
        });
        return mFloorMeshVertices.size() - 1;
    };

    uint32_t topMidIdx   = addVert(topMiddle);
    uint32_t rightMidIdx = addVert(rightMiddle);
    uint32_t btmMidIdx   = addVert(btmMiddle);
    uint32_t leftMidIdx  = addVert(leftMiddle);

    auto makeCenter = [](float topLeft[2], float btmRight[2]) -> XZCoord {
        return {0.5f * (topLeft[0] + btmRight[0]), 0.5f * (topLeft[1] + btmRight[1])};
    };

    // Add four children; recurse on children as needed.
    // Update mesh vertices and indices as needed.

    // Add top left child.

    uint32_t childDepth = square.depth + 1;

    XZCoord newCenter        = makeCenter(square.mTopLeft, center);
    float newCenterCoords[3] = {newCenter.x, newCenter.z};
    uint32_t newCenterIdx    = addVert(newCenterCoords);

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
    uint32_t newCenterIdx2    = addVert(newCenterCoords2);

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
    uint32_t newCenterIdx3    = addVert(newCenterCoords3);

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
    uint32_t newCenterIdx4    = addVert(newCenterCoords4);

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

constexpr bool DEV_DEBUG = false;

void FunctionMesh::addSquareTris(const Square &square) {
    // If square has children, instead recurse into them.
    if (square.hasChildren()) {
        for (const Square &child : square.children) {
            addSquareTris(child);
        }
        return;
    }
    if constexpr (DEV_DEBUG) {
        logIndices(square);
        uint32_t square_i = 0;
        std::cout << debugSquareCell(square, square_i, false);
    }

    auto addTri = [this](uint32_t idx1, uint32_t idx2, uint32_t idx3) {
        Triangle newTri = {.vert1Idx = idx1, .vert2Idx = idx2, .vert3Idx = idx3};
        mFunctionMeshTriangles.push_back(newTri);
        size_t newTriIdx = mFunctionMeshTriangles.size() - 1;
        mVertexTriangles[idx1].insert(newTriIdx);
        mVertexTriangles[idx2].insert(newTriIdx);
        mVertexTriangles[idx3].insert(newTriIdx);
        if constexpr (DEV_DEBUG) {
            debugTriangle(mFunctionMeshTriangles.back());
        }
    };

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
    // Assign normal and area to each triangle.
    for (Triangle &tri : mFunctionMeshTriangles) {
        glm::vec3 &vert1 = mFunctionMeshVertices[tri.vert1Idx].pos;
        glm::vec3 &vert2 = mFunctionMeshVertices[tri.vert2Idx].pos;
        glm::vec3 &vert3 = mFunctionMeshVertices[tri.vert3Idx].pos;
        tri.normal       = glm::normalize(glm::cross(vert2 - vert1, vert3 - vert1));

        double len1 = glm::length(vert1 - vert2);
        double len2 = glm::length(vert2 - vert3);
        double len3 = glm::length(vert3 - vert1);
        tri.area    = math_util::triangleArea(len1, len2, len3);
    }

    constexpr glm::dvec3 xDir = {1.0f, 0.0f, 0.0f};
    constexpr glm::dvec3 zDir = {0.0f, 0.0f, 1.0f};

    // Now compute TBN basis for each vertex by averaging tri normals.
    for (uint32_t i = 0; i < mFloorMeshVertices.size(); i++) {
        Vertex &funcVert = mFunctionMeshVertices[i];

        glm::dvec3 avgNormal = {0.0f, 0.0f, 0.0f};
        for (uint32_t vertTriIdx : mVertexTriangles[i]) {
            const Triangle &vertTri = mFunctionMeshTriangles[vertTriIdx];
            avgNormal += vertTri.area * glm::dvec3(vertTri.normal);
        }
        avgNormal = glm::normalize(avgNormal);

        glm::dvec3 numNormal = normalAtPoint(funcVert.pos);
        double secondDeriv   = secondDerivEstMax(funcVert.pos);
        double t             = mSecondDerivCutoff(secondDeriv);
        glm::dvec3 normal    = glm::normalize(t * numNormal + (1.0 - t) * avgNormal);

        if constexpr (DEV_DEBUG) {
            spdlog::trace("Vertex pos 2nd deriv est: {}", secondDeriv);
            spdlog::trace("- t = : {}", t);
        }

        // Here we use the second derivate estimate to deicde how much
        // to interpolate this averaged triangle normal with the directly-
        // computed normal. (See the note below.)

        // Use Gram-Schmidt to get ONB.
        glm::dvec3 tangent = glm::normalize(xDir - glm::dot(xDir, normal) * normal);
        glm::dvec3 bitangent =
            glm::normalize(zDir - glm::dot(zDir, normal) * normal - glm::dot(zDir, tangent) * tangent);

        // Verify orientation and orthonormality.
        constexpr float TRIPLE_ERROR_TOLERANCE = 0.01f;
        float scalarTriple                     = glm::dot(normal, glm::cross(bitangent, tangent));
        if (std::abs(scalarTriple - 1.0f) >= TRIPLE_ERROR_TOLERANCE) {
            std::cout << "TBN scalar triple product: " << std::to_string(scalarTriple) << std::endl;
        }
        assert(std::abs(scalarTriple - 1.0f) < TRIPLE_ERROR_TOLERANCE);

        funcVert.tangent   = glm::vec3(tangent);
        funcVert.bitangent = glm::vec3(bitangent);
        funcVert.normal    = glm::vec3(normal);
    }
}

// NOTE:
//
// There is a visual artifact where there are bright spots on steep
// parts of the surface that are parallel to the x- or z-axis when the
// normal is computed directly from the numerical derivatives.
// It seems to be due to a combination of the mesh grid shape, the
// function shape, and numerical stability issues in our computations.
//
// On the other hand, the method of averaging the triangle normals
// above looks worse in parts of the graph where the curvature is high,
// which may be due to averaging with normal computed from farther-away
// vertices.
//
// The setFuncVertTBNs function now tries to interpolate between each
// version of the normals based on the magnitude of the second
// derivative of the function as a measure of the surface curvature.
//
// Either way, there is a limit to how much accuracy we can get when the
// features of the function are changing fast compared to the distance
// between vertices, due to the effect of fragment interpolation. Another
// idea is to compute the normals at a higher-resolution grid of points
// and put them into a multidimensional texture that the fragment shader
// can sample.

glm::dvec3 FunctionMesh::normalAtPoint(const glm::vec3 &pos) {
    double x = pos.x;
    double z = pos.z;

    double dydx = (mFunc(x + H, z) - mFunc(x - H, z)) / (2.0 * H);
    double dydz = (mFunc(x, z + H) - mFunc(x, z - H)) / (2.0 * H);

    return glm::normalize(glm::dvec3(-dydx, 1.0, -dydz));
}

void FunctionMesh::setFuncVertTBNsDirect() {
    spdlog::trace("Setting vertex TBN vectors using direct method...");

    for (Vertex &vert : mFunctionMeshVertices) {
        // Convert to double precision for computation.
        double x = vert.pos.x;
        double z = vert.pos.z;

        double dydx = (mFunc(x + H, z) - mFunc(x - H, z)) / (2.0 * H);
        double dydz = (mFunc(x, z + H) - mFunc(x, z - H)) / (2.0 * H);

        glm::dvec3 tx     = glm::normalize(glm::dvec3(1.0, dydx, 0.0));
        glm::dvec3 tz     = glm::normalize(glm::dvec3(0.0, dydz, 1.0));
        glm::dvec3 normal = glm::normalize(glm::dvec3(-dydx, 1.0, -dydz));

        // Sanity check.
        constexpr float ORTHO_ERROR_TOLERANCE = 1e-8;
        double txDotN                         = glm::dot(tx, normal);
        double tzDotN                         = glm::dot(tz, normal);
        if (std::abs(txDotN) > ORTHO_ERROR_TOLERANCE || std::abs(tzDotN) > ORTHO_ERROR_TOLERANCE) {
            spdlog::debug("Vertex TBN vectors failed orthogonality check.");
        }

        vert.tangent   = glm::vec3(tx);
        vert.bitangent = glm::vec3(tz);
        vert.normal    = glm::vec3(normal);
    }
}

// New method. Once complete will replace old methods.
void FunctionMesh::computeVerticesAndIndices() {
    mFloorMeshVertices.clear();
    mFloorMeshVertices.reserve((NUM_CELLS + 1) * (NUM_CELLS + 1) + NUM_CELLS * NUM_CELLS);

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
        if (square.topLeftIdx == UINT32_MAX) {
            addFloorMeshVertex(square.mTopLeft[0], square.mTopLeft[1]);
            square.topLeftIdx = mFloorMeshVertices.size() - 1;
        }
        if (square.topRightIdx == UINT32_MAX) {
            addFloorMeshVertex(square.mBtmRight[0], square.mTopLeft[1]);
            square.topRightIdx = mFloorMeshVertices.size() - 1;
        }
        if (square.bottomRightIdx == UINT32_MAX) {
            addFloorMeshVertex(square.mBtmRight[0], square.mBtmRight[1]);
            square.bottomRightIdx = mFloorMeshVertices.size() - 1;
        }
        if (square.bottomLeftIdx == UINT32_MAX) {
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
            refine(square); // TODO: There is a memory problem here.
        }
        [[maybe_unused]] auto &_ = square.populateRefinements();
    }

    // Update edge refinements from neighbors to make mesh water tight.
    for (auto &square : mFloorMeshSquares) {
        syncEdgeRefinements(square); // TODO: There is a memory problem here.
    }

    mMeshIndices.clear();
    // NOTE: This size will not be accurate if refinement happens, but it
    //       is okay if this reallocates as nothing references its data.
    mMeshIndices.reserve(mFloorMeshSquares.size() * 12);
    mFunctionMeshTriangles.clear();
    mVertexTriangles.resize(mFunctionMeshVertices.size());

    // Create triangles for squares.
    for (auto &square : mFloorMeshSquares) {
        addSquareTris(square);
    }

    // Create indices for triangles.
    for (const Triangle &tri : mFunctionMeshTriangles) {
        addTriIndices(tri);
    }

    if constexpr (DIRECT_NORMALS) {
        setFuncVertTBNsDirect();
    } else {
        setFuncVertTBNs();
    }
}

static std::vector<uint32_t> *getNorthNbRefinements(Square *square) {
    while (square != nullptr) {
        if (square->northNeighbor != nullptr) {
            return &square->northNeighbor->edgeRefinements.south;
        } else {
            square = square->parent;
        }
    }
    return nullptr;
}
static std::vector<uint32_t> *getSouthNbRefinements(Square *square) {
    while (square != nullptr) {
        if (square->southNeighbor != nullptr) {
            return &square->southNeighbor->edgeRefinements.north;
        } else {
            square = square->parent;
        }
    }
    return nullptr;
}
static std::vector<uint32_t> *getEastNbRefinements(Square *square) {
    while (square != nullptr) {
        if (square->eastNeighbor != nullptr) {
            return &square->eastNeighbor->edgeRefinements.west;
        } else {
            square = square->parent;
        }
    }
    return nullptr;
}
static std::vector<uint32_t> *getWestNbRefinements(Square *square) {
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
void FunctionMesh::syncRefmtsHoriz(std::vector<uint32_t> &to, std::vector<uint32_t> &from) {
    assert(!to.empty());

    auto getX = [this](size_t index) -> float {
        return mFloorMeshVertices[index].pos.x; //
    };

    float leftLim  = getX(to.front());
    float rightLim = getX(to.back());

    for (uint32_t fromIdx : from) {
        float fromX = getX(fromIdx);
        if (leftLim < fromX && fromX < rightLim) {
            to.push_back(fromIdx);
        }
    }

    // Now re-sort by x-coord and remove duplicates.
    std::sort(to.begin(), to.end(), [getX](uint32_t a, uint32_t b) {
        return getX(a) < getX(b); //
    });
    auto last = std::unique(to.begin(), to.end(), [getX](uint32_t a, uint32_t b) {
        return getX(a) == getX(b); //
    });
    to.erase(last, to.end());
}

// Precondition: to and from are sorted by increasing z.
void FunctionMesh::syncRefmtsVert(std::vector<uint32_t> &to, std::vector<uint32_t> &from) {
    assert(!to.empty());

    auto getZ = [this](size_t index) -> float {
        return mFloorMeshVertices[index].pos.z; //
    };

    float leftLim  = getZ(to.front());
    float rightLim = getZ(to.back());

    for (uint32_t fromIdx : from) {
        float fromX = getZ(fromIdx);
        if (leftLim < fromX && fromX < rightLim) {
            to.push_back(fromIdx);
        }
    }

    // Now re-sort by z-coord and remove duplicates.
    std::sort(to.begin(), to.end(), [getZ](uint32_t a, uint32_t b) {
        return getZ(a) < getZ(b); //
    });
    auto last = std::unique(to.begin(), to.end(), [getZ](uint32_t a, uint32_t b) {
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
        syncRefmtsHoriz(square.edgeRefinements.north, *northNbRefmnts);
    }

    auto southNbRefmnts = getSouthNbRefinements(&square);
    if (southNbRefmnts != nullptr && southNbRefmnts->size() > 2) {
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

#ifndef MESH_DEBUG_H_
#define MESH_DEBUG_H_

#include "function_mesh.h"

#include <filesystem>
#include <format>
#include <fstream>

struct Box {
    double topLeftX     = 0.0;
    double topLeftZ     = 0.0;
    double bottomRightX = 0.0;
    double bottomRightZ = 0.0;
};

class MeshDebug {
    FunctionMesh mFuncMesh;

public:
    MeshDebug(FunctionMesh &&mesh)
        : mFuncMesh{std::forward<FunctionMesh>(mesh)} {
    }

    void meshVG(const std::filesystem::path &outfile, Box boundingBox) {
        constexpr int svgDim = 1000;

        double xStretch = 1.0 / (boundingBox.bottomRightX - boundingBox.topLeftX);
        double zStretch = 1.0 / (boundingBox.topLeftZ - boundingBox.bottomRightZ);

        boundingBox.topLeftX *= svgDim;
        boundingBox.topLeftZ *= svgDim;
        boundingBox.bottomRightX *= svgDim;
        boundingBox.bottomRightZ *= svgDim;

        std::ofstream file;
        file.open(outfile);
        file << R"(<svg width="1000" height="1000" xmlns="http://www.w3.org/2000/svg">)" << std::endl;

        auto addTri = [this, &file, boundingBox, xStretch, zStretch](const Triangle &meshTri) {
            auto coords1 = mFuncMesh.meshXZ(meshTri.vert1Idx);
            double x1    = coords1.x * svgDim;
            double z1    = coords1.z * svgDim;
            auto coords2 = mFuncMesh.meshXZ(meshTri.vert2Idx);
            double x2    = coords2.x * svgDim;
            double z2    = coords2.z * svgDim;
            auto coords3 = mFuncMesh.meshXZ(meshTri.vert3Idx);
            double x3    = coords3.x * svgDim;
            double z3    = coords3.z * svgDim;

            double xMin = std::min({x1, x2, x3});
            double xMax = std::max({x1, x2, x3});
            double zMin = std::min({z1, z2, z3});
            double zMax = std::max({z1, z2, z3});

            // Reject outside bounding box.
            if (xMin < boundingBox.topLeftX || xMax > boundingBox.bottomRightX || //
                zMin < boundingBox.bottomRightZ || zMax > boundingBox.topLeftZ) {
                return;
            }

            // Shift and rescale so bounding box fills image.
            x1 = (x1 - boundingBox.topLeftX) * xStretch;
            z1 = (z1 - boundingBox.bottomRightZ) * zStretch;
            x2 = (x2 - boundingBox.topLeftX) * xStretch;
            z2 = (z2 - boundingBox.bottomRightZ) * zStretch;
            x3 = (x3 - boundingBox.topLeftX) * xStretch;
            z3 = (z3 - boundingBox.bottomRightZ) * zStretch;

            file << std::format(R"(  <polygon points="{},{} {},{} {},{}" fill="none" stroke="green" />)", //
                                x1, z1, x2, z2, x3, z3)
                 << std::endl;
        };

        for (const auto &tri : mFuncMesh.mFunctionMeshTriangles) {
            addTri(tri);
        }

        file << R"(</svg>)" << std::endl;
        file.close();
    }
};

#endif // MESH_DEBUG_H_

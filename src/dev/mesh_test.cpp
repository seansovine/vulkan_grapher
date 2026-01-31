#include <function_mesh.h>

#include <mesh_debug.h>

#include <algorithm>
#include <limits>
#include <string>

#include <spdlog/spdlog.h>

[[maybe_unused]]
static auto TEST_FUNCTION_PARABOLIC = [](double x, double y) -> double {
    return 1.0 - (x - 0.5) * (x - 0.5) - (y - 0.5) * (y - 0.5);
};

static auto sinc = [](double x, double y) -> double {
    double scale = 50; // 100
    double mag   = scale * std::sqrt(x * x + y * y);
    return mag == 0.0 ? 1.0 : std::sin(mag) / mag;
};
static auto TEST_FUNCTION_SHIFTED_SINC = [](double x, double y) -> double {
    return 0.75 * sinc(x - 0.5, y - 0.5) + 0.25; //
};

int main() {
    spdlog::set_level(spdlog::level::trace);
    spdlog::info("Testing function mesh generation.");

    FunctionMesh mesh{TEST_FUNCTION_SHIFTED_SINC};
    spdlog::info("Squares in top-level tessellation: {}", std::to_string(mesh.tessellationSquare().size()));

    float maxY = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    for (const auto &vertex : mesh.functionVertices()) {
        maxY = std::max(maxY, vertex.pos.y);
        minY = std::min(minY, vertex.pos.y);
    }

    spdlog::info("Function mesh max y: {}", std::to_string(maxY));
    spdlog::info("Function mesh min y: {}", std::to_string(minY));

    if (false) {
        spdlog::info(mesh.debugMesh());
    }

    MeshDebug meshDebug{std::move(mesh)};

    [[maybe_unused]]
    Box boundingBox1{
        .topLeftX     = 0.25, //
        .topLeftZ     = 0.5,  //
        .bottomRightX = 0.5,  //
        .bottomRightZ = 0.25  //
    };
    [[maybe_unused]]
    Box boundingBox2{
        .topLeftX     = 0.375, //
        .topLeftZ     = 0.5,   //
        .bottomRightX = 0.5,   //
        .bottomRightZ = 0.375  //
    };

    meshDebug.meshVG("scratch/mesh_test.svg", boundingBox2);
}

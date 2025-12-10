#include <function_mesh.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <string>

static auto TEST_FUNCTION_PARABOLIC = [](double x, double y) -> double {
    return 1.0 - (x - 0.5) * (x - 0.5) - (y - 0.5) * (y - 0.5);
};

static auto sinc = [](double x, double y) -> double {
    double scale = 50; // 100
    double mag = scale * std::sqrt(x * x + y * y);
    return mag == 0.0 ? 1.0 : std::sin(mag) / mag;
};
static auto TEST_FUNCTION_SHIFTED_SINC = [](double x, double y) -> double {
    return 0.75 * sinc(x - 0.5, y - 0.5) + 0.25; //
};

int main() {
    std::cout << "Testing function mesh generation." << std::endl;

    FunctionMesh mesh{TEST_FUNCTION_SHIFTED_SINC};
    std::cout << "Squares in tessellation: " << std::to_string(mesh.tessellationSquare().size()) << std::endl;

    float maxY = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    for (const auto &vertex : mesh.functionVertices()) {
        maxY = std::max(maxY, vertex.pos.y);
        minY = std::min(minY, vertex.pos.y);
    }

    std::cout << "Function mesh max y: " << std::to_string(maxY) << std::endl;
    std::cout << "Function mesh min y: " << std::to_string(minY) << std::endl;

    std::cout << mesh.debugMesh() << std::endl;
}

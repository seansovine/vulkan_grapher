#include <function_mesh.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <string>

static auto TEST_FUNCTION = [](double x, double y) -> double {
    return 1.0 - (x - 0.5) * (x - 0.5) - (y - 0.5) * (y - 0.5);
};

int main() {
    std::cout << "Testing function mesh generation." << std::endl;

    FunctionMesh mesh{TEST_FUNCTION};
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

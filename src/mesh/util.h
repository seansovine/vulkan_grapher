#ifndef MESH_UTIL_H_
#define MESH_UTIL_H_

#include <algorithm>
#include <cmath>
#include <format>
#include <utility>

#include <glm/fwd.hpp>
#include <glm/geometric.hpp>

// Numerically stable Heron's formula found on Low Latency Trading Insights substack.
[[maybe_unused]]
static double triangleArea(double len1, double len2, double len3) {
    // Sort lengths.
    if (len2 > len1) {
        std::swap(len2, len1);
    }
    if (len3 > len2) {
        std::swap(len3, len2);
    }
    if (len2 > len1) {
        std::swap(len2, len1);
    }

    // Degenerate case.
    if (len1 >= len2 + len3) {
        return 0.0;
    }

    // Heron's formula with ordered operations.
    return 0.25 * std::sqrt(                   //
                      (len1 + (len2 + len3)) * //
                      (len3 - (len1 - len2)) * //
                      (len3 + (len1 - len2)) * //
                      (len1 + (len2 - len3))   //
                  );
}

[[maybe_unused]]
static std::string debugGlmVecTrunc(glm::dvec3 vec) {
    return std::format("({:.6f}, {:.6f}, {:.6f})", vec.x, vec.y, vec.z);
};

[[maybe_unused]]
static std::string debugGlmVec(glm::dvec3 vec) {
    return std::format("({}, {}, {})", vec.x, vec.y, vec.z);
};

// Logistic function shifted and scaled for use as a cutoff in
// interpolation, since its values increase smoothly from near 0 to
// near 1 over the interval (center - width / 2, center + width / 2).
class LogisticCutoff {
    const double center;
    const double width;

public:
    constexpr LogisticCutoff(double center, double width)
        : center{center},
          width{width} {
    }

    double operator()(double t) const {
        return 1.0 / (1.0 + std::exp(-12.0 * (t - center) / width));
    }
};

#endif // MESH_UTIL_H_

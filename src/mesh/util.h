#ifndef MESH_UTIL_H_
#define MESH_UTIL_H_

#include "user_function.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <utility>

#include <glm/fwd.hpp>
#include <glm/geometric.hpp>

namespace math_util {

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

[[maybe_unused]]
static auto TEST_FUNCTION_PARABOLIC(double x, double z) -> double {
    return 0.75 - (x - 0.5) * (x - 0.5) - (z - 0.5) * (z - 0.5);
};

static auto sinc(double x, double z) -> double {
    constexpr double scale = 30; // 100
    double mag             = scale * std::sqrt(x * x + z * z);
    return mag == 0.0 ? 1.0 : std::sin(mag) / mag;
};
[[maybe_unused]]
static auto TEST_FUNCTION_SHIFTED_SINC(double x, double z) -> double {
    return 0.75 * sinc(x - 0.5, z - 0.5) + 0.25; //
};

static auto expSine(double x, double z) -> double {
    return std::pow(std::numbers::e, -std::sin(x * x + z * z));
};
static auto TEST_FUNCTION_SHIFTED_SCALED_EXP_SINE = [](double x, double z) -> double {
    constexpr double scale = 8.0;
    return 0.125 * expSine(scale * (x - 0.5), scale * (z - 0.5));
};

static UserFunction TEST_FUNCTION_SHIFTED_SCALED_EXP_SINE_USER_ = {
    "0.75 * sin(30.0 * sqrt(x * x + z * z)) / (30.0 * sqrt(x * x + z * z)) + 0.25"};

static auto TEST_FUNCTION_SHIFTED_SCALED_EXP_SINE_USER = [](double x, double z) -> double {
    return TEST_FUNCTION_SHIFTED_SCALED_EXP_SINE_USER_(x, z);
};

} // namespace math_util

#endif // MESH_UTIL_H_

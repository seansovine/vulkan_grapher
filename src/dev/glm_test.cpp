// Testing out some numerical computations
// with GLM and the standard library.

#include <util.h>

#include <cmath>
#include <cstdlib>
#include <format>
#include <iostream>
#include <numbers>

#include <glm/fwd.hpp>
#include <glm/geometric.hpp>

/**
 * Our function is: 0.75 * sinc(30 * ||(x - 0.5, z - 0.5)||) + 0.25
 */

static auto sinc = [](double x, double z) -> double {
    constexpr double scale = 30; // 100
    double mag             = scale * std::hypot(x, z);
    return mag == 0.0 ? 1.0 : std::sin(mag) / mag;
};
static auto mFunc = [](double x, double z) -> double {
    return 0.75 * sinc(x - 0.5, z - 0.5) + 0.25; //
};

/**
 * Compute analytically derived derivatives of function.
 */

static auto dydxA = [](double x, double z) -> double {
    double xs = x - 0.5;
    double zs = z - 0.5;
    return 0.75 * xs * (std::cos(30.0 * std::hypot(xs, zs)) - sinc(xs, zs)) / (xs * xs + zs * zs);
};

static auto dydzA = [](double x, double z) -> double {
    double xs = x - 0.5;
    double zs = z - 0.5;
    return 0.75 * zs * (std::cos(30.0 * std::hypot(xs, zs)) - sinc(xs, zs)) / (xs * xs + zs * zs);
};

// Test funcs.

void glmCrossTest(const glm::dvec3 &tx, const glm::dvec3 &tz);

void derivativeEstimate(double r);

// Main.

int main() {
    constexpr double delta = 0.05;
    double r               = 0.5;

    for (int i = 0; i < 10; ++i) {
        derivativeEstimate(r);
        r -= delta;
    }
}

// Test func defs.

void derivativeEstimate(double r) {
    std::cout << "========================\n\n";
    std::cout << std::format("r = {}\n\n", r);

    constexpr double H     = 10e-6;
    constexpr double theta = 0.5 * std::numbers::pi;

    const double x = 0.5 + r * std::sin(theta);
    const double z = 0.5 + r * std::cos(theta);

    std::cout << std::format("y : {}\n", x);
    std::cout << std::format("z : {}\n\n", z);

    double dydx = (mFunc(x + H, z) - mFunc(x - H, z)) / (2.0 * H);
    double dydz = (mFunc(x, z + H) - mFunc(x, z - H)) / (2.0 * H);

    std::cout << std::format("dydx estimate: {}\n", dydx);
    std::cout << std::format("dydz estimate: {}\n\n", dydz);

    double dydx_a = dydxA(x, z);
    double dydz_a = dydzA(x, z);

    std::cout << std::format("Analytic dydx: {}\n", dydx_a);
    std::cout << std::format("Analytic dydz: {}\n\n", dydz_a);

    // Report appeoximation error compared to computing derivate from
    // known expression. Tells roughly how good our finite differnence
    // approximation is.
    std::cout << std::format("Analytic-numeric error: {}\n", std::abs(dydx - dydx_a));
    std::cout << std::format("Analytic-numeric error: {}\n\n", std::abs(dydz - dydz_a));

    if constexpr (false) {
        glm::dvec3 tx = glm::normalize(glm::dvec3(1.0, dydx, 0.0));
        glm::dvec3 tz = glm::normalize(glm::dvec3(0.0, dydz, 1.0));
        glmCrossTest(tx, tz);
    }
}

void crossExample() {
    glm::dvec3 tx(0.3152908477409399, 0.9489950902564246, 0);
    glm::dvec3 tz(0, -0.961188655455929, 0.2758919509931805);
    glmCrossTest(tx, tz);
}

[[maybe_unused]]
void glmCrossTest(const glm::dvec3 &tx, const glm::dvec3 &tz) {
    std::cout << std::format("Computed Tx: {}\n", debugGlmVec(tx));
    std::cout << std::format("Computed Tz: {}\n\n", debugGlmVec(tz));

    std::cout << std::format("Tx length: {}\n", glm::length(tx));
    std::cout << std::format("Tz length: {}\n\n", glm::length(tz));

    glm::dvec3 normal = glm::cross(tz, tx);
    std::cout << std::format("GLM computed normal: {}\n\n", debugGlmVec(normal));

    double normalLen = glm::length(normal);
    std::cout << std::format("Normal length: {}\n\n", normalLen);

    std::cout << std::format("Tx dot normal: {}\n", glm::dot(tx, normal));
    std::cout << std::format("Tz dot normal: {}\n\n", glm::dot(tz, normal));

    // Sanity check.

    // Tx = (1.0, dydx, 0.0) / norm.
    // Tz = (0.0, dydz, 1.0) / norm.

    double cx = tx.z * tz.y - tx.y * tz.z;
    double cy = tx.x * tz.z - tx.z * tz.x;
    double cz = tx.y * tz.x - tx.x * tz.y;

    std::cout << std::format("Manually computed normal: {}\n\n", debugGlmVec(glm::dvec3(cx, cy, cz)));

    double cx1 = tx.z * tz.y;
    double cx2 = tx.y * tz.z;
    double cy1 = tx.x * tz.z;
    double cy2 = tx.z * tz.x;
    double cz1 = tx.y * tz.x;
    double cz2 = tx.x * tz.y;

    cx = cx1 - cx2;
    cy = cy1 - cy2;
    cz = cz1 - cz2;

    std::cout << "Computation is:\n";
    std::cout << std::format("- cx: {} - {}\n", cx1, cx2);
    std::cout << std::format("- cy: {} - {}\n", cy1, cy2);
    std::cout << std::format("- cz: {} - {}\n\n", cz1, cz2);

    std::cout << std::format("Normal computed in steps: {}\n\n", debugGlmVec(glm::dvec3(cx, cy, cz)));
}

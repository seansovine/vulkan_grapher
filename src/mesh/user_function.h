#ifndef USER_FUNCTION_H_
#define USER_FUNCTION_H_

#include <array>
#include <cmath>
#include <stdexcept>
#include <string>

#include <mathpresso/mathpresso.h>

class BadExpression : public std::runtime_error {
public:
    BadExpression()
        : std::runtime_error("Failed to parse expression.") {
    }
};

class UserFunction {
    mathpresso::Context ctx;
    mathpresso::Expression exp;

    static constexpr double DEFAULT_MESH_RADIUS = 10e-3;
    double meshRadius                           = DEFAULT_MESH_RADIUS;

public:
    UserFunction() = default;

    UserFunction(const std::string &expression, double meshRadius = DEFAULT_MESH_RADIUS)
        : meshRadius{meshRadius} {
        assign(expression);
    }

    void assign(const std::string &expression) {
        ctx.add_builtins();
        ctx.add_variable("x", 0 * sizeof(double));
        ctx.add_variable("z", 1 * sizeof(double));

        mathpresso::Error err = exp.compile(ctx, expression.c_str(), mathpresso::kNoOptions);

        if (err != mathpresso::kErrorOk) {
            throw BadExpression();
        }
    }

    double operator()(double x, double z) {
        if (!exp.is_compiled()) {
            throw std::runtime_error("Cannot evaluate with no assigned expression.");
        }

        double data[] = {x, z};
        double result = exp.evaluate(data);
        if (std::isnan(result)) {
            return approximateSingularity(x, z);
        }
        return result;
    }

private:
    double approximateSingularity(double x, double z) {
        std::array<double, 2> data = {x - meshRadius, z};
        double v1                  = exp.evaluate(data.data());
        data                       = {x + meshRadius, z};
        double v2                  = exp.evaluate(data.data());
        data                       = {x, z - meshRadius};
        double v3                  = exp.evaluate(data.data());
        data                       = {x, z + meshRadius};
        double v4                  = exp.evaluate(data.data());
        return (v1 + v2 + v3 + v4) / 4.0;
    }
};

#endif // USER_FUNCTION_H_

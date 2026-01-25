#include <mathpresso/mathpresso.h>

#include <cmath>
#include <format>
#include <iostream>
#include <stdexcept>

int main(int argc, char *argv[]) {
    mathpresso::Context ctx;
    mathpresso::Expression exp;

    ctx.add_builtins();
    ctx.add_variable("x", 0 * sizeof(double));
    ctx.add_variable("z", 1 * sizeof(double));

    mathpresso::Error err = exp.compile(ctx, "sin(sqrt(x * x + z * z)) / sqrt(x * x + z * z)", mathpresso::kNoOptions);

    // Handle possible syntax or compilation error.
    if (err != mathpresso::kErrorOk) {
        throw std::runtime_error(std::format("Expression Error: {}\n", err));
        return 1;
    }

    double data[] = {0.5, -0.5};
    double result = exp.evaluate(data);

    std::cout << std::format("f(0.5, -0.5) = {:.4}\n", result);

    data[0] = 0.0;
    data[1] = 0.0;
    result  = exp.evaluate(data);

    // Convert NaNs to zero.
    result = std::isnan(result) ? result : 0.0;

    std::cout << std::format("f(0.0,  0.0) = {:.4}\n", result);

    return 0;
}

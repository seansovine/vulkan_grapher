#include "application.h"

#include <iostream>

#include <spdlog/common.h>
#include <spdlog/spdlog.h>

int main() {
    spdlog::set_level(spdlog::level::trace);
    Application app;

    try {
        app.run();
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

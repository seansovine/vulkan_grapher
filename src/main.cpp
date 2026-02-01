#include "application.h"

#include <spdlog/spdlog.h>

int main() {
    std::locale::global(std::locale(""));
    spdlog::set_level(spdlog::level::trace);

    Application app;
    try {
        app.run();
    } catch (std::exception &e) {
        spdlog::error("An error happened: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

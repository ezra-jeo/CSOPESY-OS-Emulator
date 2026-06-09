#include "core/Application.h"
#include <cstdio>
#include <stdexcept>

int main() {
    try {
        core::Application app;
        app.run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Fatal: %s\n", e.what());
        return 1;
    }
    return 0;
}

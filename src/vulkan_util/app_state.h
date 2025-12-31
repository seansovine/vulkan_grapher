#ifndef APP_STATE_H_
#define APP_STATE_H_

#include <glm/fwd.hpp>
#include <glm/glm.hpp>

#include <cstdint>

// This is just here for testing mesh updating,
// until we have a math expression parser added.
enum class TestFunc : uint8_t {
    Parabolic   = 0,
    ShiftedSinc = 1,
    NUM_FUNCS   = 2,
};

struct AppState {
    bool rotating        = true;
    bool wireframe       = false;
    bool pbrFragPipeline = true;

    glm::vec3 graphColor = {0.0f, 0.13f, 0.94f};

    float metallic  = 0.15;
    float roughness = 0.27;

    TestFunc testFunc = TestFunc::ShiftedSinc;

    void toggleTestFunc() {
        testFunc =
            static_cast<TestFunc>((static_cast<uint8_t>(testFunc) + 1) % static_cast<uint8_t>(TestFunc::NUM_FUNCS));
    }
};

#endif // APP_STATE_H_

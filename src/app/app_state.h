#ifndef APP_STATE_H_
#define APP_STATE_H_

#include <GLFW/glfw3.h>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>

#include <cstdint>

// This is just here for testing mesh updating,
// until we have a math expression parser added.
enum class TestFunc : uint8_t {
    Parabolic   = 0,
    ShiftedSinc = 1,
    ExpSine     = 2,
    NUM_FUNCS   = 3,
};

struct AppState {
    bool rotating        = true;
    bool wireframe       = false;
    bool pbrFragPipeline = true;
    bool drawFloor       = true;

    glm::vec3 graphColor = {0.0f, 0.13f, 0.94f};

    float metallic  = 0.15;
    float roughness = 0.27;

    TestFunc testFunc = TestFunc::ShiftedSinc;

    void toggleTestFunc() {
        uint8_t numFuncs = static_cast<uint8_t>(TestFunc::NUM_FUNCS);
        testFunc         = static_cast<TestFunc>((static_cast<uint8_t>(testFunc) + 1) % numFuncs);
    }
};

class WindowEvents {
    // Mouse events.
    bool leftMousePressed = false;
    double lastMouseX;
    double lastMouseY;

    // Keyboard state.
    bool controlPressed = false;

    // Let ImGui capture some events.
    bool imGuiWantsMouse = false;

public:
    void initGlfw(GLFWwindow *window) {
        glfwSetWindowUserPointer(window, this);
    }

    void setGuiWantsInputs(bool wantsMouse) {
        imGuiWantsMouse = wantsMouse;
    }

public:
    static WindowEvents *getThisPtr(GLFWwindow *window) {
        return static_cast<WindowEvents *>(glfwGetWindowUserPointer(window));
    }

    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
        WindowEvents *thisPtr = getThisPtr(window);
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS && !thisPtr->imGuiWantsMouse) {
                // TODO: Get ImGui io and check it wants event.
                thisPtr->leftMousePressed = true;
                glfwGetCursorPos(window, &thisPtr->lastMouseX, &thisPtr->lastMouseX);
            } else if (action == GLFW_RELEASE) {
                thisPtr->leftMousePressed = false;
            }
        }
    }

    static void mousePositionCallback(GLFWwindow *window, double xpos, double ypos) {
        WindowEvents *thisPtr = getThisPtr(window);
        if (thisPtr->leftMousePressed) {
            double dx = xpos - thisPtr->lastMouseX;
            double dy = ypos - thisPtr->lastMouseY;
            // TODO: Use to update camera based on modifier state.

            thisPtr->lastMouseX = xpos;
            thisPtr->lastMouseY = ypos;
        }
    }
};

#endif // APP_STATE_H_

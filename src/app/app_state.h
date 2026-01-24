#ifndef APP_STATE_H_
#define APP_STATE_H_

#include <GLFW/glfw3.h>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>

#include <cstdint>
#include <utility>

// Built-in functions; we will add an expression parser.
enum class TestFunc : uint8_t {
    Parabolic   = 0,
    ShiftedSinc = 1,
    ExpSine     = 2,
    NUM_FUNCS   = 3,
};

struct AppState {
    // Function selection.
    TestFunc testFunc = TestFunc::ShiftedSinc;

    // Render preferences.
    bool rotating        = true;
    bool wireframe       = false;
    bool pbrFragPipeline = true;
    bool drawFloor       = true;

    // Mesh parameters.
    glm::vec3 graphColor = {0.0f, 0.13f, 0.94f};
    float metallic       = 0.15;
    float roughness      = 0.27;

    // User interaction state.
    bool mouseInteracting = false;
    double xUserRot       = 0.0;
    double yUserRot       = 0.0;
    double userScroll     = 0.0;

public:
    void toggleTestFunc() {
        uint8_t numFuncs = static_cast<uint8_t>(TestFunc::NUM_FUNCS);
        testFunc         = static_cast<TestFunc>((static_cast<uint8_t>(testFunc) + 1) % numFuncs);
    }

    std::pair<double, double> takeUserRotation() {
        std::pair<double, double> ret = {xUserRot, yUserRot};
        xUserRot                      = 0.0;
        yUserRot                      = 0.0;
        return ret;
    }

    double takeUserScroll() {
        double scroll = userScroll;
        userScroll    = 0.0;
        return scroll;
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

    AppState *appState = nullptr;

public:
    void init(GLFWwindow *window, AppState *inAppState) {
        glfwSetWindowUserPointer(window, this);
        appState = inAppState;
        glfwSetMouseButtonCallback(window, WindowEvents::mouseButtonCallback);
        glfwSetCursorPosCallback(window, WindowEvents::mousePositionCallback);
        glfwSetScrollCallback(window, WindowEvents::mouseScrollCallback);
    }

    void setGuiWantsInputs(bool wantsMouse) {
        imGuiWantsMouse = wantsMouse;
    }

    void applyMousePositionChange(double dx, double dy) {
        if (!controlPressed) {
            appState->xUserRot = dx;
            appState->yUserRot = dy;
        }
        // TODO: Control + drag will do translation.
    }

    void applyScrollChange(double dy) {
        appState->userScroll = dy;
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
                thisPtr->leftMousePressed           = true;
                thisPtr->appState->mouseInteracting = true;
                glfwGetCursorPos(window, &thisPtr->lastMouseX, &thisPtr->lastMouseX);
            } else if (action == GLFW_RELEASE) {
                thisPtr->leftMousePressed           = false;
                thisPtr->appState->mouseInteracting = false;
            }
        }
    }

    static void mousePositionCallback(GLFWwindow *window, double xpos, double ypos) {
        WindowEvents *thisPtr = getThisPtr(window);
        double dx             = xpos - thisPtr->lastMouseX;
        double dy             = ypos - thisPtr->lastMouseY;
        thisPtr->lastMouseX   = xpos;
        thisPtr->lastMouseY   = ypos;
        if (thisPtr->leftMousePressed) {
            thisPtr->applyMousePositionChange(dx, dy);
        }
    }

    static void mouseScrollCallback(GLFWwindow *window, double _dx, double dy) {
        WindowEvents *thisPtr = getThisPtr(window);
        thisPtr->applyScrollChange(dy);
    }
};

#endif // APP_STATE_H_

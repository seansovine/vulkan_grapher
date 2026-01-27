#ifndef APP_STATE_H_
#define APP_STATE_H_

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include <GLFW/glfw3.h>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>

enum class TestFunc : uint8_t {
    Parabolic   = 0,
    ShiftedSinc = 1,
    ExpSine     = 2,
    UserInput   = 3,
    NUM_FUNCS   = 4,
};

static constexpr std::array<const char *, static_cast<size_t>(TestFunc::NUM_FUNCS)> funcNames = {
    "Parabolic",   //
    "Sinc",        //
    "Exp of sine", //
    "User input",  //
};

struct UserInput {
    double xUserRot   = 0.0;
    double yUserRot   = 0.0;
    double userScroll = 0.0;
    double xUserTrans = 0.0;
    double yUserTrans = 0.0;
};

struct AppState {
    // Function selection.
    TestFunc testFunc = TestFunc::ShiftedSinc;

    // User function input.
    static constexpr size_t INPUT_BUFFER_LEN               = 1024;
    std::array<char, INPUT_BUFFER_LEN> functionInputBuffer = {0};

    // Render preferences.
    bool rotating        = false;
    bool wireframe       = false;
    bool pbrFragPipeline = true;
    bool drawFloor       = false;
    bool resetPosition   = false;

    // Mesh parameters.
    glm::vec3 graphColor = {0.0f, 0.13f, 0.94f};
    float metallic       = 0.15;
    float roughness      = 0.27;

    // User interaction state.
    bool mouseInteracting = false;
    UserInput userInput   = {};

public:
    void toggleTestFunc() {
        uint8_t numFuncs = static_cast<uint8_t>(TestFunc::NUM_FUNCS);
        testFunc         = static_cast<TestFunc>((static_cast<uint8_t>(testFunc) + 1) % numFuncs);
    }

    UserInput takeUserInput() {
        UserInput returnVal = userInput;
        userInput           = {};
        return returnVal;
    }

    size_t selectedFuncIndex() {
        return static_cast<size_t>(testFunc);
    }
};

class WindowEvents {
    // Mouse events.
    bool leftMousePressed = false;
    double lastMouseX;
    double lastMouseY;

    // Keyboard state.
    bool controlDown = false;
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
        glfwSetKeyCallback(window, WindowEvents::keyboardCallback);
    }

    void setGuiWantsInputs(bool wantsMouse) {
        imGuiWantsMouse = wantsMouse;
    }

    void applyMousePositionChange(double dx, double dy) {
        // Clamp to work around apparent glfw bug causing large jumps.
        if (!controlDown) {
            constexpr double MAX_ALLOWED_ROTATION = 20.0;
            appState->userInput.xUserRot          = std::clamp(dx, -MAX_ALLOWED_ROTATION, MAX_ALLOWED_ROTATION);
            appState->userInput.yUserRot          = std::clamp(dy, -MAX_ALLOWED_ROTATION, MAX_ALLOWED_ROTATION);
        } else {
            constexpr double MAX_ALLOWED_MOVE = 20.0;
            appState->userInput.xUserTrans    = std::clamp(dx, -MAX_ALLOWED_MOVE, MAX_ALLOWED_MOVE);
            appState->userInput.yUserTrans    = std::clamp(dy, -MAX_ALLOWED_MOVE, MAX_ALLOWED_MOVE);
        }
    }

    void applyScrollChange(double dy) {
        appState->userInput.userScroll = dy;
    }

public:
    static WindowEvents *getThisPtr(GLFWwindow *window) {
        return static_cast<WindowEvents *>(glfwGetWindowUserPointer(window));
    }

    static void mouseButtonCallback(GLFWwindow *window, int button, int action, [[maybe_unused]] int mods) {
        WindowEvents *thisPtr = getThisPtr(window);
        if (thisPtr->imGuiWantsMouse) {
            return;
        }
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
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
        if (thisPtr->imGuiWantsMouse) {
            return;
        }
        if (thisPtr->leftMousePressed) {
            double dx           = xpos - thisPtr->lastMouseX;
            double dy           = ypos - thisPtr->lastMouseY;
            thisPtr->lastMouseX = xpos;
            thisPtr->lastMouseY = ypos;
            thisPtr->applyMousePositionChange(dx, dy);
        }
    }

    static void mouseScrollCallback(GLFWwindow *window, [[maybe_unused]] double dx, double dy) {
        WindowEvents *thisPtr = getThisPtr(window);
        if (thisPtr->imGuiWantsMouse) {
            return;
        }
        thisPtr->applyScrollChange(dy);
    }

    static void keyboardCallback(GLFWwindow *window, int key, [[maybe_unused]] int scancode, int action,
                                 [[maybe_unused]] int mods) {
        if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        WindowEvents *thisPtr = getThisPtr(window);
        if (action == GLFW_PRESS && key == 341) {
            thisPtr->controlDown = true;
        } else if (action == GLFW_RELEASE && key == 341) {
            thisPtr->controlDown = false;
        }
    }
};

#endif // APP_STATE_H_

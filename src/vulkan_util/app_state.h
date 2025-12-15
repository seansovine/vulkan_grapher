#ifndef APP_STATE_H_
#define APP_STATE_H_

#include <glm/fwd.hpp>
#include <glm/glm.hpp>

struct AppState {
    bool rotating  = true;
    bool wireframe = true;

    glm::vec3 graphColor = {0.070f, 0.336f, 0.594f};
};

#endif // APP_STATE_H_

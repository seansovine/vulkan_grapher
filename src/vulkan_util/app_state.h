#ifndef APP_STATE_H_
#define APP_STATE_H_

#include <glm/fwd.hpp>
#include <glm/glm.hpp>

struct AppState {
    bool rotating        = true;
    bool wireframe       = false;
    bool pbrFragPipeline = true;

    glm::vec3 graphColor = {0.070f, 0.336f, 0.594f};

    float metallic  = 0.5;
    float roughness = 0.5;
};

#endif // APP_STATE_H_

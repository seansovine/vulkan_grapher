#version 450
#extension GL_ARB_separate_shader_objects : enable

// Uniforms.
layout(set = 0, binding = 0) uniform CameraUniform {
    mat4 view;
    mat4 proj;
    vec3 viewerPos;
} cameraUbo;

layout(set = 1, binding = 0) uniform ModelUniform {
    mat4 model;
    vec3 meshColor;
    float _roughness;
    float _metallic;
} modelUbo;

// Inputs.
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec3 inNormal;

// Outputs.
layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = cameraUbo.proj * cameraUbo.view * modelUbo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}

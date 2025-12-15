#version 450
#extension GL_ARB_separate_shader_objects : enable

// Uniforms.
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;

    // Currently unused by this pipeline.
    vec3 meshColor;
} ubo;

// Inputs.
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

// Outputs.
layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}

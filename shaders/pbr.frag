#version 450
#extension GL_ARB_separate_shader_objects : enable

// Inputs.

layout(location = 0) in vec3 vInColor;

// Outputs.

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(vInColor, 1.0);
}

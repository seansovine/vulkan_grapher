#version 450
#extension GL_ARB_separate_shader_objects : enable

// Uniforms.

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;

    vec3 viewerPos;
    vec3 meshColor;
} ubo;

// Inputs.

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec3 inNormal;

// Outputs.

struct VertexOut {
    vec3 color;
    vec3 tangentLightPos;
    vec3 tangentViewPos;
    vec3 tangentFragPos;
};

layout(location = 0) out VertexOut vOut;

// Constants.

const vec3 lightPos = vec3(-2.0, 6.0, 0.0);

void main() {
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    vOut.color = ubo.meshColor;

    mat3 TBNt = transpose(mat3(inTangent, inBitangent, inNormal));
    vOut.tangentLightPos = TBNt * lightPos;
    vOut.tangentViewPos = TBNt * ubo.viewerPos;
    vOut.tangentFragPos = vec3(worldPos);
}

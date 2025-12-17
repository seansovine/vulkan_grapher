#version 450
#extension GL_ARB_separate_shader_objects : enable

// Uniforms.

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;

    vec3 viewerPos;
    vec3 _meshColor;

    float _roughness;
    float _metallic;
} ubo;

// Inputs.

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec3 inNormal;

// Outputs.

struct VertexOut {
    vec3 tangentLightOffset;
    vec3 tangentViewOffset;
};

layout(location = 0) out VertexOut vOut;

// Constants.

const vec3 lightPos = vec3(-1.0, 5.0, 1.0);

void main() {
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;

    mat3 modelRot = mat3(ubo.model);
    vec3 T = modelRot * inTangent;
    vec3 B = modelRot * inBitangent;
    vec3 N = modelRot * inNormal;

    mat3 TBNt = transpose(mat3(T, B, N));
    vOut.tangentLightOffset = TBNt * (lightPos - vec3(worldPos));
    vOut.tangentViewOffset = TBNt * (ubo.viewerPos - vec3(worldPos));
}

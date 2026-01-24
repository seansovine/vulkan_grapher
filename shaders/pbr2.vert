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
    vec3 _meshColor;
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

struct VertexOut {
    vec3 tangentLightOffset[2];
    vec3 tangentViewOffset;
};

layout(location = 0) out VertexOut vOut;

// Constants.

const vec3 lightPos[2] = {vec3(-2.0, 5.0, 2.0), vec3(2.0, 5.0, 2.0)};

void main() {
    vec4 worldPos = modelUbo.model * vec4(inPosition, 1.0);
    gl_Position = cameraUbo.proj * cameraUbo.view * worldPos;

    mat3 modelRot = mat3(modelUbo.model);
    vec3 T = modelRot * inTangent;
    vec3 B = modelRot * inBitangent;
    vec3 N = modelRot * inNormal;

    mat3 TBNt = transpose(mat3(T, B, N));
    for (int i = 0; i < 2; ++i)
    {
        vOut.tangentLightOffset[i] = TBNt * (lightPos[i] - vec3(worldPos));
    }
    vOut.tangentViewOffset = TBNt * (cameraUbo.viewerPos - vec3(worldPos));
}

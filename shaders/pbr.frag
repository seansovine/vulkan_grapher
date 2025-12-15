#version 450
#extension GL_ARB_separate_shader_objects : enable

// Inputs.

struct VertexOut {
    vec3 color;
    vec3 tangentLightPos;
    vec3 tangentViewPos;
    vec3 tangentFragPos;
};

layout(location = 0) in VertexOut vIn;

// Outputs.

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir = normalize(vIn.tangentViewPos - vIn.tangentFragPos);
    vec3 viewDir = normalize(vIn.tangentLightPos - vIn.tangentFragPos);

    // Simple diffuse + ambient lighting for testing.
    float lightDotNormal = max(dot(lightDir, vec3(0.0, 0.0, 1.0)), 0.1);
    outColor = vec4(lightDotNormal * vIn.color, 1.0);
}

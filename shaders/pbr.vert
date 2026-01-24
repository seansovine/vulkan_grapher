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
    float roughness;
    float metallic;
} modelUbo;

// Inputs.

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 _inColor;
layout(location = 2) in vec3 _inTangent;
layout(location = 3) in vec3 _inBitangent;
layout(location = 4) in vec3 inNormal;

// Outputs.

layout(location = 0) out vec3 vOutColor;

// Constants.

const float PI = 3.14159265359;
const vec3 lightPos[2] = {vec3(-1.0, 5.0, 1.0), vec3(1.0, 4.0, 1.0)};

// Cited as RGB for noon sunlight.
const vec3 lightColors[2] = {vec3(1.0, 1.0, 0.9843), vec3(1.0, 1.0, 0.9843)};

// ---------

// Credit: The code here is adapted from the example on learnopengl.com.

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

void main() {
    vec3 worldPos = vec3(modelUbo.model * vec4(inPosition, 1.0));
    vec3 V = normalize(cameraUbo.viewerPos - worldPos);
    vec3 N = mat3(modelUbo.model) * inNormal;

    vec3 albedo = modelUbo.meshColor;
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, modelUbo.metallic);

    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 2; ++i)
    {
        vec3 L = normalize(lightPos[i] - worldPos);
        vec3 H = normalize(V + L);
        float distance    = length(L);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = lightColors[i] * attenuation;

        float NDF = DistributionGGX(N, H, modelUbo.roughness);
        float G   = GeometrySmith(N, V, L, modelUbo.roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - modelUbo.metallic;

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo;
    vec3 color = ambient + Lo;

    vOutColor = color;
    gl_Position = cameraUbo.proj * cameraUbo.view * vec4(worldPos, 1.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

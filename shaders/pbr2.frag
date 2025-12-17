#version 450
#extension GL_ARB_separate_shader_objects : enable

// Uniforms.

layout(binding = 0) uniform UniformBufferObject {
    mat4 _model;
    mat4 _view;
    mat4 _proj;

    vec3 _viewerPos;
    vec3 meshColor;

    float roughness;
    float metallic;
} ubo;

// Inputs.

struct VertexOut {
    vec3 tangentLightOffset;
    vec3 tangentViewOffset;
};

layout(location = 0) in VertexOut vIn;

// Outputs.

layout(location = 0) out vec4 outColor;

// Constants.

const float PI = 3.14159265359;

// Cited as RGB for noon sunlight.
const vec3 lightColor = vec3(1.0, 1.0, 0.9843);

// For now we give everything an outward unit normal.
const vec3 N = vec3(0.0, 0.0, 1.0);

// ---------

// Credit: The code here is adapted from the example on learnopengl.com.

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

void main() {
    vec3 V = normalize(vIn.tangentViewOffset);

    vec3 albedo = ubo.meshColor;
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, ubo.metallic);

    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 1; ++i) // TODO: Make array of lights.
    {
        vec3 L = normalize(vIn.tangentLightOffset);
        vec3 H = normalize(V + L);
        float distance    = length(L);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = lightColor * attenuation;

        float NDF = DistributionGGX(N, H, ubo.roughness);
        float G   = GeometrySmith(N, V, L, ubo.roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - ubo.metallic;

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo;
    outColor = vec4(ambient + Lo, 1.0);
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

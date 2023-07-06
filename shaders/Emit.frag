#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    vec3 DlightDir;		// direction of the direct light
    vec3 DlightColor;	// color of the direct light
    vec3 AmbLightColor;	// ambient light
    vec3 eyePos;		// position of the viewer
    float usePointLight;
} gubo;

layout(set = 1, binding = 0) uniform UniformBufferObject {
    float amb;
    float sigma;
    mat4 mvpMat;
    mat4 mMat;
    mat4 nMat;
    vec3 offset;
} ubo;

layout(set = 1, binding = 1) uniform sampler2D tex;
layout(set = 1, binding = 2) uniform sampler2D texEmit;

/*
1) LIGHTING: DIRECT vs POINT (vs SPOT)

DIRECT LIGHT
vec3 lightDir = gubo.lightDir;
vec3 lightColor = gubo.lightColor.rgb;

POINT LIGHT
vec3 lightDir = normalize(gubo.eyePos - fragPos);
vec3 lightColor = vec3(gubo.lightColor) * pow((g / length(gubo.eyePos - fragPos)), beta);

SPOT LIGHT
vec3 lightDir = normalize(gubo.eyePos - fragPos);
vec3 arg = (normalize(gubo.eyePos - fragPos) * gubo.lightDir - cosout) / (cosin - cosout);
vec3 lightColor = vec3(gubo.lightColor) * pow((g / length(gubo.eyePos - fragPos)), beta) * clamp(arg, cosout, cosin);

2) BRDF: LAMBERT + PHONG/BLINN vs OREN-NAYAR vs COOK-TORRANCE

3) AMBIENT: STANDARD vs HEMISPHERIC vs IMAGE-BASED

*/

vec3 BRDF(vec3 V, vec3 N, vec3 L, vec3 Md, float sigma) {
    //vec3 V  - direction of the viewer [aka OmegaR]
    //vec3 N  - normal vector to the surface
    //vec3 L  - light vector (from the light model) [aka d]
    //vec3 Md - main color of the surface
    //float sigma - Roughness of the model

    // used long computation method
    float lAlongN = dot(L, N);
    float vAlongN = dot(V, N);
    float tetaI = acos(lAlongN);
    float tetaR = acos(vAlongN);
    float alpha = max(tetaI, tetaR);
    float beta = min(tetaI, tetaR);
    float sigma2 = pow(sigma, 2.0);
    float A = 1 - 0.5 * sigma2 / (sigma2 + 0.33);
    float B = 0.45 * sigma2 / (sigma2 + 0.09);

    vec3 vI = normalize(L - lAlongN * N);
    vec3 vR = normalize(V - vAlongN * N);

    float G = max(0, dot(vI, vR));
    vec3 LightDir = Md * clamp(lAlongN, 0.0, 1.0);

    return LightDir * (A + B * G * sin(alpha) * tan(beta));
}

const float beta = 2.0f;
const float g = 1.0f;
const vec3 pointLightPos = vec3(-64.0, 100.0, -64.0);

vec3 pointLightDir() {
    return normalize(pointLightPos - fragPos);
}

vec3 pointLightColor() {
    return gubo.DlightColor.rgb * pow((g / length(gubo.eyePos - fragPos)), beta);
}

void main() {
    // DIRECT LIGHT
    vec3 lightDir = (gubo.usePointLight == 1.0)? pointLightDir() : normalize(gubo.DlightDir); // AKA l
    vec3 lightColor = (gubo.usePointLight == 1.0)? pointLightColor() : gubo.DlightColor.rgb;

    vec3 albedo = texture(tex, fragUV).rgb;
    // OREN-NAYAR
    vec3 normal = normalize(fragNorm); // AKA n
    vec3 diffuseColor = albedo; // AKA mD - surface diffuse color
    vec3 eyeDir = normalize(gubo.eyePos - fragPos); // AKA V, v, omegaR

    // STANDARD - AMBIENT LIGHTING
    vec3 mAmbient = albedo * ubo.amb;
    vec3 lAmbient = gubo.AmbLightColor;
    vec3 ambient = lAmbient * mAmbient;

    // EMISSION
    vec3 emission = gubo.usePointLight * texture(texEmit, fragUV).rgb;		// emission color

    // ADDING EVERYTHING
    vec3 reflection = BRDF(eyeDir, normal, lightDir, albedo, ubo.sigma); // last parameter is roughness
    outColor = vec4(clamp(reflection * lightColor + ambient + emission,0.0,1.0), 1.0f);
}
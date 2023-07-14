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
    float useAltLight;
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
const vec3 altLightPos = vec3(-64.0, 100.0, -64.0);
const float cosout = 0.5;
const float cosin  = 0.95;

vec3 pointLightDir() {
    return normalize(altLightPos - fragPos);
}

vec3 pointLightColor() {
    return gubo.DlightColor.rgb * pow((g / length(altLightPos - fragPos)), beta);
}

vec3 altLightColor(vec3 lightDir) {
    float arg = (dot(normalize(altLightPos - fragPos), lightDir) - cosout) / (cosin - cosout);
    vec3 lightColor = gubo.DlightColor.rgb * pow((g / length(altLightPos - fragPos)), beta) * clamp(arg, 0.0, 1.0);
    return lightColor;
}

void main() {
    // DIRECT or SPOT LIGHT (POINT disabled)
    vec3 lightDir = /*(gubo.useAltLight == 1.0)? pointLightDir() :*/ normalize(gubo.DlightDir); // AKA l
    vec3 lightColor = (gubo.useAltLight == 1.0)? /*pointLightColor()*/ altLightColor(lightDir) : gubo.DlightColor.rgb;

    vec3 albedo = texture(tex, fragUV).rgb;
    // OREN-NAYAR
    vec3 normal = normalize(fragNorm); // AKA n
    vec3 diffuseColor = albedo; // AKA mD - surface diffuse color
    vec3 eyeDir = normalize(gubo.eyePos - fragPos); // AKA V, v, omegaR

    // STANDARD - AMBIENT LIGHTING
    float amb = (gubo.useAltLight == 1.0)? ubo.amb / 4.0 : ubo.amb;
    vec3 mAmbient = albedo * amb;
    vec3 lAmbient = gubo.AmbLightColor;
    vec3 ambient = lAmbient * mAmbient;

    // EMISSION
    vec3 emission = gubo.useAltLight * texture(texEmit, fragUV).rgb;		// emission color

    // ADDING EVERYTHING
    vec3 reflection = BRDF(eyeDir, normal, lightDir, albedo, ubo.sigma); // last parameter is roughness
    outColor = vec4(clamp(reflection * lightColor + ambient + emission,0.0,1.0), 1.0f);
}
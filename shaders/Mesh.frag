#version 450#extension GL_ARB_separate_shader_objects : enablelayout(location = 0) in vec3 fragPos;layout(location = 1) in vec3 fragNorm;layout(location = 2) in vec2 fragUV;layout(location = 0) out vec4 outColor;layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {	vec3 DlightDir;		// direction of the direct light	vec3 DlightColor;	// color of the direct light	vec3 AmbLightColor;	// ambient light	vec3 eyePos;		// position of the viewer} gubo;layout(set = 1, binding = 0) uniform UniformBufferObject {	float amb;	float gamma;	vec3 sColor;	mat4 mvpMat;	mat4 mMat;	mat4 nMat;} ubo;layout(set = 1, binding = 1) uniform sampler2D tex;/*1) LIGHTING: DIRECT vs POINT (vs SPOT)DIRECT LIGHTvec3 lightDir = gubo.lightDir;vec3 lightColor = gubo.lightColor.rgb;POINT LIGHTvec3 lightDir = normalize(gubo.eyePos - fragPos);vec3 lightColor = vec3(gubo.lightColor) * pow((g / length(gubo.eyePos - fragPos)), beta);SPOT LIGHTvec3 lightDir = normalize(gubo.eyePos - fragPos);vec3 arg = (normalize(gubo.eyePos - fragPos) * gubo.lightDir - cosout) / (cosin - cosout);vec3 lightColor = vec3(gubo.lightColor) * pow((g / length(gubo.eyePos - fragPos)), beta) * clamp(arg, cosout, cosin);2) BRDF: LAMBERT + PHONG/BLINN vs OREN-NAYAR vs COOK-TORRANCE3) AMBIENT: STANDARD vs HEMISPHERIC vs IMAGE-BASED*/void main() {	// DIRECT LIGHT	vec3 lightDir = normalize(gubo.DlightDir); // AKA l	vec3 lightColor = gubo.DlightColor.rgb;	vec3 albedo = texture(tex, fragUV).rgb;	// LAMBERT - BRDF diffuse reflection - diffuseBRDF(l, n, v, mD)	vec3 normal = normalize(fragNorm); // AKA n	vec3 diffuseColor = albedo; // AKA mD - surface diffuse color	vec3 diffuse = diffuseColor * clamp(dot(lightDir, normal), 0.0f, 1.0f);	// [unused] PHONG - BRDF specular reflection - specularBRDF(l, n, v, mD)	vec3 specularColor = ubo.sColor; // AKA mS - should be vec3(1) for standard object or = diffuseColor for metallic objects	vec3 eyeDir = normalize(gubo.eyePos - fragPos); // AKA V, v, omegaR	// vec3 reflectDirection = - reflect(lightDir, normal); // direction of the reflected ray	// vec3 specular = specularColor * pow(clamp(dot(eyeDir, reflectDirection), 0.0f, 1.0f), ubo.gamma);	// BLINN - BRDF specular reflection (alternative: more expesive and more "reflective")	vec3 halfVector = normalize(lightDir + eyeDir);	vec3 specular = specularColor * pow(clamp(dot(normal, halfVector), 0.0f, 1.0f), gamma);	// AMBIENT LIGHTING	vec3 mAmbient = albedo * ubo.amb;	vec3 lAmbient = gubo.AmbLightColor;	vec3 ambient = lAmbient * mAmbient;	// ADDING EVERYTHING	vec3 reflection = diffuse + specular;	outColor = vec4(clamp(reflection * lightColor + ambient,0.0,1.0), 1.0f);}
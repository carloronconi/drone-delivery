#version 450#extension GL_ARB_separate_shader_objects : enablelayout(location = 0) in vec3 fragPos;layout(location = 0) out vec4 outColor;layout(binding = 0) uniform UniformBufferObject {	float visible;	mat4 mvpMat;	vec3 offset;	float time;} ubo;const vec4 colorDark = vec4(0.4f, 0.4f, 0.4f, 0.7f);const vec4 colorLight = vec4(0.8f, 0.8f, 0.8f, 0.7f);void main() {	// change color every 10th of a second	outColor = mod(int(20.0 * ubo.time), 2) == 0 ? colorDark : colorLight;}
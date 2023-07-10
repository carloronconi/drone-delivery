#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	float visible;
	mat4 mvpMat;
	vec3 offset;
	float time;
} ubo;

layout(location = 0) in vec3 inPosition;

void main() {
	vec3 offset = ubo.offset * gl_InstanceIndex;
	gl_Position = ubo.mvpMat * vec4((inPosition + offset) * ubo.visible, 1.0f);
}
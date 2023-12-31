#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	float visible;
	mat4 mvpMat;
	vec2 offset;
	float instancesToDraw;
// add vec2 translation (use: inPosition * translation * ubo.visible...)
// or even a mat4 mvpMat for complete rotations (use: = mvpMat * vec4(inPosition...)
// or even better a mat? for only transl and rot in 2 dimensions

// also add float depth param to be put in place of 0.5f to have some
// overlay components placed over others
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 outUV;

void main() {
	vec2 offset = ubo.offset * gl_InstanceIndex;
	float showInstance = clamp(ubo.instancesToDraw - gl_InstanceIndex, 0.0f, 1.0);
	float visible = ubo.visible * showInstance;
	gl_Position = ubo.mvpMat * vec4(inPosition * visible + offset, 0.5f, 1.0f);
	outUV = inUV;
}
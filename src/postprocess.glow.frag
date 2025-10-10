#version 450

layout(location = 0) in vec2 inUV;

layout(binding = 0) uniform sampler2D originalSampler;
layout(binding = 1) uniform sampler2D glowSampler;

layout(location = 0) out vec4 outColor;


void main() {
    vec4 originalColor = texture(originalSampler, inUV);
    vec4 glowColor = texture(glowSampler, inUV);
    outColor = originalColor + glowColor;
}


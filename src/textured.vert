#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 tex_uv;

layout(binding = 0) uniform UniformBufferObject {
    mat4 viewport;
} ubo;

layout(location = 0) out vec2 uv;

void main() {
    gl_Position = ubo.viewport * vec4(position, 0.0, 1.0);
    uv = tex_uv;
}

#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 tex_uv;
layout(location = 2) in uint tex_id;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 viewport;
} ubo;

layout(location = 0) out vec2 uv;
layout(location = 1) out uint sampler_id;

void main() {
    gl_Position = ubo.viewport * vec4(position, 0.0, 1.0);
    uv = tex_uv;
    sampler_id = tex_id;
}

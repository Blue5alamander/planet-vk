#version 450

layout(set = 1, binding = 0) uniform sampler2D[1024] tex_sampler;

layout(location = 0) in vec2 tex_uv;
layout(location = 1) flat in uint sampler_id;

layout(location = 0) out vec4 colour;

void main() {
    colour = texture(tex_sampler[sampler_id] , tex_uv);
}

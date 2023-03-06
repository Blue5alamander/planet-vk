#version 450

layout(binding = 0, set = 1) uniform sampler2D tex_sampler;

layout(location = 0) in vec2 tex_uv;

layout(location = 0) out vec4 colour;

void main() {
    colour = texture(tex_sampler , tex_uv);
}

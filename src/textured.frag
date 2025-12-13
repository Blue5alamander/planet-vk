#version 450

layout(set = 1, binding = 0) uniform sampler2D tex_sampler;

layout(location = 0) in vec2 tex_uv;
layout(location = 1) in vec4 hint_colour;


layout(location = 0) out vec4 colour;


void main() {
    colour = texture(tex_sampler , tex_uv) * hint_colour;
}

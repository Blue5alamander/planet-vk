#version 450

layout(set = 1, binding = 0) uniform sampler2D tex_sampler;

layout(location = 0) in vec2 tex_uv;
layout(location = 1) in vec4 hint_colour;


layout(location = 0) out vec4 colour;
layout(location = 1) out vec4 glow;


void main() {
    vec4 tx = texture(tex_sampler, tex_uv);
    colour = vec4(tx.xyz * hint_colour.xyz * hint_colour.w, tx.w);
    glow = vec4(tx.xyz * hint_colour.xyz * hint_colour.w * tx.w, 1);
}


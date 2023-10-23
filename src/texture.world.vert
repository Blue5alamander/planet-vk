#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 tex_colour;
layout(location = 2) in vec2 tex_uv;

layout(set = 0, binding = 0) uniform CoordinateSpace {
    mat4 world;
    mat4 pixel;
} coordinates;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 colour;

void main() {
    gl_Position = coordinates.world * position;
    uv = tex_uv;
    colour = tex_colour;
}

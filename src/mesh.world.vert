#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform CoordinateSpace {
    mat4 world;
    mat4 screen;
    mat4 perspective;
} coordinates;

void main() {
    gl_Position = coordinates.perspective * coordinates.world * inPosition;
    fragColor = inColor;
}

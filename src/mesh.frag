#version 450

layout(location = 0) in vec4 frag_color;

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 glow;

void main() {
    color = frag_color;
    glow = vec4(0.0, 0.0, 0.0, color.a);
}

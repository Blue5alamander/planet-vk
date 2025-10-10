#version 450

layout(location = 0) in vec2 inUV;

layout(binding = 0) uniform sampler2D inputSampler;

layout(location = 0) out vec4 outColor;


void horizontal() {
    vec2 texSize = textureSize(inputSampler, 0);
    float pixelSize = 1.0 / texSize.x;

    const float weights[11] = float[](0.0222, 0.0456, 0.0798, 0.1191, 0.1514, 0.164, 0.1514, 0.1191, 0.0798, 0.0456, 0.0222);
    const int offsets[11] = int[](-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5);

    vec4 color = vec4(0.0);
    for (int i = 0; i < 11; ++i) {
        vec2 offsetUV = inUV + vec2(float(offsets[i]) * pixelSize, 0.0);
        color += texture(inputSampler, offsetUV) * weights[i];
    }

    outColor = color;
}


void vertical() {
    vec2 texSize = textureSize(inputSampler, 0);
    float pixelSize = 1.0 / texSize.y;

    const float weights[11] = float[](0.0222, 0.0456, 0.0798, 0.1191, 0.1514, 0.164, 0.1514, 0.1191, 0.0798, 0.0456, 0.0222);
    const int offsets[11] = int[](-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5);

    vec4 color = vec4(0.0);
    for (int i = 0; i < 11; ++i) {
        vec2 offsetUV = inUV + vec2(0.0, float(offsets[i]) * pixelSize);
        color += texture(inputSampler, offsetUV) * weights[i];
    }

    outColor = color;
}

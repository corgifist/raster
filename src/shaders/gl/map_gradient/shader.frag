#version 310 es

#ifdef GL_ES
precision highp float;
#endif

#define MAGIC_SMOOTHER 0.0618

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform bool uInvert;
uniform float uOpacity;

uniform sampler2D uColorTexture;
uniform sampler2D uUVTexture;

struct GradientStop {
    float percentage;
    vec4 color;
};

layout(std430, binding = 0) readonly buffer GradientSSBO {
    float gradientData[];
};

GradientStop getStop(int index) {
    GradientStop result;
    int gradientIndex = 1 + index * 5;
    result.percentage = gradientData[gradientIndex];
    result.color = vec4(gradientData[gradientIndex + 1], gradientData[gradientIndex + 2], gradientData[gradientIndex + 3], gradientData[gradientIndex + 4]);
    return result;
}

vec4 createGradient(in float y) {
    vec4 color = getStop(0).color;
    int iGradientsCount = int(gradientData[0]);
    for (int i = 1; i < iGradientsCount; i++) {
        GradientStop currentStop = getStop(i);
        GradientStop previousStop = getStop(i - 1);
        color = mix(color, currentStop.color, smoothstep(previousStop.percentage, currentStop.percentage + MAGIC_SMOOTHER, y + MAGIC_SMOOTHER)); 
    }
    return color;
}

void main() {
    vec2 screenUV = gl_FragCoord.xy / uResolution;
    vec4 texel = texture(uColorTexture, screenUV);
    float grayscale = (texel.r + texel.g + texel.b) / 3.0f;
    gColor = createGradient(uInvert ? 1.0 - grayscale : grayscale);
    gColor.a *= uOpacity;
    screenUV -= 0.5;
    screenUV.x *= uResolution.x / uResolution.y;
    gUV = vec4(screenUV, uResolution.x / uResolution.y, 1.);
}
#version 310 es

#ifdef GL_ES
precision highp float;
#endif

#define MAGIC_SMOOTHER 0.0618

// This shader was taken from https://www.shadertoy.com/view/NtScz1
// And modified in order to be compatible with Raster
// Many thanks to https://www.shadertoy.com/user/zsjasper !


layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform bool uUseYPlane;
uniform float uOpacity;

uniform sampler2D uColorTexture;
uniform sampler2D uUVTexture;
uniform bool uScreenSpaceRendering;

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
    gColor = vec4(0.);
    if (uScreenSpaceRendering) {
        gColor = createGradient(uUseYPlane ? screenUV.y : screenUV.x);
        gUV = vec4(screenUV, 1., 1.);
    } else {
        vec4 uvPixel = texture(uUVTexture, screenUV);
        if (uvPixel.a != 0.0) {
            vec2 uv = uvPixel.rg;
            vec4 colorPixel = texture(uColorTexture, screenUV);
            if (colorPixel.a != 0.0) {
                uv.x /= uvPixel.z;
                uv += 0.5;
                gColor = createGradient(uUseYPlane ? uv.y : uv.x);
                gUV = vec4(uv, uvPixel.b, 1.);
            }
        }
    }
    gColor.a *= uOpacity;
}
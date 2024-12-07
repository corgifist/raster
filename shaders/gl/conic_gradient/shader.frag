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

uniform vec2 uPosition;
uniform float uAngle;
uniform float uOpacity;

uniform sampler2D uColorTexture;
uniform sampler2D uUVTexture;
uniform bool uScreenSpaceRendering;

vec2 rotate(vec2 v, float angle) {
    return vec2(
            v.x * cos(angle) - v.y * sin(angle),
            v.x * sin(angle) + v.y * cos(angle)
    );
}


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
    vec2 correctPosition = uPosition;
    correctPosition.x *= -1.0;
    if (uScreenSpaceRendering) {
        vec2 uv = screenUV;
        uv -= 0.5;
        uv.x *= uResolution.x / uResolution.y;
        vec2 relative = rotate(uv + correctPosition, radians(-uAngle));

        float angle = atan(relative.x, relative.y);
        float t = (angle + 3.14) / 2.0 / 3.14;
        gColor = createGradient(t);
        gColor.a *= uOpacity;
        gUV = vec4(uv, uResolution.x / uResolution.y, 1.);
    } else {
        vec4 uvPixel = texture(uUVTexture, screenUV);
        if (uvPixel.a != 0.0) {
            vec2 uv = uvPixel.rg;
            vec4 colorPixel = texture(uColorTexture, screenUV);
            if (colorPixel.a != 0.0) {
                uv.x /= uvPixel.z;
                vec2 relative = rotate(uv + correctPosition, radians(-uAngle));

                float angle = atan(relative.x, relative.y);
                float t = (angle + 3.14) / 2.0 / 3.14;
                gColor = createGradient(t);
                gColor = mix(texture(uColorTexture, screenUV), gColor, uOpacity);
                gUV = uvPixel;
            }
        }
    }
    gColor.a *= uOpacity;
}
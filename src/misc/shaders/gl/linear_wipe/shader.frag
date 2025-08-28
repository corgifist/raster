#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;
uniform sampler2D uColorTexture;
uniform sampler2D uUVTexture;
uniform bool uScreenSpaceRendering;
uniform float uOpacity;

uniform vec4 uAColor, uBColor;
uniform float uPhase;
uniform bool uUseYPlane;
uniform bool uInvertPhase;
uniform bool uSwapColors;

vec4 generateNoise(vec2 uv) {
    vec4 a = uAColor;
    vec4 b = uBColor;
    if (uSwapColors) {
        b = uAColor;
        a = uBColor;
    }
    float phase = uPhase;
    if (uInvertPhase) phase = 1.0 - phase;
    float plane = uUseYPlane ? uv.y : uv.x;
    if (phase > plane) return a;
    return b;
}

void main()
{
    vec2 screenUV = gl_FragCoord.xy / uResolution;
    if (uScreenSpaceRendering) {
        vec2 correctedUV = screenUV;
        correctedUV.x *= uResolution.x / uResolution.y;
        gColor = generateNoise(correctedUV);
        gColor.a *= uOpacity;
        screenUV -= 0.5;
        screenUV.x *= uResolution.x / uResolution.y;
        gUV = vec4(screenUV, uResolution.x / uResolution.y, 1.0);
    } else {
        vec4 uvPixel = texture(uUVTexture, screenUV);
        if (uvPixel.a != 0.0) {
            vec4 colorPixel = texture(uColorTexture, screenUV);
            if (colorPixel.a != 0.0) {
                vec2 uv = vec2(uvPixel.x / uvPixel.z, uvPixel.y);
                uv += 0.5;
                gColor = mix(colorPixel, generateNoise(uv), uOpacity);
                gUV = uvPixel;
            }
        }
    }
}

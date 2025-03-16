#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform sampler2D uColor;
uniform sampler2D uMaskColor;
uniform sampler2D uUV;
uniform float uOpacity;
uniform bool uInvert;

uniform bool uMaskAvailable;

void main() {
    vec2 screenUV = gl_FragCoord.xy / uResolution;
    vec4 mask = texture(uMaskColor, screenUV);
    if (!uMaskAvailable) {
        mask = vec4(1.);
    }
    if (uInvert) {
        mask = 1.0 - mask;
    }
    gColor = texture(uColor, screenUV) * mix(vec4(1.), mask, uOpacity);
    // gColor = vec4(mask);
    gUV = texture(uUV, screenUV) * mix(vec4(1.), mask, uOpacity);
}
#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform bool uFlipX;
uniform bool uFlipY;
uniform sampler2D uColor;
uniform sampler2D uUV;

void main() {
    vec2 flipC = gl_FragCoord.xy / uResolution;
    if (uFlipX) flipC.x = 1.0 - flipC.x;
    if (uFlipY) flipC.y = 1.0 - flipC.y;
    vec4 texel = texture(uColor, flipC);
    gColor = texture(uColor, flipC);
    gUV = texture(uUV, flipC);
}
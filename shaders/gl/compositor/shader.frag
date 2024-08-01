#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform sampler2D uColor;
uniform sampler2D uUV;

uniform float uOpacity;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    gColor = texture(uColor, uv);
    gUV = texture(uUV, uv);
    gColor.a *= uOpacity;
    if (gColor.a == 0.0) discard;
}
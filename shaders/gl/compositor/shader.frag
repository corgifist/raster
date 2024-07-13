#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform sampler2D uColor;
uniform sampler2D uUV;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    gColor = texture2D(uColor, uv);
    gUV = texture2D(uUV, uv);
    if (gColor.a == 0.0) discard;
}
#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;
uniform int uOp;

uniform sampler2D uBase;
uniform sampler2D uMask;
uniform sampler2D uUV;

void main() {
    vec2 screenUV = gl_FragCoord.xy / uResolution;
    gColor = texture(uBase, screenUV) * texture(uMask, screenUV);
}
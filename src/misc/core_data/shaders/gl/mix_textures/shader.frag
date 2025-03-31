#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;

uniform vec2 uResolution;

uniform float uPhase;
uniform sampler2D uA;
uniform sampler2D uB;

void main() {
    vec2 p = gl_FragCoord.xy / uResolution.xy;
    gColor = mix(texture(uA, p), texture(uB, p), uPhase);
}
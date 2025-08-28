#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;

uniform vec2 uResolution;

uniform sampler2D uR;
uniform sampler2D uG;
uniform sampler2D uB;
uniform sampler2D uA;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    gColor = vec4(texture(uR, uv).r, texture(uG, uv).g, texture(uB, uv).b, texture(uA, uv).a);
}
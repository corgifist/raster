#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform float uGamma;
uniform sampler2D uColor;
uniform sampler2D uUV;
uniform int uUVAvailable;

void main() {
    vec4 texel = texture(uColor, gl_FragCoord.xy / uResolution);
    gColor = vec4(pow(texel.rgb, vec3(1.0 / uGamma)), texel.a);
    if (uUVAvailable == 1) gUV = texture(uUV, gl_FragCoord.xy / uResolution);
}
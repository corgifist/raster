#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform float uOpacity;
uniform vec2 uResolution;

uniform sampler2D uColorTexture;
uniform sampler2D uUVTexture;

void main() {
    gColor = texture(uColorTexture, gl_FragCoord.xy / uResolution);
    gColor.a *= uOpacity;

    gUV = texture(uUVTexture, gl_FragCoord.xy / uResolution);
    gUV.a *= uOpacity;
}
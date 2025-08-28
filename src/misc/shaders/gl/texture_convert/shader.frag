#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;

uniform vec2 uResolution;

uniform sampler2D uTexture;

void main() {
    gColor = texture(uTexture, gl_FragCoord.xy / uResolution);
}
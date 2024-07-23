#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

in vec2 vUV;

uniform vec4 uColor;

uniform bool uTextureAvailable;
uniform sampler2D uTexture;

void main() {
    gColor = uColor;
    if (uTextureAvailable) gColor *= texture(uTexture, vUV);
    gUV = vec4(vUV, 0, 1);
}
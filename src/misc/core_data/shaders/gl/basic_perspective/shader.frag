#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

in vec2 vUV;

uniform bool uTextureAvailable;
uniform sampler2D uTexture;

uniform float uAspectRatio;

void main() {
    gColor = vec4(1.0);
    if (uTextureAvailable) gColor *= texture(uTexture, vUV);
    
    gUV = vec4(vUV, uAspectRatio, 1);
}
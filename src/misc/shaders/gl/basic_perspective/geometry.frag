#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gPos;

in vec4 vPos;
in vec2 vUV;

void main() {
    gPos = vPos;
}
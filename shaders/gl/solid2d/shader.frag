#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;

uniform vec4 uColor;


void main() {
    gColor = uColor;
}
#version 310 es

#ifdef GL_ES
precision highp float;
#endif

#define OP_NORMAL 0
#define OP_ADD 1
#define OP_SUBTRACT 2
#define OP_MULTIPLY 3
#define OP_DIVIDE 4

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;
uniform int uOp;

uniform sampler2D uBase;
uniform sampler2D uColor;
uniform sampler2D uUV;

void main() {
    vec2 screenUV = gl_FragCoord.xy / uResolution;
    vec4 color = texture(uBase, screenUV);
    vec4 maskTexel = texture(uColor, screenUV);
    if (uOp == OP_NORMAL) {
        color = maskTexel;
    } else if (uOp == OP_ADD) {
        color += maskTexel;
    } else if (uOp == OP_SUBTRACT) {
        color -= maskTexel;
    } else if (uOp == OP_MULTIPLY) {
        color *= maskTexel;
    } else if (uOp == OP_DIVIDE) {
        color /= maskTexel;
    }

    gColor = color;
    if (gColor.a == 0.0) discard;
}
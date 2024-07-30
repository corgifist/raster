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

uniform vec2 uUVPosition;
uniform vec2 uUVSize;
uniform float uUVAngle;
uniform vec2 uUVAnchor;

uniform bool uMaintainUVRange;

float saturateUV(float a) {
    if (a < -1.0) a = fract(a);
    if (a < 0.0) return 1.0 + a;
    return mod(a, 1.0);
}

vec2 rotate(vec2 v, float a) {
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}

void main() {
    gColor = uColor;
    vec2 uv = vUV;
    uv -= 0.5;

    uv += uUVPosition;

    uv += uUVAnchor;
    uv = rotate(uv, uUVAngle);
    uv -= uUVAnchor;

    uv *= uUVSize;

    uv += 0.5;


    if (uTextureAvailable) gColor *= texture(uTexture, uv);
    gUV = vec4(uMaintainUVRange ? vec2(saturateUV(uv.x), saturateUV(uv.y)) : uv, 0, 1);
}
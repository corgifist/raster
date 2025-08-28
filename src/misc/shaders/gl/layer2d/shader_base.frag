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

uniform float uAspectRatio;
uniform bool uAspectRatioCorrection;

SDF_UNIFORMS_PLACEHOLDER

vec2 rotate(vec2 v, float a) {
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}

SDF_DISTANCE_FUNCTIONS_PLACEHOLDER

void main() {
    gColor = uColor;
    vec2 uv = vUV;
    uv -= 0.5;
    if (uAspectRatioCorrection) uv.x *= uAspectRatio;

    uv += uUVPosition;

    uv = rotate(uv, uUVAngle);

    uv *= uUVSize;

    uv += 0.5;

    float d = SDF_DISTANCE_FUNCTION_PLACEHOLDER(uv - 0.5);
    if (d > 0.0) gColor *= vec4(0);

    if (uTextureAvailable) gColor *= texture(uTexture, uv);
    
    gUV = vec4(uv - 0.5, uAspectRatio, 1);
}
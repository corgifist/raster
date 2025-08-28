#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;

uniform vec2 uResolution;

SDF_UNIFORMS_PLACEHOLDER

SDF_DISTANCE_FUNCTIONS_PLACEHOLDER

void main() {
    gColor = vec4(1);
    vec2 uv = gl_FragCoord.xy / uResolution;
    uv -= 0.5;
    uv.x *= uResolution.x / uResolution.y;

    float d = SDF_DISTANCE_FUNCTION_PLACEHOLDER(uv);
    if (d > 0.0) gColor = vec4(0);
}
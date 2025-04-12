#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gPos;

uniform sampler2D uA, uB;
uniform vec2 uResolution;

in vec4 vPos;
in vec2 vUV;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution.xy;
    vec4 a = texture(uA, uv);
    vec4 b = texture(uB, uv);
    if (a.a != 0.0 && b.a == 0.0) gPos = a;
    if (a.a == 0.0 && b.a != 0.0) gPos = b;
    if (a.a != 0.0 && b.a != 0.0) {
        if (a.z < b.z) {
            gPos = a;
        } else {
            gPos = b;
        }
    }
}
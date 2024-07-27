#version 310 es

#ifdef GL_ES
precision highp float;
#endif

const vec2 vertices[6] = vec2[](
    vec2(-1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0)
);

const vec2 uv[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0)
);

out vec2 vUV;

uniform mat4 uMatrix;
uniform mat4 uUVMatrix;

void main() {
    gl_Position = uMatrix * vec4(vertices[gl_VertexID], 0.0, 1.0);

    vUV = (uUVMatrix * vec4(uv[gl_VertexID]- 0.5, 0.0, 1.0)).xy + 0.5;
}
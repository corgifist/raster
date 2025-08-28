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

uniform mat4 uMatrix;

void main() {
    gl_Position = uMatrix * vec4(vertices[gl_VertexID], 0.0, 1.0);
}
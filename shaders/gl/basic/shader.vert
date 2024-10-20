#version 310 es

#ifdef GL_ES
precision highp float;
#endif

const vec2 vertices[3] = vec2[3](
    vec2(-1,-1), vec2(3,-1), vec2(-1, 3)
);


void main() {
    gl_Position = vec4(vertices[gl_VertexID], 0.0, 1.0);
}
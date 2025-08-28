#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec4 uColor;
uniform vec2 uResolution;

uniform sampler2D uTexture;

void main() {
    gColor = texture(uTexture, gl_FragCoord.xy / uResolution) * uColor;
    gUV = vec4(gl_FragCoord.xy / uResolution, 0.0, 1.0);
}
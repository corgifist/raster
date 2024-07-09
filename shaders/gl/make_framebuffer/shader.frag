#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 g_fragColor;

uniform vec4 uColor;
uniform vec2 uResolution;

uniform sampler2D uTexture;

void main() {
    g_fragColor = texture2D(uTexture, gl_FragCoord.xy / uResolution) * uColor;
}
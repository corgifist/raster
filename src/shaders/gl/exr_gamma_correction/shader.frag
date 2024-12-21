#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;

uniform vec2 uResolution;
uniform sampler2D uTexture;

void main() {
    vec4 texel = texture(uTexture, gl_FragCoord.xy / uResolution);
    gColor = vec4(pow(texel.rgb, vec3(0.454545455)), texel.a);
}
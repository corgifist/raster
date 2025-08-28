#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;

uniform vec2 uResolution;

uniform float uAlpha;
uniform bool uReplaceAlpha;
uniform int uChannel;
uniform sampler2D uBase;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 baseTexel = texture(uBase, uv);
    vec4 result = vec4(0.0);
    if (uChannel == 0) result.rgba = vec4(baseTexel.r);
    if (uChannel == 1) result.rgba = vec4(baseTexel.g);
    if (uChannel == 2) result.rgba = vec4(baseTexel.b);
    if (uChannel == 3) result.rgba = vec4(baseTexel.a);
    if (uReplaceAlpha) result.a = uAlpha;
    gColor = result; 
}
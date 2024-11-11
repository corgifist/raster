#version 310 es

#ifdef GL_ES
precision highp float;
#endif

// This shader was taken from https://www.shadertoy.com/view/NtScz1
// And modified in order to be compatible with Raster
// Many thanks to https://www.shadertoy.com/user/zsjasper !

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform vec2 uPosition;
uniform float uRadius;
uniform vec4 uFirstColor;
uniform vec4 uSecondColor;

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * uResolution) / uResolution.y;
	gColor = mix(uFirstColor, uSecondColor, 
        1.0 - length(uPosition - uv) - (1.-uRadius));
    
    gUV = vec4(uv, 0., 1.);
}
#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform vec4 uFirstColor;
uniform vec4 uSecondColor;

uniform vec2 uPosition;
uniform vec2 uSize;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    uv.x *= uResolution.x/uResolution.y;
    uv *= 10.0 * uSize;
    uv += uPosition;
    
    float u = 1.0 - floor(mod(uv.x, 2.0));
    float v = 1.0 - floor(mod(uv.y, 2.0));
    float mask = 1.0;

    if(u == 1.0 && v < 1.0 || v == 1.0 && u < 1.0) {
    	mask = 0.0;
    }

    gColor = mix(uFirstColor, uSecondColor, mask);
    gUV = vec4(gl_FragCoord.xy / uResolution, 0.0, 1.0) * mask;
}
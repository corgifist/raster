#version 310 es

#ifdef GL_ES
precision highp float;
#endif

// This shader uses code from https://www.shadertoy.com/view/NscGDf
// Many thanks to Xor (https://www.shadertoy.com/user/Xor) on Shadertoy!


layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;
uniform float uLinearBlurIntensity;
uniform float uAngle;

uniform int uSamples;
uniform sampler2D uTexture;
uniform float uOpacity;

#define SAMPLES abs(float(uSamples))

vec4 blur_linear(sampler2D tex, vec2 texel, vec2 uv, vec2 line) {
    vec4 total = vec4(0);
    
    float dist = 1.0/SAMPLES;
    for(float i = -0.5; i<=0.5; i+=dist)
    {
        vec2 coord = uv+i*line*texel;
        total += texture(tex, coord);
    }
    
    return total * dist;
}

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec2 texel = 1.0 / uResolution;

    vec2 intensity = vec2(cos(radians(uAngle)), sin(radians(uAngle)));
    intensity *= 0.1 * uLinearBlurIntensity * uResolution;

    vec4 result = blur_linear(uTexture, texel, uv, intensity);
    result = mix(texture(uTexture, uv), result, uOpacity);

    gColor = result;

    uv -= 0.5;
    uv.x *= uResolution.x / uResolution.y;
    gUV = vec4(uv, 1., 1.);
}
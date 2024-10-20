#version 310 es

#ifdef GL_ES
precision highp float;
#endif

// This shader uses code from https://www.shadertoy.com/view/NscGDf
// Many thanks to Xor (https://www.shadertoy.com/user/Xor) on Shadertoy!


layout(location = 0) out vec4 gColor;

uniform vec2 uResolution;
uniform vec2 uLinearBlurIntensity;

uniform float uSamples;
uniform sampler2D uTexture;

#define SAMPLES uSamples

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

    vec4 result = blur_linear(uTexture, texel, uv, uLinearBlurIntensity);

    gColor = result;
}
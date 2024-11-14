#version 310 es

#ifdef GL_ES
precision highp float;
#endif

// This shader uses code from https://www.shadertoy.com/view/NscGDf
// Many thanks to Xor (https://www.shadertoy.com/user/Xor) on Shadertoy!

layout(location = 0) out vec4 gColor;

uniform vec2 uResolution;
uniform vec2 uRadialBlurIntensity;
uniform vec2 uCenter;

uniform float uSamples;
uniform sampler2D uTexture;

uniform float uOpacity;

#define SAMPLES uSamples

vec4 blur_radial(sampler2D tex, vec2 texel, vec2 uv, vec2 radius)
{
    vec4 total = vec4(0);
    
    float dist = 1.0/SAMPLES;
    vec2 rad = radius * length(texel);
    for(float i = 0.0; i<=1.0; i+=dist)
    {
        vec2 coord = (uv - uCenter) / (1.0+rad*i)+0.5;
        total += texture(tex,coord - (vec2(0.5) - uCenter));
    }
    
    return total * dist;
}

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec2 texel = 1.0 / uResolution;

    vec4 result = blur_radial(uTexture, texel, uv, uRadialBlurIntensity);
    result.a *= uOpacity;

    gColor = result;
    uv -= 0.5;
    uv.x *= uResolution.x / uResolution.y;
    gUV = vec4()
}
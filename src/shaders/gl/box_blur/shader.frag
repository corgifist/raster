#version 310 es

#ifdef GL_ES
precision highp float;
#endif

// This shader uses code from https://www.shadertoy.com/view/NscGDf
// Many thanks to Xor (https://www.shadertoy.com/user/Xor) on Shadertoy!


layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;
uniform vec2 uBoxBlurIntensity;

uniform int uSamples;
uniform sampler2D uTexture;
uniform float uOpacity;

#define SAMPLES abs(float(uSamples))

vec4 blur_box(sampler2D tex, vec2 texel, vec2 uv, vec2 rect)
{
    vec4 total = vec4(0);
    
    float dist = inversesqrt(SAMPLES);
    for(float i = -0.5; i<=0.5; i+=dist)
    for(float j = -0.5; j<=0.5; j+=dist)
    {
        vec2 coord = uv+vec2(i,j)*rect*texel;
        total += texture(tex,coord);
    }
    
    return total * dist * dist;
}


void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec2 texel = 1.0 / uResolution;


    vec4 result = blur_box(uTexture, texel, uv, uBoxBlurIntensity * 0.1 * uResolution);
    result = mix(texture(uTexture, uv), result, uOpacity);

    gColor = result;
    uv -= 0.5;
    uv.x *= uResolution.x / uResolution.y;
    gUV = vec4(uv, uResolution.x / uResolution.y, 1.0);
}
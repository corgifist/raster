#version 310 es

#ifdef GL_ES
precision highp float;
#endif

// This shader uses code from https://www.shadertoy.com/view/NscGDf
// Many thanks to Xor (https://www.shadertoy.com/user/Xor) on Shadertoy!

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;
uniform float uAngularBlurAngle;
uniform vec2 uCenter;
uniform float uOpacity;

uniform float uSamples;
uniform sampler2D uTexture;

#define SAMPLES uSamples

vec4 blur_angular(sampler2D tex, vec2 texel, vec2 uv, float angle)
{
    vec4 total = vec4(0);
    vec2 coord = uv - uCenter;
    
    float dist = 1.0/SAMPLES;
    vec2 dir = vec2(cos(angle*dist),sin(angle*dist));
    mat2 rot = mat2(dir.xy,-dir.y,dir.x);
    for(float i = 0.0; i<=1.0; i+=dist)
    {
        total += texture(tex, coord + uCenter);
        coord *= rot;
    }
    
    return total * dist;
}


void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec2 texel = 1.0 / uResolution;

    vec4 result = blur_angular(uTexture, texel, uv, uAngularBlurAngle);
    result.a *= uOpacity;

    gColor = result;
    gUV = vec4(uv, 1.0, 1.0);
}
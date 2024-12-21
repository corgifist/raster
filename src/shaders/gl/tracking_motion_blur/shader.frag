#version 310 es

#ifdef GL_ES
precision highp float;
#endif

// This shader uses code from https://www.shadertoy.com/view/NscGDf
// Many thanks to Xor (https://www.shadertoy.com/user/Xor) on Shadertoy!


layout(location = 0) out vec4 gColor;

uniform vec2 uResolution;
uniform vec2 uLinearBlurIntensity;
uniform float uAngularBlurAngle;
uniform vec2 uRadialBlurIntensity;
uniform vec2 uCenter;

uniform int uStage;
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

    vec4 result = vec4(vec3(0), 1);
    if (uStage == 0) result = blur_linear(uTexture, texel, uv, uLinearBlurIntensity);
    if (uStage == 1) result = blur_angular(uTexture, texel, uv, uAngularBlurAngle);
    if (uStage == 2) result = blur_radial(uTexture, texel, uv, uRadialBlurIntensity);

    gColor = result;
}
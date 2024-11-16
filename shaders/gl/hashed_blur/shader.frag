#version 310 es

#ifdef GL_ES
precision highp float;
#endif

// Hashed blur
// David Hoskins.
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// This shader was taken from https://www.shadertoy.com/view/XdjSRw
// And modified in order to be compatible with Raster


#define TAU  6.28318530718

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform int uIterations;
uniform vec2 uHashOffset;

uniform vec2 uResolution;
uniform sampler2D uTexture;

uniform float uRadius;

//-------------------------------------------------------------------------------------------
// Use last part of hash function to generate new random radius and angle...
vec2 Sample(inout vec2 r)
{
    r = fract(r * vec2(33.3983, 43.4427));
    return r-.5;
    //return sqrt(r.x+.001) * vec2(sin(r.y * TAU), cos(r.y * TAU))*.5; // <<=== circular sampling.
}

//-------------------------------------------------------------------------------------------
#define HASHSCALE 443.8975
vec2 Hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * HASHSCALE);
    p3 += dot(p3, p3.yzx+19.19);
    return fract(vec2((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y));
}

//-------------------------------------------------------------------------------------------
vec4 Blur(vec2 uv, float radius)
{
	radius = radius * .04;
    
    vec2 circle = vec2(radius) * vec2((uResolution.x / uResolution.y), 1.0);
    
	// Remove the time reference to prevent random jittering if you don't like it.
	vec2 random = Hash22(uv + uHashOffset);

    // Do the blur here...
	vec4 acc = vec4(0.0);
	for (int i = 0; i < uIterations; i++)
    {
		acc += texture(uTexture, uv + circle * Sample(random), radius*10.0);
    }
	return acc / float(uIterations);
}

//-------------------------------------------------------------------------------------------
void main() {
	vec2 uv = gl_FragCoord.xy / uResolution;
    uv.y = 1.0 - uv.y;
 
	gColor = Blur(uv * vec2(1.0, -1.0), uRadius);
    uv.y = 1.0 - uv.y;
    gUV = vec4(uv, 1.0, 1.0);
}
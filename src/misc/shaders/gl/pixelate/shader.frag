#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;
#define fragColor gColor
#define iResolution uResolution
#define iChannel0 uColor
#define fragCoord gl_FragCoord

uniform sampler2D uColor;
uniform float uIntensity;

// Adapted for Raster from: https://www.shadertoy.com/view/ldy3Wz
// License: Public Domain

void main() {
	// set the block size on the x a    nd y axes	
    vec2 BLOCK_SIZE = vec2(0.01) * max(0.00001, uIntensity);
    vec2 uv = fragCoord.xy / iResolution.xy;
    
    // calculate uv space coordinates (between 0.0 .. 1.0)
    vec2 uvOriginal = fragCoord.xy / iResolution.xy;
	
    // "pixelate" the uv coordinates, by performing a floor() function.
    // this makes all the pixels in the block sample their color from the same positiokn
    vec2 uvPixels = vec2(0.0, 0.0);
    uvPixels.x = floor(uvOriginal.x / BLOCK_SIZE.x) * BLOCK_SIZE.x;
    uvPixels.y = floor(uvOriginal.y / BLOCK_SIZE.y) * BLOCK_SIZE.y;   
    
    fragColor = texture(iChannel0, uvPixels);
    uv -= 0.5;
    uv.x *= uResolution.x / uResolution.y;
    gUV = vec4(uv, uResolution.x / uResolution.y, 1.0);
}
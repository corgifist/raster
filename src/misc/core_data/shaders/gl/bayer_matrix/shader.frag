#version 310 es

#ifdef GL_ES
precision highp float;
#endif

// Ported from https://www.shadertoy.com/view/XtV3RG
// Many thanks to https://www.shadertoy.com/user/MartyMcFly on Shadertoy!

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;
uniform sampler2D uTexture;
uniform sampler2D uUVTexture;
uniform float uOpacity;
uniform bool uScreenSpaceRendering;
uniform int uLevel;

float GetBayerFromCoordLevel(vec2 pixelpos)
{
    ivec2 ppos = ivec2(pixelpos);
    int sum = 0;
    for(int i = 0; i<uLevel; i++)
    {
         ivec2 t = ppos & 1;
         sum = sum * 4 | (t.x ^ t.y) * 2 | t.x;
         ppos /= 2;
    }    
    return float(sum) / float(1 << (2 * uLevel));
}

vec4 generateBayerMatrix(vec2 uv) {
    return vec4(vec3(GetBayerFromCoordLevel(floor(uv*exp2(float(uLevel))))), 1.0);
}

void main() {
	vec2 screenUV = gl_FragCoord.xy / uResolution;
    if (uScreenSpaceRendering) {
        gColor = generateBayerMatrix(screenUV);
        screenUV -= 0.5;
        screenUV.x *= uResolution.x / uResolution.y;
        gUV = vec4(screenUV, uResolution.x / uResolution.y, 1.0);
    } else {
        vec4 uvPixel = texture(uUVTexture, screenUV);
        if (uvPixel.a != 0.0) {
            vec4 colorPixel = texture(uTexture, screenUV);
            if (colorPixel.a != 0.0) {
                vec2 uv = vec2(uvPixel.x / uvPixel.z, uvPixel.y);
                uv += 0.5;
                gColor = mix(colorPixel, generateBayerMatrix(uv), uOpacity);
                gUV = uvPixel;
            } 
        } 
    }
}
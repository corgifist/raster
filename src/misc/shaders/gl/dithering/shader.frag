#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;
uniform float uSize;
uniform sampler2D uColor;
uniform bool uDitherTextureAvailable;
uniform sampler2D uDitherTexture;
uniform float uOpacity;

// Ported from https://www.shadertoy.com/view/XsVBW1
// Many thanks to https://www.shadertoy.com/user/Klems on Shadertoy!

float GetBayerFromCoordLevel(vec2 pixelpos)
{
    ivec2 ppos = ivec2(pixelpos);
    int sum = 0;
    for(int i = 0; i<4; i++)
    {
         ivec2 t = ppos & 1;
         sum = sum * 4 | (t.x ^ t.y) * 2 | t.x;
         ppos /= 2;
    }    
    return float(sum) / float(1 << (2 * 4));
}

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    // color to display, use floating point precision here
    vec4 pixel = texture(uColor, uv);
    vec3 col = pixel.rgb;
    
    // get some noise
    float noise = 0.0;

    if (uDitherTextureAvailable) {
        noise = texture(uDitherTexture, gl_FragCoord.xy / 8.0).r;
    } else {
        noise = GetBayerFromCoordLevel(gl_FragCoord.xy / 8.0);
    }

    col += (noise-0.5)/uSize*2.0;
    
    col = (floor(col*uSize)+0.5)/uSize;
    gColor = vec4(col, pixel.a * uOpacity);
    uv -= 0.5;
    uv.x *= uResolution.x / uResolution.y;
    gUV = vec4(uv, uResolution.x / uResolution.y, 1.0);
}
#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform sampler2D uColorTexture;
uniform vec2 uResolution;
uniform vec4 uBackgroundColor;
uniform vec4 uOutlineColor;
uniform float uIntensity;
uniform float uBackgroundAlpha;

// This shader was taken from https://www.shadertoy.com/view/7tj3Wh
// And was modified in order to be compatible with Raster
// Many thanks to portponky (https://www.shadertoy.com/user/portponky) on Shadertoy!


bool inside(vec2 uv)
{
    return texture(uColorTexture, uv).a != 0.0;
}

void main()
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 fragCoord = gl_FragCoord.xy;
    vec2 uv = fragCoord / uResolution;

    vec2 right = vec2(uIntensity, 0.0);
    vec2 down = vec2(0.0, uIntensity);
    
    vec4 x = mix(uBackgroundColor, texture(uColorTexture, uv), uBackgroundAlpha);
    if (!inside(uv))
    {
        if (inside((fragCoord + right) / uResolution) ||
            inside((fragCoord + down) / uResolution) ||
            inside((fragCoord - right) / uResolution) ||
            inside((fragCoord - down) / uResolution) ||
            inside((fragCoord + right / 2.0) / uResolution) ||
            inside((fragCoord - right / 2.0) / uResolution) ||
            inside((fragCoord - down + right / 2.0) / uResolution) ||
            inside((fragCoord + down - right / 2.0) / uResolution) ||
            inside((fragCoord + down - right / 2.0) / uResolution) ||
            inside((fragCoord - down + right / 2.0) / uResolution)) 
            x = uOutlineColor;
    }
    // Output to screen
    gColor = x;
    uv -= 0.5;
    uv.x *= uResolution.x / uResolution.y;
    gUV = vec4(uv, 1.0, 1.0);
}
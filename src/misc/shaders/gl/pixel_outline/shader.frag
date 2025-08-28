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
uniform int uIntensity;
uniform bool uOnlyOutline;

// This shader was taken from https://www.shadertoy.com/view/7tj3Wh
// And was modified in order to be compatible with Raster
// Many thanks to portponky (https://www.shadertoy.com/user/portponky) on Shadertoy!



bool inside(vec2 uv)
{
    if (uv.x > 1.0 || uv.y > 1.0 || uv.x < 0.0 || uv.y < 0.0) return false;
    return texture(uColorTexture, uv).a != 0.0;
}

void main()
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 fragCoord = gl_FragCoord.xy;
    vec2 uv = fragCoord / uResolution;

    vec2 right = vec2(float(uIntensity), 0.0);
    vec2 down = vec2(0.0, float(uIntensity));
    
    vec4 x = mix(uBackgroundColor, texture(uColorTexture, uv), uOnlyOutline ? 0.0 : 1.0);
    if (!inside(uv))
    {
        int subsamplingAmount = uIntensity;
        float angleStep = 360.0 / (4.0 * float(subsamplingAmount));
        for (int i = 0; i < 4 * subsamplingAmount + 1; i++) {
            float currentAngle = radians(float(i) * angleStep);
            vec2 direction = vec2(cos(currentAngle), sin(currentAngle)) * float(uIntensity);
            if (inside((fragCoord + direction) / uResolution)) {
                x = uOutlineColor;
                break;
            }
        }
    }
    // Output to screen
    gColor = x;
    uv -= 0.5;
    uv.x *= uResolution.x / uResolution.y;
    gUV = vec4(uv, 1.0, 1.0);
}
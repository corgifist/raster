#version 310 es

#ifdef GL_ES
precision highp float;
#endif

// This shader was taken from https://www.shadertoy.com/view/NtScz1
// And modified in order to be compatible with Raster
// Many thanks to https://www.shadertoy.com/user/zsjasper !

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform vec2 uPosition;
uniform float uRadius;
uniform vec4 uFirstColor;
uniform vec4 uSecondColor;
uniform float uOpacity;

uniform sampler2D uColorTexture;
uniform sampler2D uUVTexture;
uniform bool uScreenSpaceRendering;

void main() {
    vec2 screenUV = gl_FragCoord.xy / uResolution;
    if (uScreenSpaceRendering) {
        vec2 uv = (gl_FragCoord.xy - 0.5 * uResolution) / uResolution.y;
        gColor = mix(uFirstColor, uSecondColor, 
            1.0 - length(uPosition - uv) - (1.-uRadius));
        gColor.a *= uOpacity;
        
        gUV = vec4(uv, 1., 1.);
    } else {
        vec4 uvPixel = texture(uUVTexture, screenUV);
        if (uvPixel.a != 0.0) {
            vec2 uv = uvPixel.rg;
            vec4 colorPixel = texture(uColorTexture, screenUV);
            if (colorPixel.a != 0.0) {
                gColor = mix(uFirstColor, uSecondColor, 
                    1.0 - length(uPosition - uv) - (1.-uRadius));
                gColor = mix(texture(uColorTexture, gl_FragCoord.xy / uResolution), gColor, uOpacity);
                gUV = vec4(uv, uvPixel.b, 1.);
            }
        }
    }
}
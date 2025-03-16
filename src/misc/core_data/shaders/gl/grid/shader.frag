#version 310 es

// 🏁 Basic Grid Shader
// License CC0-1.0

// Ported from https://www.shadertoy.com/view/4Xy3RG and adapted for Raster!
// Many thanks to https://www.shadertoy.com/user/gllama on Shadertoy

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;
uniform vec4 uBackgroundColor;
uniform vec4 uGridColor;
uniform float uInterval;
uniform float uThickness;

uniform vec2 uOffset;

uniform sampler2D uColorTexture;
uniform sampler2D uUVTexture;

uniform float uOpacity;
uniform bool uScreenSpaceRendering;

vec4 calculateGrid(vec2 uv) {
    float thicc = uThickness * .005;
    float interval = .07;

    vec4 col = uBackgroundColor;

    float offset = (thicc/2.0) - ((1.-interval)/2.);
    if(mod(uv.x+offset,interval)<thicc || mod(uv.y+offset,interval)<thicc){
        col = uGridColor;
    }

    return col;
}

void main() {
    vec2 screenUV = gl_FragCoord.xy / uResolution;
    if (uScreenSpaceRendering) {
        screenUV -= 0.5;
        screenUV.x *= uResolution.x / uResolution.y;
        screenUV *= 1.0 / uInterval;
        screenUV += uOffset;
        gColor = calculateGrid(screenUV);
        gColor.a *= uOpacity;
        gUV = vec4(screenUV, uResolution.x / uResolution.y, 1.0);
    } else {
        vec4 uvPixel = texture(uUVTexture, screenUV);
        if (uvPixel.a != 0.0) {
            vec4 colorPixel = texture(uColorTexture, screenUV);
            if (colorPixel.a != 0.0) {
                vec2 uv = vec2(uvPixel.x / uvPixel.z, uvPixel.y);
                uv *= 1.0 / uInterval;
                uv += uOffset;
                gColor = mix(colorPixel, calculateGrid(uv), uOpacity);
                gUV = uvPixel;
            }
        } 
    }
}
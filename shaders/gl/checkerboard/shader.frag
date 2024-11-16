#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform vec4 uFirstColor;
uniform vec4 uSecondColor;

uniform vec2 uPosition;
uniform vec2 uSize;

uniform bool uScreenSpaceRendering;
uniform float uOpacity;
uniform sampler2D uColorTexture;
uniform sampler2D uUVTexture;

void main() {
    vec2 screenUV = gl_FragCoord.xy / uResolution;
    gColor = vec4(0.);
    if (uScreenSpaceRendering) {
        vec2 uv = screenUV;
        uv.x *= uResolution.x/uResolution.y;
        uv *= 10.0 * uSize;
        uv += uPosition;
        
        float u = 1.0 - floor(mod(uv.x, 2.0));
        float v = 1.0 - floor(mod(uv.y, 2.0));
        float mask = 1.0;

        if(u == 1.0 && v < 1.0 || v == 1.0 && u < 1.0) {
            mask = 0.0;
        }

        gColor = mix(uFirstColor, uSecondColor, mask);
        gUV = vec4(uv, 1.0, 1.0);
    } else {
        vec4 uvPixel = texture(uUVTexture, screenUV);
        if (uvPixel.a != 0.0) {
            vec4 colorPixel = texture(uColorTexture, screenUV);
            if (colorPixel.a != 0.0) {
                vec2 uv = uvPixel.xy;
                uv += 0.5;
                uv *= 10.0 * uSize;
                uv += uPosition;
                
                float u = 1.0 - floor(mod(uv.x, 2.0));
                float v = 1.0 - floor(mod(uv.y, 2.0));
                float mask = 1.0;

                if(u == 1.0 && v < 1.0 || v == 1.0 && u < 1.0) {
                    mask = 0.0;
                }

                gColor = mix(uFirstColor, uSecondColor, mask);
                // gColor = uvPixel;
                gUV = vec4(uv, uvPixel.z, 1.0);
            } else discard;
        } else discard;
    }

    gColor.a *= uOpacity;
}
#version 310 es

#ifdef GL_ES
precision highp float;
#endif

// Ruofei Du
// Dot Screen / Halftone: https://www.shadertoy.com/view/4sBBDK
// Halftone: https://www.shadertoy.com/view/lsSfWV

// Modified in order to be compatible with Raster

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;
uniform sampler2D uTexture;
uniform sampler2D uUVTexture;
uniform float uOpacity;
uniform bool uScreenSpaceRendering;

uniform vec2 uOffset;
uniform vec4 uColor;
uniform float uAngle;
uniform float uScale;

float greyScale(in vec3 col) {
    return dot(col, vec3(0.2126, 0.7152, 0.0722));
}

mat2 rotate2d(float angle){
    return mat2(cos(angle), -sin(angle), sin(angle),cos(angle));
}

float dotScreen(in vec2 uv, in float angle, in float scale) {
    float s = sin( angle ), c = cos( angle );
	vec2 p = (uv - vec2(0.5) + uOffset) * uResolution;
    vec2 q = rotate2d(angle) * p * scale; 
	return ( sin( q.x ) * sin( q.y ) ) * 4.0;
}

void main() {
	vec2 screenUV = gl_FragCoord.xy / uResolution;
    if (uScreenSpaceRendering) {
        vec3 col = texture(uTexture, screenUV).rgb; 
        float grey = greyScale(col); 
        float angle = radians(uAngle);
        float scale = uScale; 
        col = vec3( grey * 10.0 - 5.0 + dotScreen(screenUV, angle, scale ) );
        gColor = vec4( col, 1.0 ) * uColor;
        gColor.a *= uOpacity;
        gUV = vec4(screenUV, 1., 1.);
    } else {
        vec4 uvPixel = texture(uUVTexture, screenUV);
        if (uvPixel.a != 0.0) {
            vec4 colorPixel = texture(uTexture, screenUV);
            if (colorPixel.a != 0.0) {
                vec2 uv = vec2(uvPixel.x / uvPixel.z, uvPixel.y);
                uv += 0.5;
                float grey = greyScale(colorPixel.rgb); 
                float angle = radians(uAngle);
                float scale = uScale; 
                colorPixel.rgb = vec3( grey * 10.0 - 5.0 + dotScreen(uv, angle, scale ) );
                gColor = vec4( colorPixel.rgb, 1.0 ) * uColor;
                gColor.a *= uOpacity;
                gUV = uvPixel;
            } else discard;
        } else discard;
    }
}
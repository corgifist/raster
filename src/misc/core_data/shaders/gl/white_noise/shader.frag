#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;
uniform sampler2D uColorTexture;
uniform sampler2D uUVTexture;
uniform bool uScreenSpaceRendering;
uniform float uOpacity;

uniform float uSeed1;
uniform float uSeed2;
uniform float uSeed3;

// Ported from https://www.shadertoy.com/view/tlcBRl
// Many thanks to https://www.shadertoy.com/user/kinonik on Shadertoy!

float noise1(float seed1,float seed2);

float noise2(float seed1,float seed2);

float noise2(float seed1,float seed2,float seed3);

float noise3(float seed1,float seed2,float seed3);

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec4 generateNoise(vec2 uv) {
    float c = noise3(uv.x*0.000001 * uSeed1, uv.y*0.000001 * uSeed2, uSeed3); 
    return vec4(vec3(c), 1.0);
}

void main()
{
    vec2 screenUV = gl_FragCoord.xy / uResolution;
    if (uScreenSpaceRendering) {
        gColor = generateNoise(screenUV);
        screenUV -= 0.5;
        screenUV.x *= uResolution.x / uResolution.y;
        gUV = vec4(screenUV, uResolution.x / uResolution.y, 1.0);
    } else {
        vec4 uvPixel = texture(uUVTexture, screenUV);
        if (uvPixel.a != 0.0) {
            vec4 colorPixel = texture(uColorTexture, screenUV);
            if (colorPixel.a != 0.0) {
                vec2 uv = vec2(uvPixel.x / uvPixel.z, uvPixel.y);
                uv += 0.5;
                gColor = mix(colorPixel, generateNoise(uv), uOpacity);
                gUV = uvPixel;
            } else discard;
        } else discard;
    }
}

//mini
float noise1(float seed1,float seed2){
    return(
    fract(seed1+12.34567*
    fract(100.*(abs(seed1*0.91)+seed2+94.68)*
    fract((abs(seed2*0.41)+45.46)*
    fract((abs(seed2)+757.21)*
    fract(seed1*0.0171))))))
    * 1.0038 - 0.00185;
}

//2 seeds
float noise2(float seed1,float seed2){
    float buff1 = abs(seed1+100.94) + 1000.;
    float buff2 = abs(seed2+100.73) + 1000.;
    buff1 = (buff1*fract(buff2*fract(buff1*fract(buff2*0.63))));
    buff2 = (buff2*fract(buff2*fract(buff1+buff2*fract(seed1*0.79))));
    buff1 = noise1(buff1, buff2);
    return(buff1 * 1.0038 - 0.00185);
}

//3 seeds
float noise2(float seed1,float seed2,float seed3){
    float buff1 = abs(seed1+100.81) + 1000.3;
    float buff2 = abs(seed2+100.45) + 1000.2;
    float buff3 = abs(noise1(seed1, seed2)+seed3) + 1000.1;
    buff1 = (buff3*fract(buff2*fract(buff1*fract(buff2*0.146))));
    buff2 = (buff2*fract(buff2*fract(buff1+buff2*fract(buff3*0.52))));
    buff1 = noise1(buff1, buff2);
    return(buff1);
}

//3 seeds hard
float noise3(float seed1,float seed2,float seed3){
    float buff1 = abs(seed1+100.813) + 1000.314;
    float buff2 = abs(seed2+100.453) + 1000.213;
    float buff3 = abs(noise1(buff2, buff1)+seed3) + 1000.17;
    buff1 = (buff3*fract(buff2*fract(buff1*fract(buff2*0.14619))));
    buff2 = (buff2*fract(buff2*fract(buff1+buff2*fract(buff3*0.5215))));
    buff1 = noise2(noise1(seed2,buff1), noise1(seed1,buff2), noise1(seed3,buff3));
    return(buff1);
}
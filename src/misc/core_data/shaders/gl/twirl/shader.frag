#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform float uOpacity;
uniform sampler2D uTexture;

uniform vec2 uCenter;
uniform float uStrength;
uniform float uRange;

// Ported from https://www.shadertoy.com/view/slfGzN

mat2 rotate(float a)
{
    float s = sin(a);
    float c = cos(a);
    return mat2(c,-s,s,c);
}
vec2 twirl(vec2 uv, vec2 center, float range, float strength) {
    float d = distance(uv,center);
    uv -=center;
    // d = clamp(0.,strength,-strength/range * d + strength);
    d = smoothstep(0.,range,range-d) * strength;
    uv *= rotate(d);
    uv+=center;
    return uv;
}


void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    gColor = texture(uTexture, twirl(uv, (uCenter * vec2(1., -1.) + vec2(0.5)) * vec2(1./(uResolution.x/uResolution.y), 1.), abs(uRange), -uStrength));
    gColor = mix(texture(uTexture, uv), gColor, uOpacity);
    uv -= 0.5;
    uv.x *= uResolution.x / uResolution.y;
    gUV = vec4(uv, uResolution.x / uResolution.y, 1.0);
}
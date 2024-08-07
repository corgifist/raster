#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

in vec2 vUV;

uniform vec4 uColor;

uniform bool uTextureAvailable;
uniform sampler2D uTexture;

uniform vec2 uUVPosition;
uniform vec2 uUVSize;
uniform float uUVAngle;
uniform vec2 uUVAnchor;

uniform bool uMaintainUVRange;

uniform vec2 uSDFRhombusSize;
uniform vec2 uSDFHeartSize;
uniform float uSDFMixPhase;
uniform float uSDFCircleRadius;
uniform float uSDFMixPhase;


float saturateUV(float a) {
    if (a < -1.0) a = fract(a);
    if (a < 0.0) return 1.0 + a;
    return mod(a, 1.0);
}

vec2 saturateUV(vec2 a) {
    return vec2(saturateUV(a.x), saturateUV(a.y));
}

vec2 rotate(vec2 v, float a) {
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}

float fSDFRhombusNDOT(vec2 a, vec2 b ) { return a.x*b.x - a.y*b.y; }
float fSDFRhombusMix1(vec2 p) 
{
    vec2 b = uSDFRhombusSizeMix1Mix1;
    p = abs(p);
    float h = clamp( fSDFRhombusNDOT(b-2.0*p,b)/dot(b,b), -1.0, 1.0 );
    float d = length( p-0.5*b*vec2(1.0-h,1.0+h) );
    return d * sign( p.x*b.y + p.y*b.x - b.x*b.y );
}

float fSDFHeartDOT2(in vec2 v ) { return dot(v,v); }

float fSDFHeartMix2(vec2 P)
{
    float size = uSDFHeartSizeMix2Mix1.x;
    P = vec2(P.x,-P.y)/(size/3.5); // transform to weird space (to match original shader)
    
    vec2 q = abs( vec2(P.x-P.y,P.x+P.y)/1.41421356237309504880168872420969808 ) - 1.0;
    float r1 = (min(q.x,q.y) > 0.0) ? length(q) : max(q.x,q.y); 
    float r2 = length(P - (1.41421356237309504880168872420969808/2.0)*vec2(+1.0,1.0))- 1.0;
    float r3 = length(P - (1.41421356237309504880168872420969808/2.0)*vec2(-1.0,1.0))- 1.0;
    float di = min(min(r1,r2),r3);
    
    return di * (size/3.5); // untransform from weird space (to match original shader)
}

float fSDFMixMix1(vec2 uv) {
    return mix(fSDFRhombusMix1(uv), fSDFHeartMix2(uv), uSDFMixPhaseMix1);
}



float fSDFCircleMix2(vec2 uv)
{
    return length(uv) - uSDFCircleRadiusMix2;
}

float fSDFMix(vec2 uv) {
    return mix(fSDFMixMix1(uv), fSDFCircleMix2(uv), uSDFMixPhase);
}



void main() {
    gColor = uColor;
    vec2 uv = vUV;
    uv -= 0.5;

    uv += uUVPosition;

    uv += uUVAnchor;
    uv = rotate(uv, uUVAngle);
    uv -= uUVAnchor;

    uv *= uUVSize;

    uv += 0.5;

    vec2 saturatedUV = saturateUV(uv);
    float d = fSDFMix(saturatedUV - 0.5);
    if (d > 0.0) gColor *= vec4(0);

    if (uTextureAvailable) gColor *= texture(uTexture, uv);
    gUV = vec4(uMaintainUVRange ? saturatedUV : uv, 0, 1);
}
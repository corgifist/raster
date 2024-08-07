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
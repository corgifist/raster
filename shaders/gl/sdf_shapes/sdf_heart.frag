float fSDFHeartDOT2(in vec2 v ) { return dot(v,v); }

float fSDFHeart(vec2 P)
{
    float size = uSDFHeartSize.x;
    P = vec2(P.x,-P.y)/(size/3.5); // transform to weird space (to match original shader)
    
    vec2 q = abs( vec2(P.x-P.y,P.x+P.y)/1.41421356237309504880168872420969808 ) - 1.0;
    float r1 = (min(q.x,q.y) > 0.0) ? length(q) : max(q.x,q.y); 
    float r2 = length(P - (1.41421356237309504880168872420969808/2.0)*vec2(+1.0,1.0))- 1.0;
    float r3 = length(P - (1.41421356237309504880168872420969808/2.0)*vec2(-1.0,1.0))- 1.0;
    float di = min(min(r1,r2),r3);
    
    return di * (size/3.5); // untransform from weird space (to match original shader)
}
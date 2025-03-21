float fSDFUnion(vec2 uv) {
    if (uSDFUnionSmooth) {
        float d1 = SDF_UNION_FIRST_FUNCTION_PLACEHOLDER(uv);
        float d2 = SDF_UNION_SECOND_FUNCTION_PLACEHOLDER(uv);
        float k = uSDFUnionSmoothness;
        float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
        return mix( d2, d1, h ) - k*h*(1.0-h);
    }
    return min(SDF_UNION_FIRST_FUNCTION_PLACEHOLDER(uv), SDF_UNION_SECOND_FUNCTION_PLACEHOLDER(uv));
}
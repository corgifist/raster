float fSDFSubtract(vec2 uv) {
    if (uSDFSubtractSmooth) {
        float d1 = SDF_SUBTRACT_FIRST_FUNCTION_PLACEHOLDER(uv);
        float d2 = SDF_SUBTRACT_SECOND_FUNCTION_PLACEHOLDER(uv);
        float k = uSDFSubtractSmoothness;
        float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
        return mix( d2, -d1, h ) + k*h*(1.0-h);
    }
    return max(SDF_SUBTRACT_FIRST_FUNCTION_PLACEHOLDER(uv), SDF_SUBTRACT_SECOND_FUNCTION_PLACEHOLDER(uv));
}